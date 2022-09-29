/***********************************************************************
 HIDAPI - Multi-Platform library for communication with HID devices.

 hid_lin.h
 
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

#ifndef HIDAPI_H__
#define HIDAPI_H__

#include <cstring>
#include <cstdio>
#include <string>
#include <wchar.h>

#include <locale.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>

class hid_device {
public:
	int device_handle;
	int blocking;
	int uses_numbered_reports;
	hid_device() {
		device_handle = -1;
		blocking = 1;
		uses_numbered_reports = 0;
	}
	~hid_device() {}

/** @brief Write an Output report to a HID device.

	The first byte of @p data[] must contain the Report ID. For
	devices which only support a single report, this must be set
	to 0x0. The remaining bytes contain the report data. Since
	the Report ID is mandatory, calls to hid_write() will always
	contain one more byte than the report contains. For example,
	if a hid report is 16 bytes long, 17 bytes must be passed to
	hid_write(), the Report ID (or 0x0, for devices with a
	single report), followed by the report data (16 bytes). In
	this example, the length passed in would be 17.

	hid_write() will send the data on the first OUT endpoint, if
	one exists. If it does not, it will send the data through
	the Control Endpoint (Endpoint 0).

	@ingroup API
	@param data The data to send, including the report number as
		the first byte.
	@param length The length in bytes of the data to send.

	@returns
		This function returns the actual number of bytes written and
		-1 on error.
*/
	int hid_write(const unsigned char *data, size_t length);

/** @brief Read an Input report from a HID device with timeout.

	Input reports are returned
	to the host through the INTERRUPT IN endpoint. The first byte will
	contain the Report number if the device uses numbered reports.

	@ingroup API
	@param data A buffer to put the read data into.
	@param length The number of bytes to read. For devices with multiple
		reports, make sure to read an extra byte for the report number.
	@param milliseconds timeout in milliseconds or -1 for blocking wait.

	@returns
		This function returns the actual number of bytes read and
		-1 on error. If no packet was available to be read within
		the timeout period, this function returns 0.
*/
	int hid_read_timeout(unsigned char *data, size_t length, int milliseconds);

/** @brief Read an Input report from a HID device.

	Input reports are returned
    to the host through the INTERRUPT IN endpoint. The first byte will
	contain the Report number if the device uses numbered reports.

	@ingroup API
	@param device A device handle returned from hid_open().
	@param data A buffer to put the read data into.
	@param length The number of bytes to read. For devices with
		multiple reports, make sure to read an extra byte for
		the report number.

	@returns
		This function returns the actual number of bytes read and
		-1 on error. If no packet was available to be read and
		the handle is in non-blocking mode, this function returns 0.
*/
	int hid_read(unsigned char *data, size_t length);


/** @brief Get a feature report from a HID device.

	Set the first byte of @p data[] to the Report ID of the
	report to be read.  Make sure to allow space for this
	extra byte in @p data[]. Upon return, the first byte will
	still contain the Report ID, and the report data will
	start in data[1].

	@ingroup API
	@param device A device handle returned from hid_open().
	@param data A buffer to put the read data into, including
		the Report ID. Set the first byte of @p data[] to the
		Report ID of the report to be read, or set it to zero
		if your device does not use numbered reports.
	@param length The number of bytes to read, including an
		extra byte for the report ID. The buffer can be longer
		than the actual report.

	@returns
		This function returns the number of bytes read plus
		one for the report ID (which is still in the first
		byte), or -1 on error.
*/
	int hid_get_feature_report(unsigned char *data, size_t length);

/** @brief Set the device handle to be non-blocking.

	In non-blocking mode calls to hid_read() will return
	immediately with a value of 0 if there is no data to be
	read. In blocking mode, hid_read() will wait (block) until
	there is data to read before returning.

	Nonblocking can be turned on and off at any time.

	@ingroup API
	@param device A device handle returned from hid_open().
	@param nonblock enable or not the nonblocking reads
	 - 1 to enable nonblocking
	 - 0 to disable nonblocking.

	@returns
		This function returns 0 on success and -1 on error.
*/
	int hid_set_nonblocking(int nonblock);

/** @brief Send a Feature report to the device.

	Feature reports are sent over the Control endpoint as a
	Set_Report transfer.  The first byte of @p data[] must
	contain the Report ID. For devices which only support a
	single report, this must be set to 0x0. The remaining bytes
	contain the report data. Since the Report ID is mandatory,
	calls to hid_send_feature_report() will always contain one
	more byte than the report contains. For example, if a hid
	report is 16 bytes long, 17 bytes must be passed to
	hid_send_feature_report(): the Report ID (or 0x0, for
	devices which do not use numbered reports), followed by the
	report data (16 bytes). In this example, the length passed
	in would be 17.

	@ingroup API
	@param device A device handle returned from hid_open().
	@param data The data to send, including the report number as
		the first byte.
	@param length The length in bytes of the data to send, including
		the report number.

	@returns
		This function returns the actual number of bytes written and
		-1 on error.
*/
	int hid_send_feature_report(const unsigned char *data, size_t length);

/** @brief Close a HID device.

	@ingroup API
	@param device A device handle returned from hid_open().
*/

	void   hid_close();
	const char*  hid_error();

};

/// hidapi info structure

class hid_device_info {
public:
	std::string path;
	unsigned short vendor_id;
	unsigned short product_id;
	unsigned short release_number;

	std::string str_serial_number;
	std::string str_manufacturer_string;
	std::string str_product_string;

	unsigned short usage_page;  // Usage Page for this Device/Interface (Windows/Mac only).
	unsigned short usage;// Usage for this Device/Interface (Windows/Mac only).*/

/** The USB interface which this logical device
	represents. Valid on both Linux implementations
	in all cases, and valid on the Windows implementation
	only if the device contains more than one interface. */

	int interface_number;

	hid_device_info *next;

	hid_device_info() {}
	~hid_device_info() {}
};


/** @brief Enumerate the HID Devices.

	This function returns a linked list of all the HID devices
	attached to the system which match vendor_id and product_id.

	If @p vendor_id is set to 0 then any vendor matches.
	If @p product_id is set to 0 then any product matches.
	If @p vendor_id and @p product_id are both set to 0, then
		all HID devices will be returned.

	@ingroup API
	@param vendor_id The Vendor ID (VID) of the types of device
		to open.
	@param product_id The Product ID (PID) of the types of device to open.

	@returns
		This function returns a pointer to a linked list of type
		struct #hid_device, containing information about the HID devices
		attached to the system, or NULL in the case of failure. Free
		this linked list by calling hid_free_enumeration().
*/
hid_device_info  *hid_enumerate(unsigned short vendor_id, unsigned short product_id);

/** @brief Free an enumeration Linked List

    This function frees a linked list created by hid_enumerate().

	@ingroup API
    @param devs Pointer to a list of struct_device returned from
		hid_enumerate().
*/
void    hid_free_enumeration(hid_device_info *devs);

/** @brief Initialize the HIDAPI library.

	This function initializes the HIDAPI library. Calling it is not
	strictly necessary, as it will be called automatically by
	hid_enumerate() and any of the hid_open_*() functions if it is
	needed.  This function should be called at the beginning of
	execution however, if there is a chance of HIDAPI handles
	being opened by different threads simultaneously.

	@ingroup API

	@returns
This function returns 0 on success and -1 on error.
*/
int   hid_init(void);

/** @brief Finalize the HIDAPI library.

	This function frees all of the static data associated with
	HIDAPI. It should be called at the end of execution to avoid
	memory leaks.

	@ingroup API

	@returns
		This function returns 0 on success and -1 on error.
*/

/** @brief Open a HID device using a Vendor ID (VID), Product ID
	(PID) and optionally a serial number.

	If @p serial_number is NULL, the first device with the
		specified VID and PID is opened.

	@ingroup API
	@param vendor_id The Vendor ID (VID) of the device to open.
	@param product_id The Product ID (PID) of the device to open.
	@param serial_number The Serial Number of the device to open
		(Optionally NULL).

	@returns
		This function returns a pointer to a #hid_device object on
		success or NULL on failure.
*/
hid_device *  hid_open(unsigned short vendor_id, unsigned short product_id, std::string serial_number = "");

/** @brief Open a HID device by its path name.

	The path name be determined by calling hid_enumerate(), or a
	platform-specific path name can be used (eg: /dev/hidraw0 on
	Linux).

	@ingroup API
    @param path The path name of the device to open

	@returns
		This function returns a pointer to a #hid_device object on
		success or NULL on failure.
*/
	hid_device *  hid_open_path(std::string path);

	int   hid_exit(void);

#endif

