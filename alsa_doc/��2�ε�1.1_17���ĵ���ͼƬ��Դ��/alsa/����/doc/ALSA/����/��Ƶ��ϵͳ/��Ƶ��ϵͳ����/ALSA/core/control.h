#ifndef __SOUND_CONTROL_H
#define __SOUND_CONTROL_H

/*
 *  Header file for control interface
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

/*
  ���ƽӿڶ�����࿪�غ͵���������Ӧ����Թ㷺�����ܴ��û��ռ䱻��ȡ��
  control������Ҫ����;��mixer�����е�mixerԪ�ػ���control�ں�APIʵ�֡�

  ALSA��һ������ܺõ�AC97����ģ�飬���ڽ�֧��AC97��оƬ���ԣ�����ʵ�ֱ��ڵ�
  ���ݡ�
  */

#include <sound/asound.h>

#define snd_kcontrol_chip(kcontrol) ((kcontrol)->private_data)

struct snd_kcontrol;
typedef int (snd_kcontrol_info_t) (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_info * uinfo);
typedef int (snd_kcontrol_get_t) (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol);
typedef int (snd_kcontrol_put_t) (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol);
typedef int (snd_kcontrol_tlv_rw_t)(struct snd_kcontrol *kcontrol,
				    int op_flag, /* 0=read,1=write,-1=command */
				    unsigned int size,
				    unsigned int __user *tlv);


struct snd_kcontrol_new {   /* �µ�control�������µ�control������Ҫʵ��info()��get()��put()��3����Ա���� */
	snd_ctl_elem_iface_t iface;	/* interface identifier */ /* �ӿ�ID���鿴�� SNDRV_CTL_ELEM_IFACE_*  (ͨ����SNDRV_CTL_ELEM_IFACE_MIXER�����ڲ�����mixer��
	                                                                                   ȫ�ֿ��ƣ�ʹ��SNDRV_CTL_ELEM_IFACE_CARD) */
	unsigned int device;		/* device/client number */ /* �豸�� */
	unsigned int subdevice;		/* subdevice (substream) number */ /* ���豸(����)�� */
	unsigned char *name;		/* ASCII name of item */ /* ���� ��control�����Ʒǳ���Ҫ����Ϊcontrol�����������������ֵģ�������ͬ���Ƶ�
	                                                                                      control����ʹ��index���֣�name�ı�־������"SOURCE DIRECTION FUNCTION"��SOURCE������control
	                                                                                      ��Դ����"Master" "PCM"  "CD"  "Line" ������Ϊ"Playback"  "Capture"  "Bypass Playback"  "Bypass Capture"�����
	                                                                                      ʡ���˷�����ζ��playback��capture˫�򣬵�3������������"Switch"  "Volume"  "Route"��*/
	unsigned int index;		/* index of item */  /* ���� */
	unsigned int access;		/* access rights */  /* ����Ȩ�� ���鿴��SNDRV_CTL_ELEM_ACCESS_XXX*/
	unsigned int count;		/* count of same elements */  /* ����Ԫ�ص����� */
	snd_kcontrol_info_t *info;   /* ���ڻ�ȡ��control����ϸ��Ϣ���ú����������ڶ������� */
	snd_kcontrol_get_t *get;     /* ���ڵõ�controlĿǰֵ�������û��ռ� */
	snd_kcontrol_put_t *put;     /* ���ڴ��û��ռ�д��ֵ�����ֵ���ı��򷵻�1�����򷵻�0 */
	union {
		snd_kcontrol_tlv_rw_t *c;
		const unsigned int *p;
	} tlv;
	unsigned long private_value; /* ˽��ֵ������ͨ����������ĺ������ݲ��� */
};

struct snd_kcontrol_volatile { /* �ױ� �����������豸*/
	struct snd_ctl_file *owner;	/* locked */
	pid_t owner_pid;
	unsigned int access;	/* access rights */
};

struct snd_kcontrol {  /* �����豸---control */
	struct list_head list;		/* list of controls */ /* �����γ����� */
	struct snd_ctl_elem_id id;  /*ָ�� �������Ƶ�Ԫ��ID */
	unsigned int count;		/* count of same elements */ /* ��ͬ���Ƶ�Ԫ������ */
	snd_kcontrol_info_t *info;
	snd_kcontrol_get_t *get;
	snd_kcontrol_put_t *put;
	union {
		snd_kcontrol_tlv_rw_t *c;
		const unsigned int *p;
	} tlv;
	unsigned long private_value; 
	void *private_data; /* ˽������ */
	void (*private_free)(struct snd_kcontrol *kcontrol); /* �ͷ�˽������ */
	struct snd_kcontrol_volatile vd[0];	/* volatile data */
};

#define snd_kcontrol(n) list_entry(n, struct snd_kcontrol, list)

struct snd_kctl_event { /* control�¼� */
	struct list_head list;	/* list of events */
	struct snd_ctl_elem_id id;
	unsigned int mask;
};

#define snd_kctl_event(n) list_entry(n, struct snd_kctl_event, list)

struct snd_ctl_file {  /* ���������ļ� */
	struct list_head list;		/* list of all control files */ /* �����γɿ����ļ����� */
	struct snd_card *card;  /* ָ��������Ŀ� */
	pid_t pid;  
	int prefer_pcm_subdevice;
	int prefer_rawmidi_subdevice;
	wait_queue_head_t change_sleep;
	spinlock_t read_lock;
	struct fasync_struct *fasync;
	int subscribed;			/* read interface is activated */
	struct list_head events;	/* waiting events for read */
};

#define snd_ctl_file(n) list_entry(n, struct snd_ctl_file, list)

typedef int (*snd_kctl_ioctl_func_t) (struct snd_card * card,
				      struct snd_ctl_file * control,
				      unsigned int cmd, unsigned long arg);

void snd_ctl_notify(struct snd_card * card, unsigned int mask, struct snd_ctl_elem_id * id);

struct snd_kcontrol *snd_ctl_new1(const struct snd_kcontrol_new * kcontrolnew, void * private_data);
void snd_ctl_free_one(struct snd_kcontrol * kcontrol);
int snd_ctl_add(struct snd_card * card, struct snd_kcontrol * kcontrol);
int snd_ctl_remove(struct snd_card * card, struct snd_kcontrol * kcontrol);
int snd_ctl_remove_id(struct snd_card * card, struct snd_ctl_elem_id *id);
int snd_ctl_rename_id(struct snd_card * card, struct snd_ctl_elem_id *src_id, struct snd_ctl_elem_id *dst_id);
struct snd_kcontrol *snd_ctl_find_numid(struct snd_card * card, unsigned int numid);
struct snd_kcontrol *snd_ctl_find_id(struct snd_card * card, struct snd_ctl_elem_id *id);

int snd_ctl_create(struct snd_card *card);

int snd_ctl_register_ioctl(snd_kctl_ioctl_func_t fcn);
int snd_ctl_unregister_ioctl(snd_kctl_ioctl_func_t fcn);
#ifdef CONFIG_COMPAT
int snd_ctl_register_ioctl_compat(snd_kctl_ioctl_func_t fcn);
int snd_ctl_unregister_ioctl_compat(snd_kctl_ioctl_func_t fcn);
#else
#define snd_ctl_register_ioctl_compat(fcn)
#define snd_ctl_unregister_ioctl_compat(fcn)
#endif

static inline unsigned int snd_ctl_get_ioffnum(struct snd_kcontrol *kctl, struct snd_ctl_elem_id *id)
{
	return id->numid - kctl->id.numid;
}

static inline unsigned int snd_ctl_get_ioffidx(struct snd_kcontrol *kctl, struct snd_ctl_elem_id *id)
{
	return id->index - kctl->id.index;
}

static inline unsigned int snd_ctl_get_ioff(struct snd_kcontrol *kctl, struct snd_ctl_elem_id *id)
{
	if (id->numid) {
		return snd_ctl_get_ioffnum(kctl, id);
	} else {
		return snd_ctl_get_ioffidx(kctl, id);
	}
}

static inline struct snd_ctl_elem_id *snd_ctl_build_ioff(struct snd_ctl_elem_id *dst_id,
						    struct snd_kcontrol *src_kctl,
						    unsigned int offset)
{
	*dst_id = src_kctl->id;
	dst_id->index += offset;
	dst_id->numid += offset;
	return dst_id;
}

/*
 * Frequently used control callbacks
 */
int snd_ctl_boolean_mono_info(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_info *uinfo);
int snd_ctl_boolean_stereo_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo);

/*
 * virtual master control
 */
struct snd_kcontrol *snd_ctl_make_virtual_master(char *name,
						 const unsigned int *tlv);
int _snd_ctl_add_slave(struct snd_kcontrol *master, struct snd_kcontrol *slave,
		       unsigned int flags);
/* optional flags for slave */
#define SND_CTL_SLAVE_NEED_UPDATE	(1 << 0)

/**
 * snd_ctl_add_slave - Add a virtual slave control
 * @master: vmaster element
 * @slave: slave element to add
 *
 * Add a virtual slave control to the given master element created via
 * snd_ctl_create_virtual_master() beforehand.
 * Returns zero if successful or a negative error code.
 *
 * All slaves must be the same type (returning the same information
 * via info callback).  The fucntion doesn't check it, so it's your
 * responsibility.
 *
 * Also, some additional limitations:
 * at most two channels,
 * logarithmic volume control (dB level) thus no linear volume,
 * master can only attenuate the volume without gain
 */
static inline int
snd_ctl_add_slave(struct snd_kcontrol *master, struct snd_kcontrol *slave)
{
	return _snd_ctl_add_slave(master, slave, 0);
}

/**
 * snd_ctl_add_slave_uncached - Add a virtual slave control
 * @master: vmaster element
 * @slave: slave element to add
 *
 * Add a virtual slave control to the given master.
 * Unlike snd_ctl_add_slave(), the element added via this function
 * is supposed to have volatile values, and get callback is called
 * at each time quried from the master.
 *
 * When the control peeks the hardware values directly and the value
 * can be changed by other means than the put callback of the element,
 * this function should be used to keep the value always up-to-date.
 */
static inline int
snd_ctl_add_slave_uncached(struct snd_kcontrol *master,
			   struct snd_kcontrol *slave)
{
	return _snd_ctl_add_slave(master, slave, SND_CTL_SLAVE_NEED_UPDATE);
}

#endif	/* __SOUND_CONTROL_H */
