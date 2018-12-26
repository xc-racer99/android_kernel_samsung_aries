/* linux/arch/arm/mach-s5pv210/dev-aries-phone.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>

#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/gpio-aries.h>

#include "../../../drivers/misc/samsung_modemctl/modem_ctl.h"

/* Modem control */
static struct modemctl_data mdmctl_data = {
	.name = "xmm",
	.gpio_phone_active = GPIO_PHONE_ACTIVE,
	.gpio_pda_active = GPIO_PDA_ACTIVE,
	.gpio_cp_reset = GPIO_CP_RST,
#ifdef CONFIG_SAMSUNG_GALAXYS4G
	.gpio_phone_on = GPIO_PHONE_ON,
	.gpio_int_resout = GPIO_INT_RESOUT,
	.gpio_cp_pwr_rst = GPIO_CP_PWR_RST,
	.num_pdp_contexts = 3,
	.is_ste_modem = true,
#endif
};

static struct resource mdmctl_res[] = {
	[0] = {
		.name = "active",
		.start = IRQ_EINT15,
		.end = IRQ_EINT15,
		.flags = IORESOURCE_IRQ,
	},
	[1] = {
		.name = "onedram",
		.start = IRQ_EINT11,
		.end = IRQ_EINT11,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.name = "onedram",
		.start = (S5PV210_PA_SDRAM + 0x05000000),
		.end = (S5PV210_PA_SDRAM + 0x05000000 + SZ_16M - 1),
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device modemctl = {
	.name = "modemctl",
	.id = -1,
	.num_resources = ARRAY_SIZE(mdmctl_res),
	.resource = mdmctl_res,
	.dev = {
		.platform_data = &mdmctl_data,
	},
};

static void onedram_cfg_gpio(void)
{
	s3c_gpio_cfgpin(GPIO_nINT_ONEDRAM_AP, S3C_GPIO_SFN(GPIO_nINT_ONEDRAM_AP_AF));
	s3c_gpio_setpull(GPIO_nINT_ONEDRAM_AP, S3C_GPIO_PULL_UP);
	irq_set_irq_type(GPIO_nINT_ONEDRAM_AP, IRQ_TYPE_LEVEL_LOW);
}

static void modemctl_cfg_gpio(void)
{
	int err = 0;
	
	unsigned gpio_phone_on = mdmctl_data.gpio_phone_on;
	unsigned gpio_phone_active = mdmctl_data.gpio_phone_active;
	unsigned gpio_cp_rst = mdmctl_data.gpio_cp_reset;
	unsigned gpio_pda_active = mdmctl_data.gpio_pda_active;
#if defined(CONFIG_SAMSUNG_GALAXYS4G)
	unsigned gpio_int_resout = mdmctl_data.gpio_int_resout;
	unsigned gpio_cp_pwr_rst = mdmctl_data.gpio_cp_pwr_rst;
#endif
#if defined(CONFIG_PHONE_ARIES_CDMA) || defined(CONFIG_SAMSUNG_GALAXYS4G)
	err = gpio_request(gpio_phone_on, "PHONE_ON");
	if (err) {
		printk("fail to request gpio %s\n","PHONE_ON");
	} else {
		gpio_direction_output(gpio_phone_on, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_phone_on, S3C_GPIO_PULL_NONE);
	}
#endif
	err = gpio_request(gpio_cp_rst, "CP_RST");
	if (err) {
		printk("fail to request gpio %s\n","CP_RST");
	} else {
		gpio_direction_output(gpio_cp_rst, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_cp_rst, S3C_GPIO_PULL_NONE);
	}
	err = gpio_request(gpio_pda_active, "PDA_ACTIVE");
	if (err) {
		printk("fail to request gpio %s\n","PDA_ACTIVE");
	} else {
		gpio_direction_output(gpio_pda_active, GPIO_LEVEL_HIGH);
		s3c_gpio_setpull(gpio_pda_active, S3C_GPIO_PULL_NONE);
	}

#if defined(CONFIG_SAMSUNG_GALAXYS4G)
	err = gpio_request(gpio_int_resout, "INT_RESOUT");
	if (err) {
		printk("fail to request gpio %s\n","INT_RESOUT");
	} else {
		gpio_direction_output(gpio_int_resout, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_int_resout, S3C_GPIO_PULL_NONE);
	}

	s3c_gpio_cfgpin(gpio_int_resout, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(gpio_int_resout, S3C_GPIO_PULL_NONE);
	irq_set_irq_type(gpio_int_resout, IRQ_TYPE_EDGE_BOTH);
	err = gpio_request(gpio_cp_pwr_rst, "CP_PWR_RST");
	if (err) {
		printk("fail to request gpio %s\n","CP_PWR_RST");
	} else {
		gpio_direction_output(gpio_cp_pwr_rst, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_cp_pwr_rst, S3C_GPIO_PULL_NONE);
	}

	s3c_gpio_cfgpin(gpio_cp_pwr_rst, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(gpio_cp_pwr_rst, S3C_GPIO_PULL_NONE);
	irq_set_irq_type(gpio_cp_pwr_rst, IRQ_TYPE_EDGE_BOTH);
#endif

	s3c_gpio_cfgpin(gpio_phone_active, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(gpio_phone_active, S3C_GPIO_PULL_NONE);
	irq_set_irq_type(gpio_phone_active, IRQ_TYPE_EDGE_BOTH);
}

static int __init aries_init_phone_interface(void)
{
	onedram_cfg_gpio();
	modemctl_cfg_gpio();

	platform_device_register(&modemctl);
	return 0;
}
device_initcall(aries_init_phone_interface);
