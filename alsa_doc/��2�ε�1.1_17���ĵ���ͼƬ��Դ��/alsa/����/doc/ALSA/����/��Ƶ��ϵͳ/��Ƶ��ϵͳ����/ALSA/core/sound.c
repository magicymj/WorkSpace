/*
 *  Advanced Linux Sound Architecture
 *  Copyright (c) by Jaroslav Kysela <perex@perex.cz>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/time.h>
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <sound/core.h>
#include <sound/minors.h>
#include <sound/info.h>
#include <sound/version.h>
#include <sound/control.h>
#include <sound/initval.h>
#include <linux/kmod.h>
#include <linux/mutex.h>

static int major = CONFIG_SND_MAJOR;  /* ���������豸�� ---116*/
int snd_major; 
EXPORT_SYMBOL(snd_major);

static int cards_limit = 1;

MODULE_AUTHOR("Jaroslav Kysela <perex@perex.cz>");
MODULE_DESCRIPTION("Advanced Linux Sound Architecture driver for soundcards.");
MODULE_LICENSE("GPL");
module_param(major, int, 0444);
MODULE_PARM_DESC(major, "Major # for sound driver.");
module_param(cards_limit, int, 0444);
MODULE_PARM_DESC(cards_limit, "Count of auto-loadable soundcards.");
MODULE_ALIAS_CHARDEV_MAJOR(CONFIG_SND_MAJOR);

/* this one holds the actual max. card number currently available.
 * as default, it's identical with cards_limit option.  when more
 * modules are loaded manually, this limit number increases, too.
 */
int snd_ecards_limit;  /* ����������������� */
EXPORT_SYMBOL(snd_ecards_limit);

static struct snd_minor *snd_minors[SNDRV_OS_MINORS];  /* ������еĴ��豸�������Ϣ */
static DEFINE_MUTEX(sound_mutex);

#ifdef CONFIG_MODULES

/**
 * snd_request_card - try to load the card module
 * @card: the card number
 *
 * Tries to load the module "snd-card-X" for the given card number
 * via request_module.  Returns immediately if already loaded.
 */
 /* ���Լ���һ������ģ��
    ����cardΪ���Ĵ��豸��*/
void snd_request_card(int card)
{
	if (snd_card_locked(card)) /* ��������������򷵻� */
		return;
	if (card < 0 || card >= cards_limit) /*�����豸���Ƿ�Ϸ� */
		return;
	request_module("snd-card-%i", card);  /*  */
}

EXPORT_SYMBOL(snd_request_card);

/* ���Լ�������ģ�� */
static void snd_request_other(int minor)
{
	char *str;

	switch (minor) {
	case SNDRV_MINOR_SEQUENCER:	str = "snd-seq";	break;
	case SNDRV_MINOR_TIMER:		str = "snd-timer";	break;
	default:			return;
	}
	request_module(str);
}

#endif	/* modular kernel */

/**
 * snd_lookup_minor_data - get user data of a registered device
 * @minor: the minor number
 * @type: device type (SNDRV_DEVICE_TYPE_XXX)
 *
 * Checks that a minor device with the specified type is registered, and returns
 * its user data pointer.
 */
 /* ��ȡ�û����ݴ�һ��ע����豸��
     minorΪ���豸��
     typeΪ�豸ʱ�䣬�鿴SNDRV_DEVICE_TYPE_XXX*/
void *snd_lookup_minor_data(unsigned int minor, int type)
{
	struct snd_minor *mreg;
	void *private_data;

	if (minor >= ARRAY_SIZE(snd_minors)) /* �����豸���Ƿ�Ϸ� */
		return NULL;
	mutex_lock(&sound_mutex);
	mreg = snd_minors[minor]; /* ��ȡ�Դ��豸��Ϊ�±���������� */
	if (mreg && mreg->type == type)  /* �������ƥ��ƥ����˵��ƥ�� */
		private_data = mreg->private_data;  /* ��ȡ����˽������ */
	else
		private_data = NULL;
	mutex_unlock(&sound_mutex);
	return private_data; /* ����˽������ */
}

EXPORT_SYMBOL(snd_lookup_minor_data);

/* �������豸 */
static int __snd_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode); /*��ȡ���豸�� */
	struct snd_minor *mptr = NULL;
	const struct file_operations *old_fops;
	int err = 0;

	if (minor >= ARRAY_SIZE(snd_minors)) /* �����豸���Ƿ�Ϸ� */
		return -ENODEV;
	mptr = snd_minors[minor]; /* �Դ��豸��Ϊ�±��ȡ��Ӧ��struct snd_minorָ�� */
	if (mptr == NULL) {
#ifdef CONFIG_MODULES
		int dev = SNDRV_MINOR_DEVICE(minor);
		if (dev == SNDRV_MINOR_CONTROL) {
			/* /dev/aloadC? */
			int card = SNDRV_MINOR_CARD(minor);
			if (snd_cards[card] == NULL)
				snd_request_card(card); /* ���Լ���һ��ģ�� */
		} else if (dev == SNDRV_MINOR_GLOBAL) {
			/* /dev/aloadSEQ */
			snd_request_other(minor); /*  ���Լ�������ģ�� */
		}
#ifndef CONFIG_SND_DYNAMIC_MINORS
		/* /dev/snd/{controlC?,seq} */
		mptr = snd_minors[minor];
		if (mptr == NULL)
#endif
#endif
			return -ENODEV;
	}
	old_fops = file->f_op; /* ����ԭ���Ĳ���������ָ�� */
	file->f_op = fops_get(mptr->f_ops);  /* ��ȡ�豸���µĲ�����������������ֵ��file->f_op ������ϵͳ���þͿ��Ե�������豸*/
	if (file->f_op == NULL) {
		file->f_op = old_fops;
		return -ENODEV;
	}
	if (file->f_op->open) 
		err = file->f_op->open(inode, file); /* ��������open���� */
	if (err) {
		fops_put(file->f_op);
		file->f_op = fops_get(old_fops);
	}
	fops_put(old_fops);
	return err;
}


/* BKL pushdown: nasty #ifdef avoidance wrapper */
/* �������豸 */
static int snd_open(struct inode *inode, struct file *file)
{
	int ret;

	lock_kernel();
	ret = __snd_open(inode, file); /* ���øú������豸 */
	unlock_kernel();
	return ret;
}

static const struct file_operations snd_fops =  /* �����Ĳ��������� */
{
	.owner =	THIS_MODULE,
	.open =		snd_open
};

#ifdef CONFIG_SND_DYNAMIC_MINORS
/* Ѱ�ҿ��еĴ��豸�� */
static int snd_find_free_minor(void)
{
	int minor;

	for (minor = 0; minor < ARRAY_SIZE(snd_minors); ++minor) {
		/* skip minors still used statically for autoloading devices */
		if (SNDRV_MINOR_DEVICE(minor) == SNDRV_MINOR_CONTROL ||
		    minor == SNDRV_MINOR_SEQUENCER)
			continue;
		if (!snd_minors[minor])
			return minor;
	}
	return -EBUSY;
}
#else
/* ��ȡ���豸�� */
static int snd_kernel_minor(int type, struct snd_card *card, int dev)
{
	int minor;

	switch (type) {
	case SNDRV_DEVICE_TYPE_SEQUENCER:
	case SNDRV_DEVICE_TYPE_TIMER:
		minor = type;
		break;
	case SNDRV_DEVICE_TYPE_CONTROL:
		if (snd_BUG_ON(!card))
			return -EINVAL;
		minor = SNDRV_MINOR(card->number, type);
		break;
	case SNDRV_DEVICE_TYPE_HWDEP:
	case SNDRV_DEVICE_TYPE_RAWMIDI:
	case SNDRV_DEVICE_TYPE_PCM_PLAYBACK:
	case SNDRV_DEVICE_TYPE_PCM_CAPTURE:
		if (snd_BUG_ON(!card))
			return -EINVAL;
		minor = SNDRV_MINOR(card->number, type + dev);
		break;
	default:
		return -EINVAL;
	}
	if (snd_BUG_ON(minor < 0 || minor >= SNDRV_OS_MINORS))
		return -EINVAL;
	return minor;
}
#endif

/**
 * snd_register_device_for_dev - Register the ALSA device file for the card
 * @type: the device type, SNDRV_DEVICE_TYPE_XXX
 * @card: the card instance
 * @dev: the device index
 * @f_ops: the file operations
 * @private_data: user pointer for f_ops->open()
 * @name: the device file name
 * @device: the &struct device to link this new device to
 *
 * Registers an ALSA device file for the given card.
 * The operators have to be set in reg parameter.
 *
 * Returns zero if successful, or a negative error code on failure.
 */
 /* Ϊ����ע��ALSA�豸(���)�ļ� 
     ����typeΪ�豸���ͣ��鿴��SNDRV_DEVICE_TYPE_XXX
     ����cardΪ�豸Ҫ����������
     ����devΪ�豸����
     ����f_opsΪ�豸�Ĳ���������ָ��
     ����private_dataΪ˽�����飬����f_ops->open()
     ����nameΪ�豸�ļ�������
     ����deviceΪ����card��Ƕ���豸*/
int snd_register_device_for_dev(int type, struct snd_card *card, int dev,
				const struct file_operations *f_ops,
				void *private_data,
				const char *name, struct device *device)
{
	int minor;
	struct snd_minor *preg;

	if (snd_BUG_ON(!name))
		return -EINVAL;
	preg = kmalloc(sizeof *preg, GFP_KERNEL);  /* ����һ��struct snd_minor�ṹ�� */
	if (preg == NULL)
		return -ENOMEM;
	preg->type = type;
	preg->card = card ? card->number : -1;
	preg->device = dev;
	preg->f_ops = f_ops;
	preg->private_data = private_data;
	mutex_lock(&sound_mutex);
#ifdef CONFIG_SND_DYNAMIC_MINORS
	minor = snd_find_free_minor();  /* Ѱ�ҿ��еĴ��豸�� */
#else
	minor = snd_kernel_minor(type, card, dev);/* Ѱ�ҿ��еĴ��豸�� */
#else
	if (minor >= 0 && snd_minors[minor]) /* ˵�����豸���Ѿ���ռ�� */
		minor = -EBUSY;
#endif
	if (minor < 0) {
		mutex_unlock(&sound_mutex);
		kfree(preg);
		return minor;
	}
	snd_minors[minor] = preg; /* �Դ��豸��Ϊ��������������struct snd_minor �������� */
	preg->dev = device_create(sound_class, device, MKDEV(major, minor),
				  private_data, "%s", name); /* �����豸�ļ� */
	if (IS_ERR(preg->dev)) {
		snd_minors[minor] = NULL;
		mutex_unlock(&sound_mutex);
		minor = PTR_ERR(preg->dev);
		kfree(preg);
		return minor;
	}

	mutex_unlock(&sound_mutex);
	return 0;
}

EXPORT_SYMBOL(snd_register_device_for_dev);

/* find the matching minor record
 * return the index of snd_minor, or -1 if not found
 */
 /* Ѱ��ƥ��Ĵ��豸�� */
static int find_snd_minor(int type, struct snd_card *card, int dev)
{
	int cardnum, minor;
	struct snd_minor *mptr;

	cardnum = card ? card->number : -1; /* ��ȡ�������� */
	for (minor = 0; minor < ARRAY_SIZE(snd_minors); ++minor)  /* ��������snd_minors��Ѱ�Ҷ�Ӧ�������ҵ��ͽ�������±귵�أ�
	                                                                                                    �ɴ˿ɼ�������snd_minors���Դ��豸��Ϊ������*/
		if ((mptr = snd_minors[minor]) != NULL &&
		    mptr->type == type &&
		    mptr->card == cardnum &&
		    mptr->device == dev)
			return minor;
	return -1;
}

/**
 * snd_unregister_device - unregister the device on the given card
 * @type: the device type, SNDRV_DEVICE_TYPE_XXX
 * @card: the card instance
 * @dev: the device index
 *
 * Unregisters the device file already registered via
 * snd_register_device().
 *
 * Returns zero if sucecessful, or a negative error code on failure
 */
 /* ע�����������ϵ��豸(���)
     ����typeΪ�豸���ͣ��鿴��SNDRV_DEVICE_TYPE_XXX
     ����cardΪ����
     ����devΪ�豸����*/
int snd_unregister_device(int type, struct snd_card *card, int dev)
{
	int minor;

	mutex_lock(&sound_mutex);
	minor = find_snd_minor(type, card, dev);  /* Ѱ�Ҵ��豸�� */
	if (minor < 0) {
		mutex_unlock(&sound_mutex);
		return -EINVAL;
	}

	device_destroy(sound_class, MKDEV(major, minor));  /* �����豸�ڵ� */

	kfree(snd_minors[minor]);  /* �ͷŶ��ڵ�������ָ��Ľṹ��ռ���ڴ� */
	snd_minors[minor] = NULL;
	mutex_unlock(&sound_mutex);
	return 0;
}

EXPORT_SYMBOL(snd_unregister_device);

/* ��sysfs�ļ�ϵͳ������豸(���)�ļ�
    ����typeΪ�豸����
    ����cardΪ�豸����������
    ����devΪ����
    ����attrΪ�豸����*/
int snd_add_device_sysfs_file(int type, struct snd_card *card, int dev,
			      struct device_attribute *attr)
{
	int minor, ret = -EINVAL;
	struct device *d;

	mutex_lock(&sound_mutex);
	minor = find_snd_minor(type, card, dev); /* Ѱ���豸�Ĵ��豸 */
	if (minor >= 0 && (d = snd_minors[minor]->dev) != NULL)
		ret = device_create_file(d, attr); /* �����豸�����ļ� */
	mutex_unlock(&sound_mutex);
	return ret;

}

EXPORT_SYMBOL(snd_add_device_sysfs_file);

#ifdef CONFIG_PROC_FS
/*
 *  INFO PART
 */

static struct snd_info_entry *snd_minor_info_entry;

/* �ں��������豸(���)����type�����ظ����Ͷ�Ӧ���豸���� */
static const char *snd_device_type_name(int type)
{
	switch (type) {
	case SNDRV_DEVICE_TYPE_CONTROL:
		return "control";
	case SNDRV_DEVICE_TYPE_HWDEP:
		return "hardware dependent";
	case SNDRV_DEVICE_TYPE_RAWMIDI:
		return "raw midi";
	case SNDRV_DEVICE_TYPE_PCM_PLAYBACK:
		return "digital audio playback";
	case SNDRV_DEVICE_TYPE_PCM_CAPTURE:
		return "digital audio capture";
	case SNDRV_DEVICE_TYPE_SEQUENCER:
		return "sequencer";
	case SNDRV_DEVICE_TYPE_TIMER:
		return "timer";
	default:
		return "?";
	}
}

static void snd_minor_info_read(struct snd_info_entry *entry, struct snd_info_buffer *buffer)
{
	int minor;
	struct snd_minor *mptr;

	mutex_lock(&sound_mutex);
	for (minor = 0; minor < SNDRV_OS_MINORS; ++minor) {
		if (!(mptr = snd_minors[minor]))
			continue;
		if (mptr->card >= 0) {
			if (mptr->device >= 0)
				snd_iprintf(buffer, "%3i: [%2i-%2i]: %s\n",
					    minor, mptr->card, mptr->device,
					    snd_device_type_name(mptr->type));
			else
				snd_iprintf(buffer, "%3i: [%2i]   : %s\n",
					    minor, mptr->card,
					    snd_device_type_name(mptr->type));
		} else
			snd_iprintf(buffer, "%3i:        : %s\n", minor,
				    snd_device_type_name(mptr->type));
	}
	mutex_unlock(&sound_mutex);
}

int __init snd_minor_info_init(void)
{
	struct snd_info_entry *entry;

	entry = snd_info_create_module_entry(THIS_MODULE, "devices", NULL);
	if (entry) {
		entry->c.text.read = snd_minor_info_read;
		if (snd_info_register(entry) < 0) {
			snd_info_free_entry(entry);
			entry = NULL;
		}
	}
	snd_minor_info_entry = entry;
	return 0;
}

int __exit snd_minor_info_done(void)
{
	snd_info_free_entry(snd_minor_info_entry);
	return 0;
}
#endif /* CONFIG_PROC_FS */

/*
 *  INIT PART
 */

/* ��ں��� */
static int __init alsa_sound_init(void)
{
	snd_major = major; /* snd_major=16 */
	snd_ecards_limit = cards_limit; /* ��ʼ�������������ƣ�snd_ecards_limit= 1*/
	if (register_chrdev(major, "alsa", &snd_fops)) {  /* ע���ַ��豸 */
		snd_printk(KERN_ERR "unable to register native major device number %d\n", major);
		return -EIO;
	}
	if (snd_info_init() < 0) {  /* ������Ϣ��ʼ������info.c�ж��� */
		unregister_chrdev(major, "alsa");
		return -ENOMEM;
	}
	snd_info_minor_register();
#ifndef MODULE
	printk(KERN_INFO "Advanced Linux Sound Architecture Driver Version " CONFIG_SND_VERSION CONFIG_SND_DATE ".\n");
#endif
	return 0;
}

static void __exit alsa_sound_exit(void)
{
	snd_info_minor_unregister();
	snd_info_done();
	unregister_chrdev(major, "alsa");
}

module_init(alsa_sound_init)
module_exit(alsa_sound_exit)
