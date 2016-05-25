/*
 *	Sound core.  This file is composed of two parts.  sound_class
 *	which is common to both OSS and ALSA and OSS sound core which
 *	is used OSS or emulation of it.
 */

/*
 * First, the common part.
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <sound/core.h>

#ifdef CONFIG_SOUND_OSS_CORE
static int __init init_oss_soundcore(void);
static void cleanup_oss_soundcore(void);
#else
static inline int init_oss_soundcore(void)	{ return 0; }
static inline void cleanup_oss_soundcore(void)	{ }
#endif

struct class *sound_class;
EXPORT_SYMBOL(sound_class);

MODULE_DESCRIPTION("Core sound module");
MODULE_AUTHOR("Alan Cox");
MODULE_LICENSE("GPL");

static int __init init_soundcore(void)
{
	int rc;

	rc = init_oss_soundcore();  /* ��ʼ��OSS����-----����ע�����ַ��豸 */
	if (rc)
		return rc;

	sound_class = class_create(THIS_MODULE, "sound");  /* ����һ���࣬����/sys/classĿ¼�´���soundĿ¼ */
	if (IS_ERR(sound_class)) {
		cleanup_oss_soundcore();
		return PTR_ERR(sound_class);
	}

	return 0;
}

static void __exit cleanup_soundcore(void)
{
	cleanup_oss_soundcore();
	class_destroy(sound_class);
}

module_init(init_soundcore);
module_exit(cleanup_soundcore);


#ifdef CONFIG_SOUND_OSS_CORE
/*
 *	OSS sound core handling. Breaks out sound functions to submodules
 *	
 *	Author:		Alan Cox <alan@lxorguk.ukuu.org.uk>
 *
 *	Fixes:
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *                         --------------------
 * 
 *	Top level handler for the sound subsystem. Various devices can
 *	plug into this. The fact they don't all go via OSS doesn't mean 
 *	they don't have to implement the OSS API. There is a lot of logic
 *	to keeping much of the OSS weight out of the code in a compatibility
 *	module, but it's up to the driver to rember to load it...
 *
 *	The code provides a set of functions for registration of devices
 *	by type. This is done rather than providing a single call so that
 *	we can hide any future changes in the internals (eg when we go to
 *	32bit dev_t) from the modules and their interface.
 *
 *	Secondly we need to allocate the dsp, dsp16 and audio devices as
 *	one. Thus we misuse the chains a bit to simplify this.
 *
 *	Thirdly to make it more fun and for 2.3.x and above we do all
 *	of this using fine grained locking.
 *
 *	FIXME: we have to resolve modules and fine grained load/unload
 *	locking at some point in 2.3.x.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sound.h>
#include <linux/major.h>
#include <linux/kmod.h>

#define SOUND_STEP 16

struct sound_unit  /* ������Ԫ */
{
	int unit_minor;  /* ���豸�� */
	const struct file_operations *unit_fops;  /* �������� */
	struct sound_unit *next;  /* �����γ����� */
	char name[32];  /* ���� */
};

#ifdef CONFIG_SOUND_MSNDCLAS
extern int msnd_classic_init(void);
#endif
#ifdef CONFIG_SOUND_MSNDPIN
extern int msnd_pinnacle_init(void);
#endif

/*
 *	Low level list operator. Scan the ordered list, find a hole and
 *	join into it. Called with the lock asserted
 */
/* ����������Ԫ 
    ����listΪҪ����Ľڵ�
    ����listΪ��Žڵ�ı�����һ��Ϊָ������*chains[i]  (chains[0]---> mixer  chains[2]--->sequencer  chains[2]--->midi   chains[3]--->dsp
                                                                                                                             chains[4]--->audio  chains[8]---> sequencer2   chains[9]--->dmmidi
                                                                                                                               chains[10]--->dmfm   chains[12]--->adsp   chains[13]--->amidi  chains[14]--->admmidi)
                                                                                                                             
    ����indexΪ���豸��
    ����lowΪ��С���豸��
    ����topΪ�����豸��*/
static int __sound_insert_unit(struct sound_unit * s, struct sound_unit **list, const struct file_operations *fops, int index, int low, int top)
{
	int n=low;

	if (index < 0) {	/* first free */ /* ����豸���<0----->�ͷ���һ�����豸�� */

		while (*list && (*list)->unit_minor<n)  /* ����������chains[i]Ϊͷ���������� ���豸��<low�Ľڵ�*/
			list=&((*list)->next); 

		while(n<top)
		{
			/* Found a hole ? */
			if(*list==NULL || (*list)->unit_minor>n)  /* ����������chains[i]Ϊͷ���������� ���豸��>low�Ľڵ� */
				break;
			list=&((*list)->next);
			n+=SOUND_STEP;  /* n=n+16 */
		}

		if(n>=top)
			return -ENOENT;
	} else {/* ���� */
		n = low+(index*16);
		while (*list) {
			if ((*list)->unit_minor==n)  /* �����������豸����Ƚڵ㣬�������˵���ظ����򷵻ش��� */
				return -EBUSY;
			if ((*list)->unit_minor>n)
				break;
			list=&((*list)->next);
		}
	}	
		
	/*
	 *	Fill it in
	 */
	 /* ��ʼ�� */
	s->unit_minor=n;
	s->unit_fops=fops;
	
	/*
	 *	Link it
	 */
	 
	s->next=*list;  /* ���뵽����β�� */
	*list=s;
	
	
	return n;
}

/*
 *	Remove a node from the chain. Called with the lock asserted
 */
 /* ���ش��豸�ŵ���unit�Ľڵ� */
static struct sound_unit *__sound_remove_unit(struct sound_unit **list, int unit)
{
	while(*list)
	{
		struct sound_unit *p=*list;
		if(p->unit_minor==unit)
		{
			*list=p->next;
			return p;
		}
		list=&(p->next);
	}
	printk(KERN_ERR "Sound device %d went missing!\n", unit);
	return NULL;
}

/*
 *	This lock guards the sound loader list.
 */

static DEFINE_SPINLOCK(sound_loader_lock);

/*
 *	Allocate the controlling structure and add it to the sound driver
 *	list. Acquires locks as needed
 */
/* ����������Ԫ�豸 
    ����listָ��ȫ������chains[i]
    ����indexΪ�豸���
    ����lowΪ��С�豸���
    ����topΪ����豸���*/
static int sound_insert_unit(struct sound_unit **list, const struct file_operations *fops, int index, int low, int top, const char *name, umode_t mode, struct device *dev)
{
	struct sound_unit *s = kmalloc(sizeof(*s), GFP_KERNEL);  /* ����һ��struct sound_unit */
	int r;

	if (!s)
		return -ENOMEM;
		
	spin_lock(&sound_loader_lock);
	r = __sound_insert_unit(s, list, fops, index, low, top);  /* ��s���������ҷ����豸�ţ��������豸�ŷ��� */
	spin_unlock(&sound_loader_lock);
	
	if (r < 0)
		goto fail;
	else if (r < SOUND_STEP)
		sprintf(s->name, "sound/%s", name);
	else
		sprintf(s->name, "sound/%s%d", name, r / SOUND_STEP);

	device_create(sound_class, dev, MKDEV(SOUND_MAJOR, s->unit_minor),  /* �����豸�ڵ� */
		      NULL, s->name+6);
	return r;

 fail:
	kfree(s);
	return r;
}

/*
 *	Remove a unit. Acquires locks as needed. The drivers MUST have
 *	completed the removal before their file operations become
 *	invalid.
 */
 /* ɾ��һ����Ƶ��Ԫ */	
static void sound_remove_unit(struct sound_unit **list, int unit)
{
	struct sound_unit *p;

	spin_lock(&sound_loader_lock);
	p = __sound_remove_unit(list, unit);   /* ��ȡ����豸����unit��ȵĽڵ� */
	spin_unlock(&sound_loader_lock);
	if (p) {
		device_destroy(sound_class, MKDEV(SOUND_MAJOR, p->unit_minor));  /* �����豸�ļ� */
		kfree(p);   /* �ͷŽڵ� */
	}
}

/*
 *	Allocations
 *
 *	0	*16		Mixers
 *	1	*8		Sequencers
 *	2	*16		Midi
 *	3	*16		DSP
 *	4	*16		SunDSP
 *	5	*16		DSP16
 *	6	--		sndstat (obsolete)
 *	7	*16		unused
 *	8	--		alternate sequencer (see above)
 *	9	*16		raw synthesizer access
 *	10	*16		unused
 *	11	*16		unused
 *	12	*16		unused
 *	13	*16		unused
 *	14	*16		unused
 *	15	*16		unused
 */

static struct sound_unit *chains[SOUND_STEP];  /* ȫ�ֵı�����ע����豸����������������Ϊ����ͷ�������� */

/**
 *	register_sound_special_device - register a special sound node
 *	@fops: File operations for the driver
 *	@unit: Unit number to allocate
 *      @dev: device pointer
 *
 *	Allocate a special sound device by minor number from the sound
 *	subsystem. The allocated number is returned on succes. On failure
 *	a negative error code is returned.
 */
 /* ע��һ���������Ƶ�豸 */
int register_sound_special_device(const struct file_operations *fops, int unit,
				  struct device *dev)
{
	const int chain = unit % SOUND_STEP;  /* unit % 16 ȡ�� */
	int max_unit = 128 + chain;
	const char *name;
	char _name[16];

	switch (chain) {
	    case 0:
		name = "mixer";  /* ��Ƶ�豸 */
		break;
	    case 1:
		name = "sequencer";  /* �����豸 */
		if (unit >= SOUND_STEP)
			goto __unknown;
		max_unit = unit + 1;
		break;
	    case 2:
		name = "midi";  /* �Ե��豸 */
		break;
	    case 3:
		name = "dsp";  /* ��Ƶ������� */
		break;
	    case 4:
		name = "audio";  /* ��Ƶ�豸 */
		break;
	    case 8:
		name = "sequencer2";
		if (unit >= SOUND_STEP)
			goto __unknown;
		max_unit = unit + 1;
		break;
	    case 9:
		name = "dmmidi";
		break;
	    case 10:
		name = "dmfm";
		break;
	    case 12:
		name = "adsp";
		break;
	    case 13:
		name = "amidi";
		break;
	    case 14:
		name = "admmidi";
		break;
	    default:
	    	{
		    __unknown:
			sprintf(_name, "unknown%d", chain);
		    	if (unit >= SOUND_STEP)
		    		strcat(_name, "-");
		    	name = _name;
		}
		break;
	}
	return sound_insert_unit(&chains[chain], fops, -1, unit, max_unit,
				 name, S_IRUSR | S_IWUSR, dev);  /* �����豸 */
}
 
EXPORT_SYMBOL(register_sound_special_device);

/* ע��һ���������Ƶ�豸 */
int register_sound_special(const struct file_operations *fops, int unit)
{
	return register_sound_special_device(fops, unit, NULL);
}

EXPORT_SYMBOL(register_sound_special);

/**
 *	register_sound_mixer - register a mixer device
 *	@fops: File operations for the driver
 *	@dev: Unit number to allocate
 *
 *	Allocate a mixer device. Unit is the number of the mixer requested.
 *	Pass -1 to request the next free mixer unit. On success the allocated
 *	number is returned, on failure a negative error code is returned.
 */

/* ע��һ����������devΪ�豸��ţ����dev=-1����ϵͳ�Զ�����һ���豸��š�
    mixer��һ�����͵��ַ��豸����˱������Ҫ����ʱʵ�� file_operations �е�open��ioctl�Ⱥ�����
    mixer�ӿ��е�����Ҫ������ioctl����ʵ�ֻ������Ĳ�ͬ��������*/
int register_sound_mixer(const struct file_operations *fops, int dev)
{
	return sound_insert_unit(&chains[0], fops, dev, 0, 128,
				 "mixer", S_IRUSR | S_IWUSR, NULL);  /*0<=dev<=128 */
}

EXPORT_SYMBOL(register_sound_mixer);

/**
 *	register_sound_midi - register a midi device
 *	@fops: File operations for the driver
 *	@dev: Unit number to allocate
 *
 *	Allocate a midi device. Unit is the number of the midi device requested.
 *	Pass -1 to request the next free midi unit. On success the allocated
 *	number is returned, on failure a negative error code is returned.
 */
/* ע��һ���Ե��豸 */
int register_sound_midi(const struct file_operations *fops, int dev)
{
	return sound_insert_unit(&chains[2], fops, dev, 2, 130,
				 "midi", S_IRUSR | S_IWUSR, NULL);
}

EXPORT_SYMBOL(register_sound_midi);

/*
 *	DSP's are registered as a triple. Register only one and cheat
 *	in open - see below.
 */
 
/**
 *	register_sound_dsp - register a DSP device
 *	@fops: File operations for the driver
 *	@dev: Unit number to allocate
 *
 *	Allocate a DSP device. Unit is the number of the DSP requested.
 *	Pass -1 to request the next free DSP unit. On success the allocated
 *	number is returned, on failure a negative error code is returned.
 *
 *	This function allocates both the audio and dsp device entries together
 *	and will always allocate them as a matching pair - eg dsp3/audio3
 */
/* ע��һ����Ƶ������豸 ��devΪ�豸��ţ����dev=-1����ϵͳ�Զ�����һ���豸��š�
    dsp��һ�����͵��ַ��豸����˱������Ҫ����ʱʵ�� file_operations �е�open��read��write��ioctl�Ⱥ�����
    dsp�ӿ�file_operations��read��write�����ǳ���Ҫ:
    read��������Ƶ�������л�ȡ¼�����ݵ������������Ƶ��û��ռ�
    write�������û��ռ临����Ƶ���ݵ��ں˿ռ仺���������շ��͵���Ƶ������
    ioctl�������ڴ���Բ����ʡ��������ȡ�DMA���������С�Ȳ�������I/O��������Ĵ���*/
int register_sound_dsp(const struct file_operations *fops, int dev)
{
	return sound_insert_unit(&chains[3], fops, dev, 3, 131,
				 "dsp", S_IWUSR | S_IRUSR, NULL);
}

EXPORT_SYMBOL(register_sound_dsp);

/**
 *	unregister_sound_special - unregister a special sound device
 *	@unit: unit number to allocate
 *
 *	Release a sound device that was allocated with
 *	register_sound_special(). The unit passed is the return value from
 *	the register function.
 */

/* ע��һ������������豸 */
void unregister_sound_special(int unit)
{
	sound_remove_unit(&chains[unit % SOUND_STEP], unit);
}
 
EXPORT_SYMBOL(unregister_sound_special);

/**
 *	unregister_sound_mixer - unregister a mixer
 *	@unit: unit number to allocate
 *
 *	Release a sound device that was allocated with register_sound_mixer().
 *	The unit passed is the return value from the register function.
 */
/* ע�������� */
void unregister_sound_mixer(int unit)
{
	sound_remove_unit(&chains[0], unit);
}

EXPORT_SYMBOL(unregister_sound_mixer);

/**
 *	unregister_sound_midi - unregister a midi device
 *	@unit: unit number to allocate
 *
 *	Release a sound device that was allocated with register_sound_midi().
 *	The unit passed is the return value from the register function.
 */
/* ע���Ե��豸 */
void unregister_sound_midi(int unit)
{
	sound_remove_unit(&chains[2], unit);
}

EXPORT_SYMBOL(unregister_sound_midi);

/**
 *	unregister_sound_dsp - unregister a DSP device
 *	@unit: unit number to allocate
 *
 *	Release a sound device that was allocated with register_sound_dsp().
 *	The unit passed is the return value from the register function.
 *
 *	Both of the allocated units are released together automatically.
 */

/* ע�� ������豸*/
void unregister_sound_dsp(int unit)
{
	sound_remove_unit(&chains[3], unit);
}


EXPORT_SYMBOL(unregister_sound_dsp);

/*
 *	Now our file operations
 */

static int soundcore_open(struct inode *, struct file *);

static const struct file_operations soundcore_fops=
{
	/* We must have an owner or the module locking fails */
	.owner	= THIS_MODULE,
	.open	= soundcore_open,
};

/* Ѱ����Ƶ��Ԫ */
static struct sound_unit *__look_for_unit(int chain, int unit)
{
	struct sound_unit *s;
	
	s=chains[chain];
	while(s && s->unit_minor <= unit)
	{
		if(s->unit_minor==unit)
			return s;
		s=s->next;
	}
	return NULL;
}

/* �򿪺��� */
static int soundcore_open(struct inode *inode, struct file *file)
{
	int chain;
	int unit = iminor(inode);   /* ��ȡ���豸�� */
	struct sound_unit *s;
	const struct file_operations *new_fops = NULL;

	lock_kernel ();

	chain=unit&0x0F;  /* ȡ�����豸�ŵĵ�4λ */
	if(chain==4 || chain==5)	/* dsp/audio/dsp16 */
	{
		unit&=0xF0;
		unit|=3;
		chain=3;
	}
	
	spin_lock(&sound_loader_lock);
	s = __look_for_unit(chain, unit);  /* Ѱ����Ƶ��Ԫ */
	if (s)
		new_fops = fops_get(s->unit_fops);  /* ��ȡѰ���ҵ���Ƶ��Ԫ�Ĳ��������� */
	if (!new_fops) {
		spin_unlock(&sound_loader_lock);
		/*
		 *  Please, don't change this order or code.
		 *  For ALSA slot means soundcard and OSS emulation code
		 *  comes as add-on modules which aren't depend on
		 *  ALSA toplevel modules for soundcards, thus we need
		 *  load them at first.	  [Jaroslav Kysela <perex@jcu.cz>]
		 */
		request_module("sound-slot-%i", unit>>4);
		request_module("sound-service-%i-%i", unit>>4, chain);
		spin_lock(&sound_loader_lock);
		s = __look_for_unit(chain, unit);  /* �������»�ȡһ�� */
		if (s)
			new_fops = fops_get(s->unit_fops);
	}
	if (new_fops) {
		/*
		 * We rely upon the fact that we can't be unloaded while the
		 * subdriver is there, so if ->open() is successful we can
		 * safely drop the reference counter and if it is not we can
		 * revert to old ->f_op. Ugly, indeed, but that's the cost of
		 * switching ->f_op in the first place.
		 */
		int err = 0;
		const struct file_operations *old_fops = file->f_op;
		file->f_op = new_fops;  /* ָ���ȡ�����µĲ��������� */
		spin_unlock(&sound_loader_lock);
		if(file->f_op->open)
			err = file->f_op->open(inode,file);  /* ��������open���� */
		if (err) {
			fops_put(file->f_op);
			file->f_op = fops_get(old_fops);
		}
		fops_put(old_fops);
		unlock_kernel();
		return err;
	}
	spin_unlock(&sound_loader_lock);
	unlock_kernel();
	return -ENODEV;
}

MODULE_ALIAS_CHARDEV_MAJOR(SOUND_MAJOR);

/* ���oss���� */
static void cleanup_oss_soundcore(void)
{
	/* We have nothing to really do here - we know the lists must be
	   empty */
	unregister_chrdev(SOUND_MAJOR, "sound");
}

/* ��ʼ��oss ���� */
static int __init init_oss_soundcore(void)
{
	if (register_chrdev(SOUND_MAJOR, "sound", &soundcore_fops)==-1) {  /* ע��һ���ַ��豸�����豸Ϊ14 */
		printk(KERN_ERR "soundcore: sound device already in use.\n");
		return -EBUSY;
	}

	return 0;
}

#endif /* CONFIG_SOUND_OSS_CORE */
