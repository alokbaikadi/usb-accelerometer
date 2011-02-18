/*
 * USB Accelerometer Driver
 *
 * Copyright (C) 2011 Alok Baikadi (baikadi@engineering.uiuc.edu)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb.h>


#define DRIVER_AUTHOR "Alok Baikadi, baikadi@engineering.uiuc.edu"
#define DRIVER_DESC "USB Accelerometer Driver"

#define VENDOR_ID	0x0ffe
#define PRODUCT_ID	0x1008

/* table of devices that work with this driver */
static const struct usb_device_id id_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ },
};
MODULE_DEVICE_TABLE (usb, id_table);

struct usb_accel {
	struct usb_device *	udev;
	unsigned char		x;
	unsigned char		y;
	unsigned char		z;
};

static unsigned char rest_x;
static unsigned char rest_y;
static unsigned char rest_z;

static int read_position(struct device *dev, unsigned char *x, unsigned char *y, unsigned char *z)
{
    struct usb_interface *intf = to_usb_interface(dev);
    struct usb_accel *accel = usb_get_intfdata(intf);
    *x = accel->x;
    *y = accel->y;
    *z = accel->z;
    return 0;
}

static ssize_t show_position(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned char x;
    unsigned char y;
    unsigned char z;

    read_position(dev,&x,&y,&z);
    return sprintf(buf, "(%d,%d,%d)", x, y, z);
}

static ssize_t show_calibrate(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "(%d,%d,%d)", rest_x, rest_y, rest_z);
}

static ssize_t set_recalibrate(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    if (*buf == '1')
        read_position(dev, &rest_x, &rest_y, &rest_z);
    return count;
}

static DEVICE_ATTR(position, S_IRUGO, show_position, NULL);
static DEVICE_ATTR(calibrate, S_IRUGO, show_calibrate, NULL);
static DEVICE_ATTR(recalibrate, S_IWUSR, NULL, set_recalibrate);

static int accel_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	struct usb_accel *dev = NULL;
	int retval = -ENOMEM;

    printk(KERN_INFO "Probing device\n");
    dev_info(&interface->dev, "Found USB Accelerometer\n");

	dev = kzalloc(sizeof(struct usb_accel), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&interface->dev, "Out of memory\n");
		goto error_mem;
	}

	dev->udev = usb_get_dev(udev);

	usb_set_intfdata (interface, dev);

	retval = device_create_file(&interface->dev, &dev_attr_position);
	if (retval)
		goto error;
	retval = device_create_file(&interface->dev, &dev_attr_calibrate);
	if (retval)
		goto error;
	retval = device_create_file(&interface->dev, &dev_attr_recalibrate);
	if (retval)
		goto error;

    read_position(&interface->dev, &rest_x, &rest_y, &rest_z);

	dev_info(&interface->dev, "USB Accelerometer device now attached\n");
	return 0;

error:
	device_remove_file(&interface->dev, &dev_attr_position);
	device_remove_file(&interface->dev, &dev_attr_calibrate);
	device_remove_file(&interface->dev, &dev_attr_recalibrate);
	usb_set_intfdata (interface, NULL);
	usb_put_dev(dev->udev);
	kfree(dev);
error_mem:
	return retval;
}

static void accel_disconnect(struct usb_interface *interface)
{
	struct usb_accel *dev;

	dev = usb_get_intfdata (interface);

	device_remove_file(&interface->dev, &dev_attr_position);
	device_remove_file(&interface->dev, &dev_attr_calibrate);
	device_remove_file(&interface->dev, &dev_attr_recalibrate);

	/* first remove the files, then set the pointer to NULL */
	usb_set_intfdata (interface, NULL);

	usb_put_dev(dev->udev);

	kfree(dev);

	dev_info(&interface->dev, "USB Accelerometer now disconnected\n");
}

static struct usb_driver accel_driver = {
	.name =		"usbaccel",
	.probe =	accel_probe,
	.disconnect =	accel_disconnect,
	.id_table =	id_table,
};

static int __init usb_accel_init(void)
{
	int retval = 0;

	retval = usb_register(&accel_driver);
	if (retval)
		err("usb_register failed. Error number %d", retval);
	return retval;
}

static void __exit usb_accel_exit(void)
{
	usb_deregister(&accel_driver);
}

module_init (usb_accel_init);
module_exit (usb_accel_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
