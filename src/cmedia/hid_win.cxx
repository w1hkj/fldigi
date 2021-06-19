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

#include "debug.h"

#include "hid_win.h"

#undef MIN
#define MIN(x,y) ((x) < (y)? (x): (y))

#ifdef _MSC_VER
	/* Thanks Microsoft, but I know how to use strncpy(). */
	#pragma warning(disable:4996)
#endif

void errtext(std::string s)
{
	FILE *erf = fopen("erf.txt", "a");
	fprintf(erf, "%s\n", s.c_str());
	fclose(erf);
}

std::string wchar2str( const char *where, wchar_t *WC )
{
	size_t count = wcstombs(NULL, WC, 1024);
	char MB[count + 1];
	memset(MB, 0, count + 1);
	int ret = wcstombs(MB, WC, count);

	if (ret == -1) {
		LOG_DEBUG("Cannot convert %s: %ls", where, WC);
		return "";
	}
	return MB;
}

hid_api_version api_version = {
	HID_API_VERSION_MAJOR,
	HID_API_VERSION_MINOR,
	HID_API_VERSION_PATCH
};

void hid_device::register_error(const char *op)
{
	WCHAR *ptr, *msg;
	(void)op; // unreferenced  param
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&msg, 0/*sz*/,
		NULL);

	/* Get rid of the CR and LF that FormatMessage() sticks at the
	   end of the message. Thanks Microsoft! */
	ptr = msg;
	while (*ptr) {
		if (*ptr == '\r') {
			*ptr = 0x0000;
			break;
		}
		ptr++;
	}

	/* Store the message off in the Device entry so that
	   the hid_error() function can pick it up. */
	LocalFree(last_error_str);
	last_error_str = msg;
}

HANDLE open_device(const char *path, BOOL open_rw)
{
	HANDLE handle;
	DWORD desired_access = (open_rw)? (GENERIC_WRITE | GENERIC_READ): GENERIC_READ;
	DWORD share_mode = FILE_SHARE_READ|FILE_SHARE_WRITE;

	handle = CreateFileA(path,
		desired_access,
		share_mode,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, //FILE_FLAG_OVERLAPPED,/*FILE_ATTRIBUTE_NORMAL,*/
		NULL);

	return handle;
}

const struct hid_api_version*  hid_version()
{
	return &api_version;
}

const char*  hid_version_str()
{
	return HID_API_VERSION_STR;
}

int  hid_init(void)
{
	/*
#ifndef HIDAPI_USE_DDK
	if (!initialized) {
		if (lookup_functions() < 0) {
			hid_exit();
			return -1;
		}
		initialized = TRUE;
	}
#endif
*/
//	initialized = TRUE;
	return 0;
}

int  hid_exit(void)
{
//#ifndef HIDAPI_USE_DDK
//	if (lib_handle)
//		FreeLibrary(lib_handle);
//	lib_handle = NULL;
//	initialized = FALSE;
//#endif
	return 0;
}

hid_device_info  *  hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	BOOL res;
	hid_device_info *root = NULL; /* return object */
	hid_device_info *cur_dev = NULL;

	/* Hard-coded GUID retreived by HidD_GetHidGuid */
	GUID InterfaceClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, {0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30} };

	/* Windows objects for interacting with the driver. */
	SP_DEVINFO_DATA devinfo_data;
	SP_DEVICE_INTERFACE_DATA device_interface_data;
	SP_DEVICE_INTERFACE_DETAIL_DATA_A *device_interface_detail_data = NULL;
	HDEVINFO device_info_set = INVALID_HANDLE_VALUE;
	char driver_name[256];
	int device_index = 0;

	if (hid_init() < 0)
		return NULL;

	/* Initialize the Windows objects. */
	memset(&devinfo_data, 0x0, sizeof(devinfo_data));
	devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
	device_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	/* Get information for all the devices belonging to the HID class. */
	device_info_set = SetupDiGetClassDevsA(&InterfaceClassGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	/* Iterate over each device in the HID class, looking for the right one. */

	for (;;) {
		HANDLE query_handle = INVALID_HANDLE_VALUE;
		DWORD required_size = 0;
		HIDD_ATTRIBUTES attrib;

		res = SetupDiEnumDeviceInterfaces(device_info_set,
			NULL,
			&InterfaceClassGuid,
			device_index,
			&device_interface_data);

		if (!res) {
			/* A return of FALSE from this function means that
			   there are no more devices. */
			break;
		}

		/* Call with 0-sized detail size, and let the function
		   tell us how long the detail struct needs to be. The
		   size is put in &required_size. */
		res = SetupDiGetDeviceInterfaceDetailA(device_info_set,
			&device_interface_data,
			NULL,
			0,
			&required_size,
			NULL);

		/* Allocate a long enough structure for device_interface_detail_data. */
		device_interface_detail_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*) malloc(required_size);
		device_interface_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

		/* Get the detailed data for this device. The detail data gives us
		   the device path for this device, which is then passed into
		   CreateFile() to get a handle to the device. */
		res = SetupDiGetDeviceInterfaceDetailA(device_info_set,
			&device_interface_data,
			device_interface_detail_data,
			required_size,
			NULL,
			NULL);

		if (!res) {
			/* register_error("Unable to call SetupDiGetDeviceInterfaceDetail");
			   Continue to the next device. */
			goto cont;
		}

		/* Populate devinfo_data. This function will return failure
		   when the device with such index doesn't exist. We've already checked it does. */
		res = SetupDiEnumDeviceInfo(device_info_set, device_index, &devinfo_data);
		if (!res)
			goto cont;


		/* Make sure this device has a driver bound to it. */
		res = SetupDiGetDeviceRegistryPropertyA(device_info_set, &devinfo_data,
			   SPDRP_DRIVER, NULL, (PBYTE)driver_name, sizeof(driver_name), NULL);
		if (!res)
			goto cont;

		//wprintf(L"HandleName: %s\n", device_interface_detail_data->DevicePath);

		/* Open a handle to the device */
		query_handle = open_device(device_interface_detail_data->DevicePath, FALSE);

		/* Check validity of query_handle. */
		if (query_handle == INVALID_HANDLE_VALUE) {
			/* Unable to open the device. */
			//register_error("CreateFile");
			goto cont_close;
		}


		/* Get the Vendor ID and Product ID for this device. */
		attrib.Size = sizeof(HIDD_ATTRIBUTES);
		HidD_GetAttributes(query_handle, &attrib);
		//wprintf(L"Product/Vendor: %x %x\n", attrib.ProductID, attrib.VendorID);

		/* Check the VID/PID to see if we should add this
		   device to the enumeration list. */
		if ((vendor_id == 0x0 || attrib.VendorID == vendor_id) &&
		    (product_id == 0x0 || attrib.ProductID == product_id)) {

			#define WSTR_LEN 512
			const char *str;
			hid_device_info *tmp;
			PHIDP_PREPARSED_DATA pp_data = NULL;
			HIDP_CAPS caps;
			NTSTATUS nt_res;

			/* VID/PID match. Create the record. */
			tmp = new hid_device_info;
			if (cur_dev) {
				cur_dev->next = tmp;
			}
			else {
				root = tmp;
			}
			cur_dev = tmp;

			/* Get the Usage Page and Usage for this device. */
			res = HidD_GetPreparsedData(query_handle, &pp_data);
			if (res) {
				nt_res = HidP_GetCaps(pp_data, &caps);
				if (nt_res == HIDP_STATUS_SUCCESS) {
					cur_dev->usage_page = caps.UsagePage;
					cur_dev->usage = caps.Usage;
				}

				HidD_FreePreparsedData(pp_data);
			}

			/* Fill out the record */
			cur_dev->next = NULL;
			str = device_interface_detail_data->DevicePath;
			if (str) {
				cur_dev->path = str;
			}
			else
				cur_dev->path.clear();

			/* Serial Number */
			static wchar_t WC[1024];
			memset(WC, 0, sizeof(wchar_t) * 1024);
			cur_dev->str_serial_number.clear();
			res = HidD_GetSerialNumberString(query_handle, WC, 1024);
			if (res) cur_dev->str_serial_number = wchar2str("HidD_GetSerialNumberString", WC);;

			/* Manufacturer String */
			memset(WC, 0, sizeof(wchar_t) * 1024);
			cur_dev->str_manufacturer_string.clear();
			res = HidD_GetManufacturerString(query_handle, WC, 1024);
			if (res) cur_dev->str_manufacturer_string = wchar2str("HidD_GetManufacturerString", WC);;

			/* Product String */
			memset(WC, 0, sizeof(wchar_t) * 1024);
			cur_dev->str_product_string.clear();
			res = HidD_GetProductString(query_handle, WC, 1024);
			if (res) cur_dev->str_product_string = wchar2str("HidD_GetProductString", WC);;

			/* VID/PID */
			cur_dev->vendor_id = attrib.VendorID;
			cur_dev->product_id = attrib.ProductID;

			/* Release Number */
			cur_dev->release_number = attrib.VersionNumber;

			/* Interface Number. It can sometimes be parsed out of the path
			   on Windows if a device has multiple interfaces. See
			   http://msdn.microsoft.com/en-us/windows/hardware/gg487473 or
			   search for "Hardware IDs for HID Devices" at MSDN. If it's not
			   in the path, it's set to -1. */
			cur_dev->interface_number = -1;
//			if (!cur_dev->path.empty()) {
//				char *interface_component = strstr(cur_dev->path, "&mi_");
//				if (interface_component) {
//					char *hex_str = interface_component + 4;
//					char *endptr = NULL;
//					cur_dev->interface_number = strtol(hex_str, &endptr, 16);
//					if (endptr == hex_str) {
//						/* The parsing failed. Set interface_number to -1. */
//						cur_dev->interface_number = -1;
//					}
//				}
//			}
		}

cont_close:
		CloseHandle(query_handle);
cont:
		/* We no longer need the detail data. It can be freed */
		free(device_interface_detail_data);

		device_index++;

	}

	/* Close the device information handle. */
	SetupDiDestroyDeviceInfoList(device_info_set);

	return root;

}

void    hid_free_enumeration(hid_device_info *devs)
{
	/* TODO: Merge this with the Linux version. This function is platform-independent. */
	hid_device_info *d = devs;
	while (d) {
		hid_device_info *next = d->next;
//		free(d->path);
//		free(d->serial_number);
//		free(d->manufacturer_string);
//		free(d->product_string);
		delete d;
		d = next;
	}
}

hid_device *  hid_open(unsigned short vendor_id, unsigned short product_id, std::string serial_number)
{
	/* TODO: Merge this functions with the Linux version. This function should be platform independent. */
	hid_device_info *devs, *cur_dev;
	std::string path_to_open;
	hid_device *handle = NULL;

	path_to_open.clear();

	devs = hid_enumerate(vendor_id, product_id);
	cur_dev = devs;
	while (cur_dev) {
		if (cur_dev->vendor_id == vendor_id &&
		    cur_dev->product_id == product_id) {
			if (!cur_dev->str_serial_number.empty()) {
				if (cur_dev->str_serial_number == serial_number) {
					path_to_open = cur_dev->path;
					break;
				}
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
		handle = hid_open_path(path_to_open);
	}

	hid_free_enumeration(devs);

	return handle;
}

hid_device *  hid_open_path(std::string path)
{
	hid_device *dev;
	HIDP_CAPS caps;
	PHIDP_PREPARSED_DATA pp_data = NULL;
	BOOLEAN res;
	NTSTATUS nt_res;

	if (hid_init() < 0) {
		return NULL;
	}

	dev = new hid_device;

	/* Open a handle to the device */
	dev->device_handle = open_device(path.c_str(), TRUE);

	/* Check validity of write_handle. */
	if (dev->device_handle == INVALID_HANDLE_VALUE) {
		/* System devices, such as keyboards and mice, cannot be opened in
		   read-write mode, because the system takes exclusive control over
		   them.  This is to prevent keyloggers.  However, feature reports
		   can still be sent and received.  Retry opening the device, but
		   without read/write access. */
		dev->device_handle = open_device(path.c_str(), FALSE);

		/* Check the validity of the limited device_handle. */
		if (dev->device_handle == INVALID_HANDLE_VALUE) {
			/* Unable to open the device, even without read-write mode. */
			dev->register_error("CreateFile");
errtext("CreateFile for ro failed: INVALID_HANDLE");
			goto err;
		} else
errtext("CreateFile for ro OK");
	} else
errtext("CreateFile for r/w OK");

	/* Set the Input Report buffer size to 64 reports. */
	res = HidD_SetNumInputBuffers(dev->device_handle, 64);
	if (!res) {
		dev->register_error("HidD_SetNumInputBuffers");
		goto err;
	}

	/* Get the Input Report length for the device. */
	res = HidD_GetPreparsedData(dev->device_handle, &pp_data);
	if (!res) {
		dev->register_error("HidD_GetPreparsedData");
		goto err;
	}
	nt_res = HidP_GetCaps(pp_data, &caps);
	if (nt_res != HIDP_STATUS_SUCCESS) {
		dev->register_error("HidP_GetCaps");
		goto err_pp_data;
	}
	dev->output_report_length = caps.OutputReportByteLength;
	dev->input_report_length = caps.InputReportByteLength;
	dev->feature_report_length = caps.FeatureReportByteLength;
	HidD_FreePreparsedData(pp_data);

	dev->read_buf = (char*) malloc(dev->input_report_length);

	return dev;

err_pp_data:
		HidD_FreePreparsedData(pp_data);
err:
		return NULL;
}

int  hid_device::hid_write(const unsigned char *data, size_t length)
{
	DWORD bytes_written = 0;
	int function_result = -1;
	BOOL res;
	BOOL overlapped = FALSE;

	unsigned char *buf;

	/* Make sure the right number of bytes are passed to WriteFile. Windows
	   expects the number of bytes which are in the _longest_ report (plus
	   one for the report number) bytes even if the data is a report
	   which is shorter than that. Windows gives us this value in
	   caps.OutputReportByteLength. If a user passes in fewer bytes than this,
	   use cached temporary buffer which is the proper size. */
	if (length >= output_report_length) {
		/* The user passed the right number of bytes. Use the buffer as-is. */
		buf = (unsigned char *) data;
	} else {
		if (write_buf == NULL)
			write_buf = (unsigned char *) malloc(output_report_length);
		buf = write_buf;
		memcpy(buf, data, length);
		memset(buf + length, 0, output_report_length - length);
		length = output_report_length;
	}

	res = WriteFile(device_handle, buf, (DWORD) length, NULL, &write_ol);

	if (!res) {
		if (GetLastError() != ERROR_IO_PENDING) {
			/* WriteFile() failed. Return error. */
			register_error("WriteFile");
			goto end_of_function;
		}
		overlapped = TRUE;
	}

	if (overlapped) {
		/* Wait for the transaction to complete. This makes
		   hid_write() synchronous. */
		res = WaitForSingleObject(write_ol.hEvent, 1000);
		if (res != WAIT_OBJECT_0) {
			/* There was a Timeout. */
			register_error("WriteFile/WaitForSingleObject Timeout");
			goto end_of_function;
		}

		/* Get the result. */
		res = GetOverlappedResult(device_handle, &write_ol, &bytes_written, FALSE/*wait*/);
		if (res) {
			function_result = bytes_written;
		}
		else {
			/* The Write operation failed. */
			register_error("WriteFile");
			goto end_of_function;
		}
	}

end_of_function:
	return function_result;
}


int hid_device::hid_read_timeout(unsigned char *data, size_t length, int milliseconds)
{
	DWORD bytes_read = 0;
	size_t copy_len = 0;
	BOOL res = FALSE;
	BOOL overlapped = FALSE;

	/* Copy the handle for convenience. */
	HANDLE ev = ol.hEvent;

	if (!read_pending) {
		/* Start an Overlapped I/O read. */
		read_pending = TRUE;
		memset(read_buf, 0, input_report_length);
		ResetEvent(ev);
		res = ReadFile(device_handle, read_buf, (DWORD) input_report_length, &bytes_read, &ol);

		if (!res) {
			if (GetLastError() != ERROR_IO_PENDING) {
				/* ReadFile() has failed.
				   Clean up and return error. */
				CancelIo(device_handle);
				read_pending = FALSE;
				goto end_of_function;
			}
			overlapped = TRUE;
		}
	}
	else {
		overlapped = TRUE;
	}

	if (overlapped) {
		if (milliseconds >= 0) {
			/* See if there is any data yet. */
			res = WaitForSingleObject(ev, milliseconds);
			if (res != WAIT_OBJECT_0) {
				/* There was no data this time. Return zero bytes available,
				   but leave the Overlapped I/O running. */
				return 0;
			}
		}

		/* Either WaitForSingleObject() told us that ReadFile has completed, or
		   we are in non-blocking mode. Get the number of bytes read. The actual
		   data has been copied to the data[] array which was passed to ReadFile(). */
		res = GetOverlappedResult(device_handle, &ol, &bytes_read, TRUE/*wait*/);
	}
	/* Set pending back to false, even if GetOverlappedResult() returned error. */
	read_pending = FALSE;

	if (res && bytes_read > 0) {
		if (read_buf[0] == 0x0) {
			/* If report numbers aren't being used, but Windows sticks a report
			   number (0x0) on the beginning of the report anyway. To make this
			   work like the other platforms, and to make it work more like the
			   HID spec, we'll skip over this byte. */
			bytes_read--;
			copy_len = length > bytes_read ? bytes_read : length;
			memcpy(data, read_buf+1, copy_len);
		}
		else {
			/* Copy the whole buffer, report number and all. */
			copy_len = length > bytes_read ? bytes_read : length;
			memcpy(data, read_buf, copy_len);
		}
	}

end_of_function:
	if (!res) {
		register_error("GetOverlappedResult");
		return -1;
	}

	return (int) copy_len;
}

int hid_device::hid_read(unsigned char *data, size_t length)
{
	return hid_read_timeout(data, length, (blocking)? -1: 0);
}

int hid_device::hid_set_nonblocking(int nonblock)
{
	blocking = !nonblock;
	return 0; /* Success */
}

int hid_device::hid_send_feature_report(const unsigned char *data, size_t length)
{
	BOOL res = FALSE;
	unsigned char *buf;
	size_t length_to_send;

	/* Windows expects at least caps.FeatureReportByteLength bytes passed
	   to HidD_SetFeature(), even if the report is shorter. Any less sent and
	   the function fails with error ERROR_INVALID_PARAMETER set. Any more
	   and HidD_SetFeature() silently truncates the data sent in the report
	   to caps.FeatureReportByteLength. */
	if (length >= feature_report_length) {
		buf = (unsigned char *) data;
		length_to_send = length;
	} else {
		if (feature_buf == NULL)
			feature_buf = (unsigned char *) malloc(feature_report_length);
		buf = feature_buf;
		memcpy(buf, data, length);
		memset(buf + length, 0, feature_report_length - length);
		length_to_send = feature_report_length;
	}

	res = HidD_SetFeature(device_handle, (PVOID)buf, (DWORD) length_to_send);

	if (!res) {
		register_error("HidD_SetFeature");
		return -1;
	}

	return (int) length;
}


int hid_device::hid_get_feature_report(unsigned char *data, size_t length)
{
	BOOL res;
#if 0
	res = HidD_GetFeature(device_handle, data, length);
	if (!res) {
		register_error("HidD_GetFeature");
		return -1;
	}
	return 0; /* HidD_GetFeature() doesn't give us an actual length, unfortunately */
#else
	DWORD bytes_returned;

	OVERLAPPED ol;
	memset(&ol, 0, sizeof(ol));

	res = DeviceIoControl(device_handle,
		IOCTL_HID_GET_FEATURE,
		data, (DWORD) length,
		data, (DWORD) length,
		&bytes_returned, &ol);

	if (!res) {
		if (GetLastError() != ERROR_IO_PENDING) {
			/* DeviceIoControl() failed. Return error. */
			register_error("Send Feature Report DeviceIoControl");
			return -1;
		}
	}

	/* Wait here until the write is done. This makes
	   hid_get_feature_report() synchronous. */
	res = GetOverlappedResult(device_handle, &ol, &bytes_returned, TRUE/*wait*/);
	if (!res) {
		/* The operation failed. */
		register_error("Send Feature Report GetOverLappedResult");
		return -1;
	}

	/* bytes_returned does not include the first byte which contains the
	   report ID. The data buffer actually contains one more byte than
	   bytes_returned. */
	bytes_returned++;

	return bytes_returned;
#endif
}


int hid_device::hid_get_input_report(unsigned char *data, size_t length)
{
	BOOL res;
#if 0
	res = HidD_GetInputReport(device_handle, data, length);
	if (!res) {
		register_error("HidD_GetInputReport");
		return -1;
	}
	return length;
#else
	DWORD bytes_returned;

	OVERLAPPED ol;
	memset(&ol, 0, sizeof(ol));

	res = DeviceIoControl(device_handle,
		IOCTL_HID_GET_INPUT_REPORT,
		data, (DWORD) length,
		data, (DWORD) length,
		&bytes_returned, &ol);

	if (!res) {
		if (GetLastError() != ERROR_IO_PENDING) {
			/* DeviceIoControl() failed. Return error. */
			register_error("Send Input Report DeviceIoControl");
			return -1;
		}
	}

	/* Wait here until the write is done. This makes
	   hid_get_feature_report() synchronous. */
	res = GetOverlappedResult(device_handle, &ol, &bytes_returned, TRUE/*wait*/);
	if (!res) {
		/* The operation failed. */
		register_error("Send Input Report GetOverLappedResult");
		return -1;
	}

	/* bytes_returned does not include the first byte which contains the
	   report ID. The data buffer actually contains one more byte than
	   bytes_returned. */
	bytes_returned++;

	return bytes_returned;
#endif
}

void hid_device::hid_close()
{
	CancelIo(device_handle);
}

std::string hid_device::hid_get_manufacturer_string()
{
	BOOL res;
	wchar_t WC[256];

	res = HidD_GetManufacturerString(device_handle, WC, 256);
	if (!res) {
		register_error("HidD_GetManufacturerString");
		return "";
	}
	return  wchar2str("HidD_GetManufacturerString", WC);
}

std::string hid_device::hid_get_product_string()
{
	BOOL res;
	wchar_t WC[256];
	res = HidD_GetProductString(device_handle, WC, 256);
	if (!res) {
		register_error("HidD_GetProductString");
		return "";
	}
	return wchar2str("HidD_GetProductString", WC);
}

std::string hid_device::hid_get_serial_number_string()
{
	BOOL res;
	wchar_t WC[256];

	res = HidD_GetSerialNumberString(device_handle, WC, 256);
	if (!res) {
		register_error("HidD_GetSerialNumberString");
		return "";
	}
	return wchar2str("HidD_GetSerialNumberString", WC);
}

int hid_device::hid_get_indexed_string(int string_index, std::string string, size_t maxlen)
{
	BOOL res;
	wchar_t WC[maxlen+1];
	res = HidD_GetIndexedString(device_handle, string_index, WC, maxlen);
	if (!res) {
		register_error("HidD_GetIndexedString");
		string = "";
		return -1;
	}
	string = wchar2str("HidD_Get_IndexedString", WC);
	return 0;
}


const char * hid_device::hid_error()
{
	return "hid_error for global errors is not implemented yet";
}

