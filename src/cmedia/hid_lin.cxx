/***********************************************************************
 HIDAPI - Multi-Platform library for communication with HID devices.

 hid_lin.cxx

 Alan Ott
 Signal 11 Software
 Copyright 2009, All Rights Reserved.
 
 C++ implementation
 * Copyright 2021
 * David Freese, W1HKJ
 * for use in fldigi

 This software is licensed under the terms of the GNU General Public 
 License v3.

***********************************************************************/

#include <iostream>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

/* Unix */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <poll.h>

/* Linux */
#include <linux/hidraw.h>
#include <linux/version.h>
#include <linux/input.h>
#include <libudev.h>

#include "hidapi.h"

/* Definitions from linux/hidraw.h. Since these are new, some distros
   may not have header files which contain them. */
#ifndef HIDIOCSFEATURE
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#endif
#ifndef HIDIOCGFEATURE
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif


/* uses_numbered_reports() returns 1 if report_descriptor describes a device
   which contains numbered reports. */
static int uses_numbered_reports(__u8 *report_descriptor, __u32 size) {
	unsigned int i = 0;
	int size_code;
	int data_len, key_size;

	while (i < size) {
		int key = report_descriptor[i];

		/* Check for the Report ID key */
		if (key == 0x85/*Report ID*/) {
			/* This device has a Report ID, which means it uses
			   numbered reports. */
			return 1;
		}

		//printf("key: %02hhx\n", key);

		if ((key & 0xf0) == 0xf0) {
			/* This is a Long Item. The next byte contains the
			   length of the data section (value) for this key.
			   See the HID specification, version 1.11, section
			   6.2.2.3, titled "Long Items." */
			if (i+1 < size)
				data_len = report_descriptor[i+1];
			else
				data_len = 0; /* malformed report */
			key_size = 3;
		}
		else {
			/* This is a Short Item. The bottom two bits of the
			   key contain the size code for the data section
			   (value) for this key.  Refer to the HID
			   specification, version 1.11, section 6.2.2.2,
			   titled "Short Items." */
			size_code = key & 0x3;
			switch (size_code) {
			case 0:
			case 1:
			case 2:
				data_len = size_code;
				break;
			case 3:
				data_len = 4;
				break;
			default:
				/* Can't ever happen since size_code is & 0x3 */
				data_len = 0;
				break;
			};
			key_size = 1;
		}

		/* Skip over this key and it's associated data */
		i += data_len + key_size;
	}

	/* Didn't find a Report ID key. Device doesn't use numbered reports. */
	return 0;
}

/*
 * The caller is responsible for free()ing the (newly-allocated) character
 * strings pointed to by serial_number_utf8 and product_name_utf8 after use.
 */
static int
parse_uevent_info(const char *uevent, int *bus_type,
	unsigned short *vendor_id, unsigned short *product_id,
	char **serial_number_utf8, char **product_name_utf8)
{
	char *tmp = strdup(uevent);
	char *saveptr = NULL;
	char *line;
	char *key;
	char *value;

	int found_id = 0;
	int found_serial = 0;
	int found_name = 0;

	line = strtok_r(tmp, "\n", &saveptr);
	while (line != NULL) {
		/* line: "KEY=value" */
		key = line;
		value = strchr(line, '=');
		if (!value) {
			goto next_line;
		}
		*value = '\0';
		value++;

		if (strcmp(key, "HID_ID") == 0) {
			/**
			 *        type vendor   product
			 * HID_ID=0003:000005AC:00008242
			 **/
			int ret = sscanf(value, "%x:%hx:%hx", bus_type, vendor_id, product_id);
			if (ret == 3) {
				found_id = 1;
			}
		} else if (strcmp(key, "HID_NAME") == 0) {
			/* The caller has to free the product name */
			*product_name_utf8 = strdup(value);
			found_name = 1;
		} else if (strcmp(key, "HID_UNIQ") == 0) {
			/* The caller has to free the serial number */
			*serial_number_utf8 = strdup(value);
			found_serial = 1;
		}

next_line:
		line = strtok_r(NULL, "\n", &saveptr);
	}

	free(tmp);
	return (found_id && found_name && found_serial);
}

int  hid_init(void)
{
	const char *locale;

	/* Set the locale if it's not set. */
	locale = setlocale(LC_CTYPE, NULL);
	if (!locale)
		setlocale(LC_CTYPE, "");

	return 0;
}

int  hid_exit(void)
{
	/* Nothing to do for this in the Linux/hidraw implementation. */
	return 0;
}


hid_device_info   *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;

	hid_device_info *root = NULL; /* return object */
	hid_device_info *cur_dev = NULL;
	hid_device_info *prev_dev = NULL; /* previous device */

	hid_init();

	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		return NULL;
	}

	/* Create a list of the devices in the 'hidraw' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	/* For each item, see if it matches the vid/pid, and if so
	   create a udev_device record for it */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *sysfs_path;
		const char *dev_path;
		const char *str;
		struct udev_device *raw_dev; /* The device's hidraw udev node. */
		struct udev_device *hid_dev; /* The device's HID udev node. */
		struct udev_device *usb_dev; /* The device's USB udev node. */
		struct udev_device *intf_dev; /* The device's interface (in the USB sense). */
		unsigned short dev_vid;
		unsigned short dev_pid;
		char *serial_number_utf8 = NULL;
		char *product_name_utf8 = NULL;
		int bus_type;
		int result;

		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		sysfs_path = udev_list_entry_get_name(dev_list_entry);
		raw_dev = udev_device_new_from_syspath(udev, sysfs_path);
		dev_path = udev_device_get_devnode(raw_dev);

		hid_dev = udev_device_get_parent_with_subsystem_devtype(
			raw_dev,
			"hid",
			NULL);

		if (!hid_dev) {
			/* Unable to find parent hid device. */
			goto next;
		}

		result = parse_uevent_info(
			udev_device_get_sysattr_value(hid_dev, "uevent"),
			&bus_type,
			&dev_vid,
			&dev_pid,
			&serial_number_utf8,
			&product_name_utf8);

		if (!result) {
			/* parse_uevent_info() failed for at least one field. */
			goto next;
		}

		if (bus_type != BUS_USB && bus_type != BUS_BLUETOOTH) {
			/* We only know how to handle USB and BT devices. */
			goto next;
		}

		/* Check the VID/PID against the arguments */
		if ((vendor_id == 0x0 || vendor_id == dev_vid) &&
		    (product_id == 0x0 || product_id == dev_pid)) {

			hid_device_info *tmp = new hid_device_info;

			if (cur_dev) {
				cur_dev->next = tmp;
			}
			else {
				root = tmp;
			}
			prev_dev = cur_dev;
			cur_dev = tmp;

			/* Fill out the record */
			cur_dev->next = NULL;
			cur_dev->path.assign(dev_path ? dev_path : "");

			/* VID/PID */
			cur_dev->vendor_id = dev_vid;
			cur_dev->product_id = dev_pid;

			/* Serial Number */
			cur_dev->str_serial_number.assign(serial_number_utf8);

			/* Release Number */
			cur_dev->release_number = 0x0;

			/* Interface Number */
			cur_dev->interface_number = -1;

			switch (bus_type) {
				case BUS_USB:
					/* The device pointed to by raw_dev contains information about
					   the hidraw device. In order to get information about the
					   USB device, get the parent device with the
					   subsystem/devtype pair of "usb"/"usb_device". This will
					   be several levels up the tree, but the function will find
					   it. */
					usb_dev = udev_device_get_parent_with_subsystem_devtype(
							raw_dev,
							"usb",
							"usb_device");

					if (!usb_dev) {
						delete cur_dev;

						/* Take it off the device list. */
						if (prev_dev) {
							prev_dev->next = NULL;
							cur_dev = prev_dev;
						}
						else {
							cur_dev = root = NULL;
						}

						goto next;
					}

					/* Manufacturer and Product strings */
					str = udev_device_get_sysattr_value(usb_dev, "manufacturer");
					if (str) cur_dev->str_manufacturer_string.assign(str);

					str = udev_device_get_sysattr_value(usb_dev, "product");
					if (str) cur_dev->str_product_string.assign(str);

					/* Release Number */
					str = udev_device_get_sysattr_value(usb_dev, "bcdDevice");
					if (str) cur_dev->release_number = strtol(str, NULL, 16);
					else cur_dev = 0x0;

					/* Get a handle to the interface's udev node. */
					intf_dev = udev_device_get_parent_with_subsystem_devtype(
							raw_dev,
							"usb",
							"usb_interface");
					if (intf_dev) {
						str = udev_device_get_sysattr_value(intf_dev, "bInterfaceNumber");
						cur_dev->interface_number = (str)? strtol(str, NULL, 16): -1;
					}

					break;

				case BUS_BLUETOOTH:
					/* Manufacturer and Product strings */
					cur_dev->str_product_string.assign(product_name_utf8);

					break;

				default:
					/* Unknown device type - this should never happen, as we
					 * check for USB and Bluetooth devices above */
					break;
			}
		}

	next:
		free(serial_number_utf8);
		free(product_name_utf8);
		udev_device_unref(raw_dev);
		/* hid_dev, usb_dev and intf_dev don't need to be (and can't be)
		   unref()d.  It will cause a double-free() error.  I'm not
		   sure why.  */
	}
	/* Free the enumerator and udev objects. */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return root;
}

void   hid_free_enumeration(hid_device_info *devs)
{
	hid_device_info *d = devs;
	while (d) {
		hid_device_info *next = d->next;
		delete d;
		d = next;
	}
}

hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, std::string serial_number)
{
	hid_device_info *devs, *cur_dev;
	hid_device *device = NULL;
	std::string path_to_open;

	devs = hid_enumerate(vendor_id, product_id);
	cur_dev = devs;
	while (cur_dev) {
		if (cur_dev->vendor_id == vendor_id &&
		    cur_dev->product_id == product_id) {
			if (!serial_number.empty() &&
				(cur_dev->str_serial_number == serial_number) ) {
				path_to_open = cur_dev->path;
				break;
			}
			else {
				path_to_open = cur_dev->path;
				break;
			}
		}
		cur_dev = cur_dev->next;
	}

	if (!path_to_open.empty()) {
		/* Open the device */
		device = hid_open_path(path_to_open);
	}

	hid_free_enumeration(devs);

	return device;
}

hid_device *  hid_open_path(std::string path)
{
	hid_device *dev = new hid_device;

	hid_init();

	/* OPEN HERE */
	dev->device_handle = open(path.c_str(), O_RDWR);

	/* If we have a good handle, return it. */
	if (dev->device_handle > 0) {

		/* Get the report descriptor */
		int res, desc_size = 0;
		struct hidraw_report_descriptor rpt_desc;

		memset(&rpt_desc, 0x0, sizeof(rpt_desc));

		/* Get Report Descriptor Size */
		res = ioctl(dev->device_handle, HIDIOCGRDESCSIZE, &desc_size);
		if (res < 0)
			perror("HIDIOCGRDESCSIZE");


		/* Get Report Descriptor */
		rpt_desc.size = desc_size;
		res = ioctl(dev->device_handle, HIDIOCGRDESC, &rpt_desc);
		if (res < 0) {
			perror("HIDIOCGRDESC");
		} else {
			/* Determine if this device uses numbered reports. */
			dev->uses_numbered_reports =
				uses_numbered_reports(rpt_desc.value,
				                      rpt_desc.size);
		}

		return dev;
	}
	else {
		/* Unable to open any devices. */
		delete dev;
		return (hid_device *)0;
	}
}


int hid_device::hid_write(const unsigned char *data, size_t length)
{
	int bytes_written;
	bytes_written = write(device_handle, data, length);
	return bytes_written;
}


int  hid_device::hid_read_timeout(unsigned char *data, size_t length, int milliseconds)
{
	int bytes_read;

	if (milliseconds >= 0) {
		/* Milliseconds is either 0 (non-blocking) or > 0 (contains
		   a valid timeout). In both cases we want to call poll()
		   and wait for data to arrive.  Don't rely on non-blocking
		   operation (O_NONBLOCK) since some kernels don't seem to
		   properly report device disconnection through read() when
		   in non-blocking mode.  */
		int ret;
		struct pollfd fds;

		fds.fd = device_handle;
		fds.events = POLLIN;
		fds.revents = 0;
		ret = poll(&fds, 1, milliseconds);
		if (ret == -1 || ret == 0) {
			/* Error or timeout */
			return ret;
		}
		else {
			/* Check for errors on the file descriptor. This will
			   indicate a device disconnection. */
			if (fds.revents & (POLLERR | POLLHUP | POLLNVAL))
				return -1;
		}
	}

	bytes_read = read(device_handle, data, length);
	if (bytes_read < 0 && (errno == EAGAIN || errno == EINPROGRESS))
		bytes_read = 0;

	return bytes_read;
}

int  hid_device::hid_read(unsigned char *data, size_t length)
{
	return hid_read_timeout(data, length, (blocking)? -1: 0);
}

int  hid_device::hid_set_nonblocking(int nonblock)
{
	/* Do all non-blocking in userspace using poll(), since it looks
	   like there's a bug in the kernel in some versions where
	   read() will not return -1 on disconnection of the USB device */
	blocking = !nonblock;
	return 0; /* Success */
}


int  hid_device::hid_send_feature_report(const unsigned char *data, size_t length)
{
	int res;
	res = ioctl(device_handle, HIDIOCSFEATURE(length), data);
	if (res < 0)
		perror("ioctl (SFEATURE)");
	return res;
}

int  hid_device::hid_get_feature_report(unsigned char *data, size_t length)
{
	int res;

	res = ioctl(device_handle, HIDIOCGFEATURE(length), data);
	if (res < 0)
		perror("ioctl (GFEATURE)");

	return res;
}


void  hid_device::hid_close()
{
	close(device_handle);
}


const char *hid_device::hid_error()
{
	return "HID error string not implemented";
}

