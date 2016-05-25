                                                                                             linux��Ƶ��ϵͳ

------------------------------------------------------------------------------------------------------------------------------------------------
/*
 *����֪ʶ��
 *
 *����Ƶ�ʣ�����Ƶ����ÿ��Ĳ����������ҳ�˵��44.1KHz����Ƶ�ʾ���ÿ�����44100�Σ������ϲ���Ƶ��Խ�ߣ�ת������Խ�ߣ�Ŀǰ�����Ĳ���Ƶ����48KHz��
 *
 *�������ȣ��Բ������ݵķ������ȣ�����24bit�������Ⱦ��ǽ���׼��ƽ�źŰ���2��24�η����з�������������Խ�ߣ�����Խ���档
 *
 *��Ƶ�豸Ӳ���ӿڣ�
 *
 *1.PCM�ӿ�
 *   ��򵥵���Ƶ�ӿ���PCM�����������ƣ��ӿڣ��ýӿ���ʱ�����壨BCLK����֡ͬ���źţ�FS�����������ݣ�DR���ͷ������ݣ�DX����ɡ���FS�źŵ�������
 *���ݴ����MSB��ʼ��FSƵ�ʵ��ڲ���Ƶ�ʡ�FS�ź�֮��ʼ�����ֵĴ��䣬��������λ��˳����д��䣬һ��ʱ�����ڴ���һ�������֡�PCM�ӿں�����ʵ�֣�
 *ԭ�����ܹ�֧���κ����ݷ������κβ���Ƶ�ʣ�����Ҫÿ����Ƶͨ�����һ�����������ݶ��С�
 *
 *2.IIS�ӿ�
 *   IIS�ӿ���20����80������ȱ�PHILIPS����������Ƶ��Ʒ������һ����ΪLRCLK��Left/Right CLOCK�����źŻ����о���ת��������·��Ƶ��ɵ�һ�����ݶ��С�
 *��LRCLKΪ��ʱ�����������ݱ����䣻LRCLKΪ��ʱ�����������ݱ����䡣��CPM��ȣ�IIS���ʺ���������ϵͳ����Ȼ��IIS�ı���Ҳ֧�ֶ�ͨ����ʱ�ָ��ã����
 *����֧�ֶ�ͨ����
 *
 *AC'97�ӿ�
 *��PCM��IIS��ͬ��AC'97��ֻ��һ�����ݸ�ʽ��������Ƶ������ڲ��ܹ�����������п��ƹ��ܡ�
 *AC'97����AC-Link���ⲿ�ı���������ӣ�AC-Link�ӿڰ���λʱ�ӣ�BITCLK����ͬ���ź�У��(SYNC)�ʹӱ��뵽���������Ӵ������н��루SDATDIN��SDATAOUT��
 *�����ݶ��С�AC'97����֡��SYNC���忪ʼ������12��20λʱ϶�Լ�1��16λ��tag���Σ�����256���������С����磬ʱ��1���͡�2�����ڱ���Ŀ���������ʱ϶
 *��3���͡�4���ֱ�������������Ƶͨ������tag���α�ʾ����ʱ϶����һ��������Ч���ݣ���֡�ֳ�ʱ϶�Ǵ�������źź���Ƶ���ݽ�ͨ��4���ߴﵽ9����Ƶͨ��
 *��ת����������������Ϊ���ܡ�
 *
 *Ӧ�ã�
 *CD��MD��MP3�����IIS�ӿڣ��ƶ��绰�����PCM�ӿڣ������ֻ���PDA���ʹ�ú�PCһ����AC'97�����ʽ��
 *
 */

/*
 *linux OSS��Ƶ�豸����
 *OSS��׼�����������������Ƶ�豸��mixer������������dsp�������������
 *��������Ӳ����·�У�mixer�������ǽ�����ź���ϻ��ߵ�����һ�𣬶��ڲ�ͬ��������˵��������������ÿ��ܸ�����ͬ��OSS�����У�/dev/mixer�豸�ļ���
 *Ӧ�ó����mixer���в���������ӿڡ�
 *��������·ͨ��������������ɣ������������������������Ի������ı�̰��������������������ļ����Լ������ڲ�ͬ����Դ������л�����Щ����ͨ��
 *�����ǲ������ģ����Ҳ�����¼�����߲���������Ҫռ�ô����ļ������Դ�����ڻ������Ĳ��������ϵ��͵Ķ�/д����ģʽ����˳���open()��close()������ϵ
 *ͳ����֮�⣬�󲿷ֵĲ�������ͨ��ioctl()ϵͳ��������ɣ���/dev/dsp��ͬ��/dev/mixer������Ӧ�ó���ͬʱ���ʣ����һ�����������ֵ�ᱣ�ֵ���Ӧ���豸
 *�ļ����ر�Ϊֹ��
 *
 *dspҲ��Ϊ���������ʵ��¼������������Ӧ���豸�ļ���/dev/dsp��/dev/sound/dsp��OSS���������ṩ��/dev/dsp���������ֲɼ�������¼�����豸�ļ�����
 *�豸д���ݼ���ζ�ż��������ϵ�D/Aת�������в��ţ�������豸����������ζ�ż��������ϵ�A/Dת��������¼�������Ӧ�ó����ȡ�����ٶȹ��������µ���
 *�����Ĳ���Ƶ�ʣ���ô��������ݽ��ᱻ�����������ȡ���ݵ��ٶȹ��죬���¸��������Ĳ���Ƶ�ʣ���ô�����������򽫻�������Щ�������ݵ�Ӧ�ó���ֱ��
 *�µ����ݵ���Ϊֹ��Ӧ�ó���д�����ݵ��ٶ�Ӧ�����ٵ��������Ĳ���Ƶ�ʣ����������������ͣ��ͣ�ٵ���������û�д�����Ļ������ᱻ�ں˵���������
 *������ֱ��Ӳ�������������µ�����Ϊֹ��
 *
 *
 *linux ALSA��Ƶ�豸����
 *ALSA����Ҫ�ص����£�
 *��1��֧�ֶ��������豸
 *��2��ģ�黯���ں���������
 *��3��֧��SMP�Ͷ��߳�
 *��4���ṩӦ�ÿ����������Լ�Ӧ�ó��򿪷�
 *��5��֧��OSS API������OSSӦ�ó���
 *
 *ALSAϵͳ��������������alsa-driver��������alsa-libs�����������alsa-libplugins�����ù����߰�alsa-utils������������ش���С�����alsa-tools��
 *������Ƶ�̼�֧�ְ�alsa-firmware��OSS�ӿڼ���ģ��㹤��alsa-oss��7������Ŀ������ֻ���������Ǳ���ġ�
 *
 *ĿǰALSA�ں��ṩ���û��ռ�Ľӿ��У�
 *
 *��1���豸��Ϣ�ӿڣ�/proc/asound��
 *��2���豸���ƽӿڣ�/dev/snd/controlCX��
 *��3���������豸�ӿڣ�/dev/snd/mixerCXDX��
 *��4��PCM�豸�ӿڣ�/dev/snd/pcmCXDX��
 *��5��ԭʼMIDI(�Ե�)�豸�ӿڣ�/dev/snd/midiCXDX��
 *��6�������ϳ�(synthesizer)�豸�ӿڣ�/dev/snd/seq��
 *��7����ʱ���ӿڣ�/dev/snd/timer��
 *��Щ�ӿڱ��ṩ��alsa-libʹ�ã������Ǹ�Ӧ�ó���ʹ�ã�Ӧ�ó������ʹ��alsa-lib�����߸��߼��Ľӿڱ���jack�ṩ�Ľӿڡ�
 *
 *
 *linux ASoC��Ƶ�豸����
 *ASoC��ALSA��SoC����ķ�չ���ݱ䣬���ı�����Ȼ����ALSA��������ALSA�ܹ������϶�CPU��صĴ����Codec��ص���������˷��룬��ԭ���ǲ��ô�ͳALSA
 *�ܹ�����£�ͬһ�ͺŵ�Codec�����ڲ�ͬ��CPUʱ����Ҫ��ͬ�����������ǲ����ϴ������õ�Ҫ��ġ�
 *
 *ASoC��Ҫ��3������ɣ�
 *��1��Codec��������һ����ֻ��ϵCodec������CPU��ص����Բ��ɴ˲��ֲ���
 *��2��ƽ̨��������һ����ֻ����CPU��������ϵCodec������Ҫ�������������⣺DMA�����SoC�����PCM��IIS��AC'97���ֽӿڿ��ơ�
 *��3������������һ���ֽ�ƽ̨������Codec��������һ�������˰�һ����Ӳ������
 *
 *����3�����У�1��2������������Ȼ��ͨ�õ������ˣ���Codec������Ϊ�Լ�������������CPU����CPU��IIS��PCM����AC'97�ӿڶ�Ӧ��ƽ̨��������Ϊ�Լ���������
 *������ӿ����͵�Codec��ֻ��3�ǲ�ͨ�õģ����ض��ĵ�·���Ͼ����CPU��Codecȷ�������������һ���������������Codec��ƽ̨��������ͷ��ASoC���û��ռ�
 *��̷�����ALSA��ȫһ�¡�
 *
 */

/***********************************************************card���������******************************************************************/

/* ����ÿ�����������봴��һ��cardʵ�����ú������ڴ���card
    ����idxΪ������
    ����idΪ��ʶ���ַ���
    ����moduleһ��ָ��THIS_MODULE
    ����extra_size��Ҫ����Ķ�������ݵĴ�С�������extra_size��С���ڴ潫��Ϊcard->private_data*/
 struct snd_card *snd_card_new(int idx, const char *id,struct module *module, int extra_size)

  /* ע������ */
int snd_card_register(struct snd_card *card)

/* �ͷ�(ע��)���� */
int snd_card_free(struct snd_card *card)

 /* ����һ��ALSA�豸����
     ����typeΪ�豸���ͣ��鿴��SNDRV_DEV_XXX*/
int snd_device_new(struct snd_card *card, snd_device_type_t type,void *device_data, struct snd_device_ops *ops)

  /* 
   �ͷ��������豸
   ����device_dataָ��Ҫ�豸��˽������
   */
int snd_device_free(struct snd_card *card, void *device_data)

 
/************************************************************************************************************************************************/

/****************************************************************PCM �豸**********************************************************************/
 /* ����PCMʵ��
     ����cardָ������
     ����id�Ǳ�ʶ�ַ���
     ����deviceΪPCM�豸����(0��ʾ��1��PCM�豸)
     ����playback_countΪ�����豸��������
     ����capture_countΪ¼���豸��������
     ����ָ�����PCMʵ��*/
int snd_pcm_new(struct snd_card *card, const char *id, int device,int playback_count, int capture_count,struct snd_pcm ** rpcm)

 /* ����PCM�������� 
     ����direction���鿴��SNDRV_PCM_STREAM_XXX*/
void snd_pcm_set_ops(struct snd_pcm *pcm, int direction, struct snd_pcm_ops *ops)

 /* ����DMA������������DMA��������Ԥ���������²ſɵ��øú��� */
int snd_pcm_lib_malloc_pages(struct snd_pcm_substream *substream, size_t size)

 /* �ͷ���snd_pcm_lib_malloc_pages���������һ����DMA������ */
int snd_pcm_lib_free_pages(struct snd_pcm_substream *substream)

 /* ���仺��������򵥵ķ����ǵ��øú���
     type��ȡֵ�ɲ鿴��SNDRV_DMA_TYPE_* */
int snd_pcm_lib_preallocate_pages_for_all(struct snd_pcm *pcm,int type, void *data,size_t size, size_t max)

/************************************************************************************************************************************************/

/**************************************************************���ƽӿ�*********************************************************************/
 /* ����һ��controlʵ��----struct snd_kcontrol�ṹ��
     ����ncontrolΪ��ʼ����¼
     private_dataΪ���õ�˽������ */
struct snd_kcontrol *snd_ctl_new1(const struct snd_kcontrol_new *ncontrol, void *private_data)

 /* Ϊ�������һ������ʵ�� */
int snd_ctl_add(struct snd_card *card, struct snd_kcontrol *kcontrol)

/* ����������жϷ�������е��øú������ı�����һ��control*/
void snd_ctl_notify(struct snd_card *card, unsigned int mask,struct snd_ctl_elem_id *id)

/************************************************************************************************************************************************/


/******************************************************************AC97**************************************************************************/
 /* ����AC97���߼������ */
int snd_ac97_bus(struct snd_card *card, int num, struct snd_ac97_bus_ops *ops,void *private_data, struct snd_ac97_bus **rbus)

 /* ����ģ�崴��һ��AC97 ������� */
int snd_ac97_mixer(struct snd_ac97_bus *bus, struct snd_ac97_template *template, struct snd_ac97 **rac97)

 /* дAC97�Ĵ��� */
void snd_ac97_write(struct snd_ac97 *ac97, unsigned short reg, unsigned short value)

 /* ����AC97�Ĵ��� */
int snd_ac97_update(struct snd_ac97 *ac97, unsigned short reg, unsigned short value)

 /* ���¼Ĵ���ĳ��bitλ----�����汾 */
int snd_ac97_update_bits(struct snd_ac97 *ac97, unsigned short reg, unsigned short mask, unsigned short value)

 /* ��AC97�Ĵ��� */
unsigned short snd_ac97_read(struct snd_ac97 *ac97, unsigned short reg)

 /* ���ò����ʣ�reg������AC97_PMC_MIC_ADC_RATE  ,  AC97_PCM_FRONT_DAC_RATE  ,  AC97_PCM_LR_ADC_RATE , AC97_PCM_SURR_DAC_RATE 
     and   AC97_PCM_LFE_DAC_RATE*/
int snd_ac97_set_rate(struct snd_ac97 *ac97, int reg, unsigned int rate)
/************************************************************************************************************************************************/

/**************************************************************�����������***************************************************************/
/* ����ALSA��Ƶ��ܵ������������:    */

1:struct snd_card *snd_card_new(int idx, const char *id,struct module *module, int extra_size);/* ����һ������ */

2:static struct snd_device_ops ops = {
		.dev_free =     xxx_free,
	};

3:struct snd_kcontrol *snd_ctl_new1(const struct snd_kcontrol_new *ncontrol,void *private_data); /*  ����controlʵ�� */

4:int snd_ctl_add(struct snd_card *card, struct snd_kcontrol *kcontrol);    /* Ϊ������ӿ���ʵ�� */

5:int snd_device_new(struct snd_card *card, SNDRV_DEV_CODEC,void *device_data, struct snd_device_ops *ops);  /* ����һ��ALSA�豸����-----������豸 */

6:int snd_pcm_new(struct snd_card *card, const char *id, int device,int playback_count, int capture_count,struct snd_pcm ** rpcm) /*  ����PCMʵ�� */

8:int snd_pcm_lib_preallocate_pages_for_all(struct snd_pcm *pcm,int type, void *data,size_t size, size_t max)  /* ���仺���� */

9:void snd_pcm_set_ops(struct snd_pcm *pcm, SNDRV_PCM_STREAM_PLAYBACK, struct snd_pcm_ops *ops)  /* ����PCM��������----����  */

10:void snd_pcm_set_ops(struct snd_pcm *pcm, SNDRV_PCM_STREAM_CAPTURE, struct snd_pcm_ops *ops)  /* ����PCM�������� ----¼�� */

11:/*��Ƶ��صĳ�ʼ��:��Ƶ����ء����ŵ���صĳ�ʼ��*/

12:DMA��ص�����

13:int snd_card_register(struct snd_card *card);  /* ע������ */IRQ_AC97
/************************************************************************************************************************************************/

/*************************************************************ASoC��Ƶ����******************************************************************/

/*
  *1:ASoC Codec����:
  */

struct snd_soc_dai { /* ������Ƶ�ӿڣ�������Codec DAI(������Ƶ�ӿ�---Digital Audio Interface)��PCM���� */
	/* DAI description */
	char *name;  /* ���� */
	unsigned int id;  /* ID */
	int ac97_control;

	struct device *dev;

	/* DAI callbacks */
	int (*probe)(struct platform_device *pdev,
		     struct snd_soc_dai *dai);
	void (*remove)(struct platform_device *pdev,
		       struct snd_soc_dai *dai);
	int (*suspend)(struct snd_soc_dai *dai);
	int (*resume)(struct snd_soc_dai *dai);

	/* ops */
	struct snd_soc_dai_ops *ops;  /* DAI�������� */

	/* DAI capabilities */
	struct snd_soc_pcm_stream capture;  /* ¼���� */
	struct snd_soc_pcm_stream playback; /* ������ */

	/* DAI runtime info */
	struct snd_pcm_runtime *runtime;  /* PCM����ʱ */
	struct snd_soc_codec *codec;  /* ������� */
	unsigned int active;
	unsigned char pop_wait:1;
	void *dma_data;

	/* DAI private data */
	void *private_data;

	/* parent codec/platform */
	union {
		struct snd_soc_codec *codec;  /* ������� */
		struct snd_soc_platform *platform;  /* ƽ̨����----CPU */
	};

	struct list_head list; /* �����γ����� */
};

struct snd_soc_codec { /* SoC��Ƶ�������---IO��������̬��Ƶ��Դ�����Լ�ʱ�ӡ�PLL�ȿ��� */
	char *name;  /* ���� */
	struct module *owner; /* THIS_MODULE */
	struct mutex mutex;
	struct device *dev;

	struct list_head list; /* �����γ����� */

	/* callbacks */
	int (*set_bias_level)(struct snd_soc_codec *,
			      enum snd_soc_bias_level level); /* �ص����� */

	/* runtime */
	struct snd_card *card;  /* ���� */
	struct snd_ac97 *ac97;  /* for ad-hoc ac97 devices */ /* AC97�豸 */
	unsigned int active;
	unsigned int pcm_devs;
	void *private_data;

	/* codec IO */
	void *control_data; /* codec control (i2c/3wire) data */
	unsigned int (*read)(struct snd_soc_codec *, unsigned int);
	int (*write)(struct snd_soc_codec *, unsigned int, unsigned int);
	int (*display_register)(struct snd_soc_codec *, char *,
				size_t, unsigned int);
	hw_write_t hw_write;
	hw_read_t hw_read;
	void *reg_cache;
	short reg_cache_size;
	short reg_cache_step;

	/* dapm */
	u32 pop_time;
	struct list_head dapm_widgets;
	struct list_head dapm_paths;
	enum snd_soc_bias_level bias_level;
	enum snd_soc_bias_level suspend_bias_level;
	struct delayed_work delayed_work;

	/* codec DAI's */
	struct snd_soc_dai *dai; /* SoC��ӿ� */
	unsigned int num_dai;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_reg;
	struct dentry *debugfs_pop_time;
#endif
};

struct snd_soc_dai_ops { /* ������Ƶ�ӿ�DAI���������� */
	/*
	 * DAI clocking configuration, all optional.
	 * Called by soc_card drivers, normally in their hw_params.
	 */
	int (*set_sysclk)(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir); /* ����ϵͳʱ�� */
	int (*set_pll)(struct snd_soc_dai *dai,
		int pll_id, unsigned int freq_in, unsigned int freq_out); /* ����PLL */
	int (*set_clkdiv)(struct snd_soc_dai *dai, int div_id, int div);  /* ����ʱ�ӷ�Ƶ */

	/*
	 * DAI format configuration
	 * Called by soc_card drivers, normally in their hw_params.
	 */
	 /* DAI��ʽ���� */
	int (*set_fmt)(struct snd_soc_dai *dai, unsigned int fmt); /* ���ø�ʽ */
	int (*set_tdm_slot)(struct snd_soc_dai *dai,
		unsigned int mask, int slots);
	int (*set_tristate)(struct snd_soc_dai *dai, int tristate);

	/*
	 * DAI digital mute - optional.
	 * Called by soc-core to minimise any pops.
	 */
	int (*digital_mute)(struct snd_soc_dai *dai, int mute); /* ���־��� */

	/*
	 * ALSA PCM audio operations - all optional.
	 * Called by soc-core during audio PCM operations.
	 */
	 /* ALSA PCM��Ƶ���� */
	int (*startup)(struct snd_pcm_substream *,
		struct snd_soc_dai *);
	void (*shutdown)(struct snd_pcm_substream *,
		struct snd_soc_dai *);
	int (*hw_params)(struct snd_pcm_substream *,
		struct snd_pcm_hw_params *, struct snd_soc_dai *);
	int (*hw_free)(struct snd_pcm_substream *,
		struct snd_soc_dai *);
	int (*prepare)(struct snd_pcm_substream *,
		struct snd_soc_dai *);
	int (*trigger)(struct snd_pcm_substream *, int,
		struct snd_soc_dai *);
};

struct snd_soc_ops { /* SoC��������----Codec��Ƶ���� */
	int (*startup)(struct snd_pcm_substream *);
	void (*shutdown)(struct snd_pcm_substream *);
	int (*hw_params)(struct snd_pcm_substream *, struct snd_pcm_hw_params *);
	int (*hw_free)(struct snd_pcm_substream *);
	int (*prepare)(struct snd_pcm_substream *);
	int (*trigger)(struct snd_pcm_substream *, int);
};

/*
  *2:ASoCƽ̨����:
  *
  *��ASoCƽ̨�������֣�ͬ��������Codec�����е�snd_soc_dai��snd_soc_dai_ops��snd_soc_ops�������ṹ���ʵ����������
  *DAI��DAI�Ĳ�����������ͬ���ǣ���ƽ̨�����У�����ֻ����CPU��صĲ��ֶ�������Codec������֮�⣬��ASoC
  *ƽ̨�����У�����ʵ��������DMA����������ͳALSA��and_pcm_ops�ṹ���Ա����trigger()��pointer()�ȣ����ASoCƽ̨
  *����ͨ����DAI��DMA��������ɡ�
  */

/*
  *ASoC������:
  *�ڰ�������ģ���ʼ�������У���ͨ��platform_device_add()ע��һ����Ϊ"soc-audio"��platform�豸��������Ϊsoc-core.c
  *ע����һ����Ϊ"soc-audio"��platform����
  */

struct snd_soc_device { /* SoC�豸 */
	struct device *dev; /* ��Ƕ���豸ģ�͵��豸 */
	struct snd_soc_card *card; /* SoC�� */
	struct snd_soc_codec_device *codec_dev;  /* SoC������豸 */
	void *codec_data;  /* ������豸ʹ�õ����� */
};

struct snd_soc_dai_link  { /* ��ASoC Codec������CPU�˵�ƽ̨�������ݽṹ */
	char *name;			/* Codec name */ /* ������������� */
	char *stream_name;		/* Stream name */ /* �������� */

	/* DAI */
	struct snd_soc_dai *codec_dai; /* SoC��Ľӿ�----��������� */
	struct snd_soc_dai *cpu_dai;     /* SoC��Ľӿ�----CPU��*/

	/* machine stream operations */
	struct snd_soc_ops *ops; /* SoC��������----���������� */

	/* codec/machine specific init - e.g. add machine controls */
	int (*init)(struct snd_soc_codec *codec);  /* ��ʼ�� */

	/* DAI pcm */
	struct snd_pcm *pcm; /* ָ��pcm */
};


/************************************************************************************************************************************************/





























































 
