/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 8/22/2009

 Copyright 2009, All Rights Reserved.

 At the discretion of the user of this library,
 this software may be licensed under the terms of the
 GNU General Public License v3, a BSD-Style license, or the
 original HIDAPI license as outlined in the LICENSE.txt,
 LICENSE-gpl3.txt, LICENSE-bsd.txt, and LICENSE-orig.txt
 files located at the root of the source distribution.
 These files may also be found in the public source
 code repository located at:
        https://github.com/libusb/hidapi .
********************************************************/

/** @file
 * @defgroup API hidapi API
 */

#ifndef HIDAPI_H__
#define HIDAPI_H__

#include <string>
#include <cstring>
#include <wchar.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __MINGW32__
#include <ntdef.h>
#include <winbase.h>

#  ifndef _NTDEF_
typedef LONG NTSTATUS;
#  endif

//#include <hidsdi.h>
#endif

//#ifdef __CYGWIN__
//#include <ntdef.h>
//#define _wcsdup wcsdup
//#endif

/* The maximum number of characters that can be passed into the
   HidD_Get*String() functions without it failing.*/
#define MAX_STRING_WCHARS 0xFFF

/*#define HIDAPI_USE_DDK*/

#ifdef __cplusplus
extern "C" {
#endif
	#include <setupapi.h>
	#include <winioctl.h>
	#ifndef HIDAPI_USE_DDK
		#include <hidsdi.h>
	#endif

	/* Copied from inc/ddk/hidclass.h, part of the Windows DDK. */
	#define HID_OUT_CTL_CODE(id)  \
		CTL_CODE(FILE_DEVICE_KEYBOARD, (id), METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
	#define IOCTL_HID_GET_FEATURE                   HID_OUT_CTL_CODE(100)
	#define IOCTL_HID_GET_INPUT_REPORT              HID_OUT_CTL_CODE(104)

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @brief Static/compile-time major version of the library.

	@ingroup API
*/
#define HID_API_VERSION_MAJOR 0
/** @brief Static/compile-time minor version of the library.

	@ingroup API
*/
#define HID_API_VERSION_MINOR 10
/** @brief Static/compile-time patch version of the library.

	@ingroup API
*/
#define HID_API_VERSION_PATCH 1

/* Helper macros */
#define HID_API_AS_STR_IMPL(x) #x
#define HID_API_AS_STR(x) HID_API_AS_STR_IMPL(x)
#define HID_API_TO_VERSION_STR(v1, v2, v3) HID_API_AS_STR(v1.v2.v3)

/** @brief Static/compile-time string version of the library.

	@ingroup API
*/
#define HID_API_VERSION_STR HID_API_TO_VERSION_STR(HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH)

struct hid_api_version {
	int major;
	int minor;
	int patch;
};

class hid_device {
public:
	HANDLE			device_handle;
	BOOL			blocking;
	USHORT			output_report_length;
	unsigned char	*write_buf;
	size_t			input_report_length;
	USHORT			feature_report_length;
	unsigned char	*feature_buf;
	void			*last_error_str;
	DWORD			last_error_num;
	BOOL			read_pending;
	char			*read_buf;
	OVERLAPPED		ol;
	OVERLAPPED		write_ol;	  

	hid_device() {
		device_handle = INVALID_HANDLE_VALUE;
		blocking = TRUE;
		output_report_length = 0;
		write_buf = NULL;
		input_report_length = 0;
		feature_report_length = 0;
		feature_buf = NULL;
		last_error_str = NULL;
		last_error_num = 0;
		read_pending = FALSE;
		read_buf = NULL;
		memset(&ol, 0, sizeof(ol));
		ol.hEvent = CreateEvent(NULL, FALSE, FALSE /*initial state f=nonsignaled*/, NULL);
		memset(&write_ol, 0, sizeof(write_ol));
		write_ol.hEvent = CreateEvent(NULL, FALSE, FALSE /*inital state f=nonsignaled*/, NULL);	  
	}

	~hid_device() {
		CloseHandle(ol.hEvent);
		CloseHandle(write_ol.hEvent);	   
		CloseHandle(device_handle);
		LocalFree(last_error_str);
		free(write_buf);
		free(feature_buf);
		free(read_buf);
	}

	void register_error(const char *op);
	int  hid_write(const unsigned char *data, size_t length);
	int  hid_read_timeout(unsigned char *data, size_t length, int milliseconds);
	int  hid_read(unsigned char *data, size_t length);
	int  hid_set_nonblocking(int nonblock);
	int  hid_send_feature_report(const unsigned char *data, size_t length);
	int  hid_get_feature_report(unsigned char *data, size_t length);
	int  hid_get_input_report(unsigned char *data, size_t length);
	void hid_close();

	std::string hid_get_manufacturer_string();
	std::string hid_get_product_string();
	std::string hid_get_serial_number_string();

	int  hid_get_indexed_string(int string_index, std::string string, size_t maxlen);

	const char * hid_error();

};

class hid_device_info {
public:
	std::string path;
	unsigned short vendor_id;
	unsigned short product_id;
	unsigned short release_number;

	std::string str_serial_number;
	std::string str_manufacturer_string;
	std::string str_product_string;

	unsigned short usage_page;
	unsigned short usage;

	int interface_number;
	hid_device_info *next;

	hid_device_info() {}
	~hid_device_info() {}
};

hid_device * hid_open_path(std::string path);
hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, std::string serial_number);

hid_device_info  *hid_enumerate(unsigned short vendor_id, unsigned short product_id);

void hid_free_enumeration(hid_device_info *devs);

int  hid_init(void);

int  hid_exit(void);


#endif

