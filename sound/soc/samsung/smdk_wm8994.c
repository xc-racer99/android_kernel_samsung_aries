/*
 *  smdk_wm8994.c
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include "../codecs/wm8994.h"

 /*
  * Default CFG switch settings to use this driver:
  *	SMDKV310: CFG5-1000, CFG7-111111
  */

 /*
  * Configure audio route as :-
  * $ amixer sset 'DAC1' on,on
  * $ amixer sset 'Right Headphone Mux' 'DAC'
  * $ amixer sset 'Left Headphone Mux' 'DAC'
  * $ amixer sset 'DAC1R Mixer AIF1.1' on
  * $ amixer sset 'DAC1L Mixer AIF1.1' on
  * $ amixer sset 'IN2L' on
  * $ amixer sset 'IN2L PGA IN2LN' on
  * $ amixer sset 'MIXINL IN2L' on
  * $ amixer sset 'AIF1ADC1L Mixer ADC/DMIC' on
  * $ amixer sset 'IN2R' on
  * $ amixer sset 'IN2R PGA IN2RN' on
  * $ amixer sset 'MIXINR IN2R' on
  * $ amixer sset 'AIF1ADC1R Mixer ADC/DMIC' on
  */

/* SMDK has a 16.934MHZ crystal attached to WM8994 */
#define SMDK_WM8994_FREQ 24000000

static int smdk_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned int pll_out;
	int ret;

	/* AIF1CLK should be >=3MHz for optimal performance */
	if (params_rate(params) == 8000 || params_rate(params) == 11025)
		pll_out = params_rate(params) * 512;
	else
		pll_out = params_rate(params) * 256;

	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1, WM8994_FLL_SRC_MCLK1,
					SMDK_WM8994_FREQ, pll_out);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1,
					pll_out, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * SMDK WM8994 DAI operations.
 */
static struct snd_soc_ops smdk_ops = {
	.hw_params = smdk_hw_params,
};

static int aries_modem_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
#if 0
	struct snd_soc_card *card = rtd->card;
	struct aries_wm8994_data *priv = snd_soc_card_get_drvdata(card);
	int mclk = WM8994_FLL_SRC_MCLK1;
#else
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int mclk = WM8994_FLL_SRC_MCLK2;
#endif
	unsigned int pll_out;
	int ret;

	if (params_rate(params) != 8000)
		return -EINVAL;

	pll_out = params_rate(params) * 256;

#if 0
	if (priv->modem_master)
		mclk = WM8994_FLL_SRC_MCLK2;
#else

	/* set the cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_DSP_A |
			SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A |
			SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;
#endif

	/* set the codec FLL */
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL2, mclk,
				  SMDK_WM8994_FREQ, pll_out);
	if (ret < 0)
		return ret;

	/* set the codec system clock */
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL2,
				     pll_out, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops aries_modem_ops = {
	.hw_params = aries_modem_hw_params,
};

static int smdk_wm8994_init_paiftx(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	/* HeadPhone */
	snd_soc_dapm_enable_pin(dapm, "HPOUT1R");
	snd_soc_dapm_enable_pin(dapm, "HPOUT1L");

	/* MicIn */
	snd_soc_dapm_enable_pin(dapm, "IN1LN");
	snd_soc_dapm_enable_pin(dapm, "IN1RN");

	/* LineIn */
	snd_soc_dapm_enable_pin(dapm, "IN2LN");
	snd_soc_dapm_enable_pin(dapm, "IN2RN");

	/* Speaker */
	snd_soc_dapm_enable_pin(dapm, "SPKOUTLN");
	snd_soc_dapm_enable_pin(dapm, "SPKOUTLP");

	/* Earpiece */
	snd_soc_dapm_enable_pin(dapm, "HPOUT2P");
	snd_soc_dapm_enable_pin(dapm, "HPOUT2N");

#if 1
	/* Other pins NC */
	snd_soc_dapm_enable_pin(dapm, "SPKOUTRP");
	snd_soc_dapm_enable_pin(dapm, "SPKOUTRN");
	snd_soc_dapm_enable_pin(dapm, "LINEOUT1N");
	snd_soc_dapm_enable_pin(dapm, "LINEOUT1P");
	snd_soc_dapm_enable_pin(dapm, "LINEOUT2N");
	snd_soc_dapm_enable_pin(dapm, "LINEOUT2P");
	snd_soc_dapm_enable_pin(dapm, "IN1LP");
	snd_soc_dapm_enable_pin(dapm, "IN2LP:VXRN");
	snd_soc_dapm_enable_pin(dapm, "IN1RP");
	snd_soc_dapm_enable_pin(dapm, "IN2RP:VXRP");
#endif

	snd_soc_dapm_sync(dapm);

	return 0;
}

static struct snd_soc_dai_driver aries_ext_dai[] = {
	{
		.name = "Voice call",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	},
	{
		.name = "Bluetooth",
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000),
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000),
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	},
};

static struct snd_soc_dai_link smdk_dai[] = {
	{ /* Primary DAI i/f */
		.name = "WM8994 AIF1",
		.stream_name = "Pri_Dai",
		.cpu_dai_name = "samsung-i2s.0",
		.codec_dai_name = "wm8994-aif1",
		.platform_name = "samsung-audio",
		.codec_name = "wm8994-codec",
		.init = smdk_wm8994_init_paiftx,
		.ops = &smdk_ops,
	},
#if 1
	{
		.name = "WM8994 AIF2",
		.stream_name = "Voice call",
		.cpu_dai_name = "Voice call",
		.codec_dai_name = "wm8994-aif2",
		.codec_name = "wm8994-codec",
//		.dai_fmt = SND_SOC_DAIFMT_LEFT_J | SND_SOC_DAIFMT_IB_IF |
//			SND_SOC_DAIFMT_CBM_CFM,
		.ops = &aries_modem_ops,
		.ignore_suspend = 1,
	},
	{
		.name = "WM8994 AIF3",
		.stream_name = "Bluetooth",
		.cpu_dai_name = "Bluetooth",
		.codec_dai_name = "wm8994-aif3",
		.codec_name = "wm8994-codec",
//		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
//			SND_SOC_DAIFMT_CBM_CFM,
		.ignore_suspend = 1,
	},
#endif
};

static struct snd_soc_card smdk = {
	.name = "SMDK-I2S",
	.dai_link = smdk_dai,
	.num_links = ARRAY_SIZE(smdk_dai),
};

static struct platform_device *smdk_snd_device;

static int __init smdk_audio_init(void)
{
	int ret;

	smdk_snd_device = platform_device_alloc("soc-audio", -1);
	if (!smdk_snd_device)
		return -ENOMEM;

	ret = snd_soc_register_dais(&smdk_snd_device->dev, aries_ext_dai, ARRAY_SIZE(aries_ext_dai));
	if (ret) {
		pr_err("smdkwm8994: Received error %d", ret);
		return ret;
	}

	platform_set_drvdata(smdk_snd_device, &smdk);

	ret = platform_device_add(smdk_snd_device);
	if (ret)
		platform_device_put(smdk_snd_device);

	return ret;
}
module_init(smdk_audio_init);

static void __exit smdk_audio_exit(void)
{
	platform_device_unregister(smdk_snd_device);
}
module_exit(smdk_audio_exit);

MODULE_DESCRIPTION("ALSA SoC SMDK WM8994");
MODULE_LICENSE("GPL");
