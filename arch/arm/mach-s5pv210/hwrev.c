/*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>

extern unsigned int HWREV;

int read_proc(char *buf,char **start,off_t offset,int count,int *eof,void *data) {
	int len = 0;

	len  += sprintf(buf + len, "0x%X\n", HWREV);

	return len;
}

void create_new_proc_entry() {
	create_proc_read_entry("hwrev", 0, NULL, read_proc, NULL);
}

int hwrev_init (void) {
	int ret;

	create_new_proc_entry();
	return 0;
}

void hwrev_cleanup(void) {
	remove_proc_entry("hwrev", NULL);
}

MODULE_DESCRIPTION("HWREV Export");
MODULE_LICENSE("GPL");

module_init(hwrev_init);
module_exit(hwrev_cleanup);
