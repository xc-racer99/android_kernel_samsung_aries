/*
 * Copyright (C)  2017, Cswl Coldwind <cswl1337@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

// User Assisted: CMA
// Allows user to allocate and deallocate CMA areas
// On a perfect world, it would be handled by Camera/OMX HAL
// But we dont' live in a perfect world.
// And current mechanism of handling at driver level isn't working
// that great.
// So let's allow user to allocate and deallocate CMA areas
//
// And who knows better when they want their camera or be able to
// play a 720p video better than the user right?
//
// TODO: Implement sysfs interface to allow userspace to signal when to
// use camera, hw decoding or both.
// TODO: Write probably and app or maybe a widget so the user can change it

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/memblock.h>
#include <linux/mm.h>

#include <mach/media.h>
#include <plat/media.h>

static int uacma_ctrl;
bool mfc_cma_allocated = false;
bool fimc_cma_allocated = false;
EXPORT_SYMBOL(mfc_cma_allocated);
EXPORT_SYMBOL(fimc_cma_allocated);

static int uacma_alloc_mfc(void) {
        int ret;

        if (mfc_cma_allocated) {
                printk(KERN_INFO "uacma: MFC already allocated \n");
                return 0;
        }
        printk(KERN_INFO "uacma: preparing to allocate MFC memory \n");
        ret = s5p_alloc_media_memory_bank(S5P_MDEV_MFC, 0);
        if(ret < 0) {
                printk(KERN_ERR "uacma: unable to allocate MFC0 memory \n");
                return -ENOMEM;
        }

        ret = s5p_alloc_media_memory_bank(S5P_MDEV_MFC, 1);
        if(ret < 0) {
                printk(KERN_ERR "uacma: unable to allocate MFC1 memory \n");
                goto err;
        }

        mfc_cma_allocated = true;
        return 0;

err:
        s5p_release_media_memory_bank(S5P_MDEV_MFC, 0);
        return -ENOMEM;
}

static int uacma_alloc_fimc(void) {
        int ret;

        if (fimc_cma_allocated) {
                printk(KERN_INFO "uacma: FIMC already allocated \n");
                return 0;
        }
        printk(KERN_INFO "uacma: preparing to allocate FIMC memory \n");
        ret = s5p_alloc_media_memory_bank(S5P_MDEV_FIMC2, 1);
        if(ret < 0) {
                printk(KERN_ERR "uacma: unable to allocate FIMC2 memory \n");
                return -ENOMEM;
        }
        ret = s5p_alloc_media_memory_bank(S5P_MDEV_FIMC0, 1);
        if(ret < 0) {
                printk(KERN_ERR "uacma: unable to allocate FIMC2 memory \n");
                goto err;
        }
        fimc_cma_allocated = true;
        return 0;

err:
        s5p_release_media_memory_bank(S5P_MDEV_FIMC2, 1);
        return -ENOMEM;
}

static int uacma_release_mfc(void) {
        if (!mfc_cma_allocated) {
                printk(KERN_INFO "uacma: MFC already deallocated \n");
                return 0;
        }
        printk(KERN_INFO "uacma: preparing to release MFC memory \n");
        s5p_release_media_memory_bank(S5P_MDEV_MFC, 0);
        s5p_release_media_memory_bank(S5P_MDEV_MFC, 1);
        mfc_cma_allocated = false;
        return 0;
}

static int uacma_release_fimc(void) {
        if (!fimc_cma_allocated) {
                printk(KERN_INFO "uacma: FIMC already deallocated \n");
                return 0;
        }
        printk(KERN_INFO "uacma: preparing to release FIMC memory \n");
        s5p_release_media_memory_bank(S5P_MDEV_FIMC2, 1);
        s5p_release_media_memory_bank(S5P_MDEV_FIMC0, 1);
        fimc_cma_allocated = false;
        return 0;
}


/* sysfs interface */
static ssize_t enable_show(struct kobject *kobj,
                           struct kobj_attribute *attr, char *buf)
{
        // TODO: show more info
        return sprintf(buf, "%d\n", uacma_ctrl);
}

static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr,
                            const char *buf, size_t count)
{
        int input;
        int ret;
        sscanf(buf, "%du", &input);
        // 0 = disable both,
        // 1 = enable MFC, disable FIMC
        // 2 = enable FIMC,disable MFC
        // 3 = enable both
        printk(KERN_INFO "uacma: userspace said: %d \n", input);
        switch (input) {
        case 0:
                if(mfc_cma_allocated) {
                        ret = uacma_release_mfc();
                        if(ret < 0)
                            goto err;
                }
                if(fimc_cma_allocated) {
                        ret = uacma_release_fimc();
                        if(ret < 0)
                            goto err;
                }
                uacma_ctrl = input;
                break;
        case 1:
                if(fimc_cma_allocated) {
                        ret = uacma_release_fimc();
                        if(ret < 0)
                            goto err;
                }
                if(!mfc_cma_allocated) {
                        ret = uacma_alloc_mfc();
                        if(ret < 0)
                            goto err;
                }
                uacma_ctrl = input;
                break;
        case 2:
                if(mfc_cma_allocated) {
                        ret = uacma_release_mfc();
                        if(ret < 0)
                            goto err;
                }
                if(!fimc_cma_allocated) {
                        ret = uacma_alloc_fimc();
                        if(ret < 0)
                            goto err;
                }
                uacma_ctrl = input;
                break;
        case 3:
                if(!mfc_cma_allocated) {
                        ret = uacma_alloc_mfc();
                        if(ret < 0)
                            goto err;
                }
                if(!fimc_cma_allocated) {
                        ret = uacma_alloc_fimc();
                        if(ret < 0)
                            goto err;
                }
                uacma_ctrl = input;
                break;
        default:
                printk(KERN_ERR "uacma: tried to set to unsupported value %d \n", input);
        }
        return count;
err:
        // We ran into an error, manually check what the sysfs should show
        printk(KERN_ERR "uacma: encountered an error allocating or deallocating memory \n");
        if(!fimc_cma_allocated && !mfc_cma_allocated)
                uacma_ctrl = 0;
        else if(!fimc_cma_allocated && mfc_cma_allocated)
                uacma_ctrl = 1;
        else if(fimc_cma_allocated && !mfc_cma_allocated)
                uacma_ctrl = 2;
        else
                uacma_ctrl = 3;
        return count;
}

static struct kobj_attribute enable_attribute =
        __ATTR(enable, 0666, enable_show, enable_store);

static struct attribute *attrs[] = {
        &enable_attribute.attr,
        NULL,
};

static struct attribute_group attr_group = {
        .attrs = attrs,
};

static struct kobject *enable_kobj;

static int __init uacma_init(void)
{
        int retval;
        printk(KERN_INFO "uacma: booting up \n");
        // Allocate at boot, since it is needed to setup devices
        // TODO: Boot time allocation should never fail
        //      probably should handle such cases and retry again
        retval = s5p_alloc_media_memory_bank(S5P_MDEV_MFC, 0);
        printk(KERN_INFO "uacma: boot time alloc for mfc0 returned: %d \n", retval);
        retval = s5p_alloc_media_memory_bank(S5P_MDEV_MFC, 1);
        printk(KERN_INFO "uacma: boot time alloc for mfc1 returned: %d \n", retval);
        retval = s5p_alloc_media_memory_bank(S5P_MDEV_FIMC2, 1);
        printk(KERN_INFO "uacma: boot time alloc for fimc2 returned: %d \n", retval);
        retval = s5p_alloc_media_memory_bank(S5P_MDEV_FIMC0, 1);
        printk(KERN_INFO "uacma: boot time alloc for fimc0 returned: %d \n", retval);

        mfc_cma_allocated = true;
        fimc_cma_allocated = true;

        // sysfs_interface
        enable_kobj = kobject_create_and_add("uacma", kernel_kobj);
        if (!enable_kobj) {
                return -ENOMEM;
        }
        retval = sysfs_create_group(enable_kobj, &attr_group);
        if (retval)
                kobject_put(enable_kobj);
        return retval;
}

MODULE_AUTHOR("Cswl Coldwind <cswl1337@gmail.com>");
MODULE_DESCRIPTION("User Assisted: CMA for Galaxy S devices.");
MODULE_LICENSE("GPL");

module_init(uacma_init);
