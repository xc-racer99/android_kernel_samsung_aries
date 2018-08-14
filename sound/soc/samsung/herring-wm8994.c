/*
 * crespo_wm8994.c
 *
 * Copyright (C) 2010, Samsung Elect. Ltd. -
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <mach/regs-clock.h>
#include <plat/regs-iis.h>
#include "../codecs/wm8994_samsung.h"

#include "i2s.h"

#include <linux/io.h>

/* #define CONFIG_SND_DEBUG */
#ifdef CONFIG_SND_DEBUG
#define debug_msg(x...) printk(x)
#else
#define debug_msg(x...)
#endif

int smdkc110_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret;
	u32 ap_codec_clk;

	debug_msg("%s\n", __func__);

	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);

	if (ret < 0) {
		printk(KERN_ERR "smdkc110_wm8994_hw_params :\
				 Codec DAI configuration error!\n");
		return ret;
	}

	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);

	if (ret < 0) {
		printk(KERN_ERR
			"smdkc110_wm8994_hw_params :\
				AP DAI configuration error!\n");
		return ret;
	}

	switch (params_rate(params)) {

	case 8000:
		ap_codec_clk = 4096000;
		break;
	case 11025:
		ap_codec_clk = 2822400;
		break;
	case 12000:
		ap_codec_clk = 6144000;
		break;
	case 16000:
		ap_codec_clk = 4096000;
		break;
	case 22050:
		ap_codec_clk = 6144000;
		break;
	case 24000:
		ap_codec_clk = 6144000;
		break;
	case 32000:
		ap_codec_clk = 8192000;
		break;
	case 44100:
		ap_codec_clk = 11289600;
		break;
	case 48000:
		ap_codec_clk = 12288000;
		break;
	default:
		ap_codec_clk = 11289600;
		break;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL,
					ap_codec_clk, 0);
	if (ret < 0)
		return ret;

	return 0;
}

/* machine stream operations */
static struct snd_soc_ops smdkc110_ops = {
	.hw_params = smdkc110_hw_params,
};

/* digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link smdkc1xx_dai = {
	.name = "herring",
	.stream_name = "WM8994 HiFi Playback",
	.cpu_dai_name = "samsung-i2s.0",
	.codec_dai_name = "WM8994 PAIFRX",
	.platform_name = "samsung-audio",
	.codec_name = "wm8994-samsung-codec.4-001a",
	.ops = &smdkc110_ops,
};

static struct snd_soc_card smdkc100 = {
	.name = "smdkc110",
	.dai_link = &smdkc1xx_dai,
	.num_links = 1,
};

static struct platform_device *smdkc1xx_snd_device;
static int __init smdkc110_audio_init(void)
{
	int ret;

	debug_msg("%s\n", __func__);

	smdkc1xx_snd_device = platform_device_alloc("soc-audio", 0);
	if (!smdkc1xx_snd_device)
		return -ENOMEM;

	platform_set_drvdata(smdkc1xx_snd_device, &smdkc100);
	ret = platform_device_add(smdkc1xx_snd_device);

	if (ret)
		platform_device_put(smdkc1xx_snd_device);

	return ret;
}

static void __exit smdkc110_audio_exit(void)
{
	debug_msg("%s\n", __func__);

	platform_device_unregister(smdkc1xx_snd_device);
}

module_init(smdkc110_audio_init);
module_exit(smdkc110_audio_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC SMDKC110 WM8994");
MODULE_LICENSE("GPL");
