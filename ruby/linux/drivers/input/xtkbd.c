/*
 * $Id$
 *
 *  Copyright (c) 1999-2000 Vojtech Pavlik
 *
 *  Sponsored by SuSE
 */

/*
 * XT keyboard driver for Linux
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Ucitelska 1576, Prague 8, 182 00 Czech Republic
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>

MODULE_AUTHOR("Vojtech Pavlik <vojtech@ucw.cz>");

#define XTKBD_EMUL0	0xe0
#define XTKBD_EMUL1	0xe1
#define XTKBD_KEY	0x7f
#define XTKBD_RELEASE	0x80

static unsigned char xtkbd_keycode[256] = {	 
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	 80, 81, 82, 83,  0,  0,  0, 87, 88,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0, 87, 88,  0,  0,  0,  0,110,111,103,108,105,
	106
};

static char *xtkbd_name = "XT Keyboard";

struct xtkbd {
	unsigned char keycode[256];
	struct input_dev dev;
	struct serio *serio;
};

void xtkbd_interrupt(struct serio *serio, unsigned char data, unsigned int flags)
{
	struct xtkbd *xtkbd = serio->private;

	switch (data) {
		case XTKBD_EMUL0:
		case XTKBD_EMUL1:
			return;
		default:

			if (xtkbd->keycode[data & XTKBD_KEY]) {
				input_report_key(&xtkbd->dev, xtkbd->keycode[data & XTKBD_KEY], !(data & XTKBD_RELEASE));
			} else {
				printk(KERN_WARNING "xtkbd.c: Unknown key (scancode %#x) %s.\n",
					data & XTKBD_KEY, data & XTKBD_RELEASE ? "released" : "pressed");
			}
		}
}

void xtkbd_connect(struct serio *serio, struct serio_dev *dev)
{
	struct xtkbd *xtkbd;
	int i;

	if ((serio->type & SERIO_TYPE) != SERIO_XT)
		return;

	if (!(xtkbd = kmalloc(sizeof(struct xtkbd), GFP_KERNEL)))
		return;

	memset(xtkbd, 0, sizeof(struct xtkbd));
	
	xtkbd->dev.evbit[0] = BIT(EV_KEY) | BIT(EV_REP);

	xtkbd->serio = serio;

	xtkbd->dev.keycode = xtkbd->keycode;
	xtkbd->dev.private = xtkbd;

	serio->private = xtkbd;

	if (serio_open(serio, dev)) {
		kfree(xtkbd);
		return;
	}

	memcpy(xtkbd->keycode, xtkbd_keycode, sizeof(xtkbd->keycode));
	for (i = 0; i < 255; i++)
		set_bit(xtkbd->keycode[i], xtkbd->dev.keybit);
	clear_bit(0, xtkbd->dev.keybit);

	xtkbd->dev.name = xtkbd_name;
	xtkbd->dev.idbus = BUS_XTKBD;
	xtkbd->dev.idvendor = 0x0001;
	xtkbd->dev.idproduct = 0x0001;
	xtkbd->dev.idversion = 0x0100;

	input_register_device(&xtkbd->dev);

	printk(KERN_INFO "input%d: %s on serio%d\n", xtkbd->dev.number, xtkbd_name, serio->number);
}

void xtkbd_disconnect(struct serio *serio)
{
	struct xtkbd *xtkbd = serio->private;
	input_unregister_device(&xtkbd->dev);
	serio_close(serio);
	kfree(xtkbd);
}

struct serio_dev xtkbd_dev = {
	interrupt:	xtkbd_interrupt,
	connect:	xtkbd_connect,
	disconnect:	xtkbd_disconnect
};

int __init xtkbd_init(void)
{
	serio_register_device(&xtkbd_dev);
	return 0;
}

void __exit xtkbd_exit(void)
{
	serio_unregister_device(&xtkbd_dev);
}

module_init(xtkbd_init);
module_exit(xtkbd_exit);
