/* arch/arm/plat-s5p/dev-mfc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Device definition for MFC device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <mach/map.h>
#include <asm/irq.h>
#include <plat/mfc.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/media.h>
#include <mach/media.h>

static struct s3c_platform_mfc s3c_mfc_pdata = {
	.buf_phy_base[0] = 0,
	.buf_phy_base[1] = 0,
	.buf_phy_size[0] = 0,
	.buf_phy_size[1] = 0,
};

static struct resource s3c_mfc_resources[] = {
	[0] = {
		.start  = S5PV210_PA_MFC,
		.end    = S5PV210_PA_MFC + S5PV210_SZ_MFC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_MFC,
		.end    = IRQ_MFC,
		.flags  = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_mfc = {
	.name           = "s3c-mfc",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s3c_mfc_resources),
	.resource       = s3c_mfc_resources,
	.dev		= {
		.platform_data = &s3c_mfc_pdata,
	},
};

/*
 * MFC hardware has 2 memory interfaces which are modelled as two separate
 * platform devices to let dma-mapping distinguish between them.
 *
 * MFC parent device (s3c_device_mfc) must be registered before memory
 * interface specific devices (s3c_device_mfc_l and s3c_device_mfc_r).
 */

struct platform_device s3c_device_mfc_l = {
	.name		= "s3c-mfc-l",
	.id		= -1,
	.dev		= {
		.parent			= &s3c_device_mfc.dev,
		.dma_mask		= &s3c_device_mfc_l.dev.coherent_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

struct platform_device s3c_device_mfc_r = {
	.name		= "s3c-mfc-r",
	.id		= -1,
	.dev		= {
		.parent			= &s3c_device_mfc.dev,
		.dma_mask		= &s3c_device_mfc_r.dev.coherent_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

void __init s3c_mfc_set_platdata(struct s3c_platform_mfc *pd)
{
	s3c_mfc_pdata.buf_phy_base[0] =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_MFC, 0);
	s3c_mfc_pdata.buf_phy_size[0] =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_MFC, 0);
	s3c_mfc_pdata.buf_phy_base[1] =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_MFC, 1);
	s3c_mfc_pdata.buf_phy_size[1] =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_MFC, 1);
}

