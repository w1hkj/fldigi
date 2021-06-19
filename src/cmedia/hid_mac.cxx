/***********************************************************************
 HIDAPI - Multi-Platform library for communication with HID devices.

 hid_mac.cxx

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

/* See Apple Technical Note TN2187 for details on IOHidManager. */
/***********************************************************************
 Tested on both Intel and M1 architecture
***********************************************************************/

#include <iostream>

#include "hidapi.h"

#define UTF8 134217984

std::string wchar2str( char *WC )
{
//	size_t count = sizeof(WC);
//	char MB[count + 1];
//	memset(MB, 0, count + 1);
//	size_t ret = wcstombs(MB, WC, count);
	static std::string retstr;
//	if (ret) retstr = MB;
	return retstr;
}

int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count)
{
	if(count == 0) {
		errno = EINVAL;
		return -1;
	}

	if(pthread_mutex_init(&barrier->mutex, 0) < 0) {
		return -1;
	}
	if(pthread_cond_init(&barrier->cond, 0) < 0) {
		pthread_mutex_destroy(&barrier->mutex);
		return -1;
	}
	barrier->trip_count = count;
	barrier->count = 0;

	return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
	pthread_cond_destroy(&barrier->cond);
	pthread_mutex_destroy(&barrier->mutex);
	return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier)
{
	pthread_mutex_lock(&barrier->mutex);
	++(barrier->count);
	if(barrier->count >= barrier->trip_count)
	{
		barrier->count = 0;
		pthread_cond_broadcast(&barrier->cond);
		pthread_mutex_unlock(&barrier->mutex);
		return 1;
	}
	else
	{
		pthread_cond_wait(&barrier->cond, &(barrier->mutex));
		pthread_mutex_unlock(&barrier->mutex);
		return 0;
	}
}

static	IOHIDManagerRef hid_mgr = 0x0;


void hid_device::register_error(const char *op)
{
}


static int32_t get_int_property(IOHIDDeviceRef device, CFStringRef key)
{
	CFTypeRef ref;
	int32_t value;

	ref = IOHIDDeviceGetProperty(device, key);
	if (ref) {
		if (CFGetTypeID(ref) == CFNumberGetTypeID()) {
			CFNumberGetValue((CFNumberRef) ref, kCFNumberSInt32Type, &value);
			return value;
		}
	}
	return 0;
}

static unsigned short get_vendor_id(IOHIDDeviceRef device)
{
	return get_int_property(device, CFSTR(kIOHIDVendorIDKey));
}

static unsigned short get_product_id(IOHIDDeviceRef device)
{
	return get_int_property(device, CFSTR(kIOHIDProductIDKey));
}

static int32_t get_max_report_length(IOHIDDeviceRef device)
{
	return get_int_property(device, CFSTR(kIOHIDMaxInputReportSizeKey));
}

static int get_string_property(IOHIDDeviceRef device, CFStringRef prop, char *buf, size_t len)
{
	if (!len)
		return 0;

	CFStringRef str = reinterpret_cast<const __CFString *>(IOHIDDeviceGetProperty(device, prop));
	UniChar ubuf[len];
	memset(buf, 0, len);
	if (str) {
		CFIndex str_len = CFStringGetLength(str);
		CFRange range;
		range.location = 0;
		range.length = ((size_t)str_len > len)? len: (size_t)str_len;

		CFStringGetCharacters(
			str,
			range,
			ubuf);
		for (int i = 0; i < range.length; i++) buf[i] = ubuf[i];
		return 0;
	}
	else
		return -1;

}

static int get_serial_number(IOHIDDeviceRef device, char *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDSerialNumberKey), buf, len);
}

static int get_manufacturer_string(IOHIDDeviceRef device, char *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDManufacturerKey), buf, len);
}

static int get_product_string(IOHIDDeviceRef device, char *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDProductKey), buf, len);
}

/* hidapi_IOHIDDeviceGetService()
 *
 * Return the io_service_t corresponding to a given IOHIDDeviceRef, either by:
 * - on OS X 10.6 and above, calling IOHIDDeviceGetService()
 * - on OS X 10.5, extract it from the IOHIDDevice struct
 */
static io_service_t hidapi_IOHIDDeviceGetService(IOHIDDeviceRef device)
{
	static void *iokit_framework = NULL;
	static io_service_t (*dynamic_IOHIDDeviceGetService)(IOHIDDeviceRef device) = NULL;

	/* Use dlopen()/dlsym() to get a pointer to IOHIDDeviceGetService() if it exists.
	 * If any of these steps fail, dynamic_IOHIDDeviceGetService will be left NULL
	 * and the fallback method will be used.
	 */
	if (iokit_framework == NULL) {
		iokit_framework = dlopen("/System/Library/IOKit.framework/IOKit", RTLD_LAZY);

		if (iokit_framework != NULL)
			dynamic_IOHIDDeviceGetService = reinterpret_cast<unsigned int (*)(__IOHIDDevice *)>(dlsym(iokit_framework, "IOHIDDeviceGetService"));
	}

	if (dynamic_IOHIDDeviceGetService != NULL) {
		/* Running on OS X 10.6 and above: IOHIDDeviceGetService() exists */
		return dynamic_IOHIDDeviceGetService(device);
	}
	else
	{
		/* Running on OS X 10.5: IOHIDDeviceGetService() doesn't exist.
		 *
		 * Be naughty and pull the service out of the IOHIDDevice.
		 * IOHIDDevice is an opaque struct not exposed to applications, but its
		 * layout is stable through all available versions of OS X.
		 * Tested and working on OS X 10.5.8 i386, x86_64, and ppc.
		 */
		struct IOHIDDevice_internal {
			/* The first field of the IOHIDDevice struct is a
			 * CFRuntimeBase (which is a private CF struct).
			 *
			 * a, b, and c are the 3 fields that make up a CFRuntimeBase.
			 * See http://opensource.apple.com/source/CF/CF-476.18/CFRuntime.h
			 *
			 * The second field of the IOHIDDevice is the io_service_t we're looking for.
			 */
			uintptr_t a;
			uint8_t b[4];
#if __LP64__
			uint32_t c;
#endif
			io_service_t service;
		};
		struct IOHIDDevice_internal *tmp = (struct IOHIDDevice_internal *)device;

		return tmp->service;
	}
}

/* Initialize the IOHIDManager. Return 0 for success and -1 for failure. */
static int init_hid_manager(void)
{
	/* Initialize all the HID Manager Objects */
	hid_mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	if (hid_mgr) {
		IOHIDManagerSetDeviceMatching(hid_mgr, NULL);
		IOHIDManagerScheduleWithRunLoop(hid_mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		return 0;
	}

	return -1;
}

/* Initialize the IOHIDManager if necessary. This is the public function, and
   it is safe to call this function repeatedly. Return 0 for success and -1
   for failure. */
int hid_init(void)
{
	if (!hid_mgr) {
		return init_hid_manager();
	}

	/* Already initialized. */
	return 0;
}

int hid_exit(void)
{
	if (hid_mgr) {
		/* Close the HID manager. */
		IOHIDManagerClose(hid_mgr, kIOHIDOptionsTypeNone);
		CFRelease(hid_mgr);
		hid_mgr = NULL;
	}

	return 0;
}

static void process_pending_events(void) {
	SInt32 res;
	do {
		res = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.001, FALSE);
	} while(res != kCFRunLoopRunFinished && res != kCFRunLoopRunTimedOut);
}

hid_device_info  *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	hid_device_info *root = NULL; /* return object */
	hid_device_info *cur_dev = NULL;
	CFIndex num_devices;
	int i;

	/* Set up the HID Manager if it hasn't been done */
	if (hid_init() < 0)
		return NULL;

	/* give the IOHIDManager a chance to update itself */
	process_pending_events();

	/* Get a list of the Devices */
	IOHIDManagerSetDeviceMatching(hid_mgr, NULL);
	CFSetRef device_set = IOHIDManagerCopyDevices(hid_mgr);

	/* Convert the list into a C array so we can iterate easily. */
	num_devices = CFSetGetCount(device_set);
	IOHIDDeviceRef *device_array = reinterpret_cast<IOHIDDeviceRef *>(calloc(num_devices, sizeof(IOHIDDeviceRef)));
	CFSetGetValues(device_set, (const void **) device_array);

	/* Iterate over each device, making an entry for it. */
	for (i = 0; i < num_devices; i++) {
		unsigned short dev_vid;
		unsigned short dev_pid;
		#define BUF_LEN 256
		char buf[BUF_LEN];

		IOHIDDeviceRef dev = device_array[i];

        if (!dev) {
            continue;
        }
		dev_vid = get_vendor_id(dev);
		dev_pid = get_product_id(dev);

		/* Check the VID/PID against the arguments */
		if ((vendor_id == 0x0 || vendor_id == dev_vid) &&
		    (product_id == 0x0 || product_id == dev_pid)) {
			hid_device_info *tmp;
			io_object_t iokit_dev;
			kern_return_t res;
			io_string_t path;

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
			cur_dev->usage_page = get_int_property(dev, CFSTR(kIOHIDPrimaryUsagePageKey));
			cur_dev->usage = get_int_property(dev, CFSTR(kIOHIDPrimaryUsageKey));

			/* Fill out the record */
			cur_dev->next = NULL;

			/* Fill in the path (IOService plane) */
			iokit_dev = hidapi_IOHIDDeviceGetService(dev);
			res = IORegistryEntryGetPath(iokit_dev, kIOServicePlane, path);
			if (res == KERN_SUCCESS)
				cur_dev->path = strdup(path);
			else
				cur_dev->path = strdup("");

			/* Serial Number */
			get_serial_number(dev, buf, BUF_LEN);
			cur_dev->str_serial_number = buf;

			/* Manufacturer and Product strings */
			get_manufacturer_string(dev, buf, BUF_LEN);
			cur_dev->str_manufacturer_string = buf;

			get_product_string(dev, buf, BUF_LEN);
std::cout << buf << std::endl;

			cur_dev->str_product_string = buf;

			/* VID/PID */
			cur_dev->vendor_id = dev_vid;
			cur_dev->product_id = dev_pid;

			/* Release Number */
			cur_dev->release_number = get_int_property(dev, CFSTR(kIOHIDVersionNumberKey));

			/* Interface Number (Unsupported on Mac)*/
			cur_dev->interface_number = -1;
		}
	}

	free(device_array);
	CFRelease(device_set);

	return root;
}

void  hid_free_enumeration(hid_device_info *devs)
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
	/* This function is identical to the Linux version. Platform independent. */
	hid_device_info *devs, *cur_dev;
	std::string path_to_open;
	hid_device * handle = NULL;

	path_to_open.clear();

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
		handle = hid_open_path(path_to_open);
	}

	hid_free_enumeration(devs);

	return handle;
}

static void hid_device_removal_callback(void *context, IOReturn result,
                                        void *sender)
{
	/* Stop the Run Loop for this device. */
	hid_device *d = reinterpret_cast<hid_device *>(context);

	d->disconnected = 1;
	CFRunLoopStop(d->run_loop);
}

/* The Run Loop calls this function for each input report received.
   This function puts the data into a linked list to be picked up by
   hid_read(). */
static void hid_report_callback(void *context, IOReturn result, void *sender,
                         IOHIDReportType report_type, uint32_t report_id,
                         uint8_t *report, CFIndex report_length)
{
	input_report *rpt;
	hid_device *dev = reinterpret_cast<hid_device *>(context);

	/* Make a new Input Report object */
	rpt = reinterpret_cast<input_report *>(calloc(1, sizeof(input_report)));
	rpt->data = reinterpret_cast<unsigned char *>(calloc(1, report_length));
	memcpy(rpt->data, report, report_length);
	rpt->len = report_length;
	rpt->next = NULL;

	/* Lock this section */
	pthread_mutex_lock(&dev->mutex);

	/* Attach the new report object to the end of the list. */
	if (dev->input_reports == NULL) {
		/* The list is empty. Put it at the root. */
		dev->input_reports = rpt;
	}
	else {
		/* Find the end of the list and attach. */
		input_report *cur = dev->input_reports;
		int num_queued = 0;
		while (cur->next != NULL) {
			cur = cur->next;
			num_queued++;
		}
		cur->next = rpt;

		/* Pop one off if we've reached 30 in the queue. This
		   way we don't grow forever if the user never reads
		   anything from the device. */
		if (num_queued > 30) {
			dev->return_data(NULL, 0);
		}
	}

	/* Signal a waiting thread that there is data. */
	pthread_cond_signal(&dev->condition);

	/* Unlock */
	pthread_mutex_unlock(&dev->mutex);

}

/* This gets called when the read_thread's run loop gets signaled by
   hid_close(), and serves to stop the read_thread's run loop. */
static void perform_signal_callback(void *context)
{
	hid_device *dev = reinterpret_cast<hid_device *>(context);
	CFRunLoopStop(dev->run_loop); /*TODO: CFRunLoopGetCurrent()*/
}

static void *read_thread(void *param)
{
	hid_device *dev = reinterpret_cast<hid_device *>(param);
	SInt32 code;

	/* Move the device's run loop to this thread. */
	IOHIDDeviceScheduleWithRunLoop(dev->device_handle, CFRunLoopGetCurrent(), dev->run_loop_mode);

	/* Create the RunLoopSource which is used to signal the
	   event loop to stop when hid_close() is called. */
	CFRunLoopSourceContext ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.version = 0;
	ctx.info = dev;
	ctx.perform = &perform_signal_callback;
	dev->source = CFRunLoopSourceCreate(kCFAllocatorDefault, 0/*order*/, &ctx);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), dev->source, dev->run_loop_mode);

	/* Store off the Run Loop so it can be stopped from hid_close()
	   and on device disconnection. */
	dev->run_loop = CFRunLoopGetCurrent();

	/* Notify the main thread that the read thread is up and running. */
	pthread_barrier_wait(&dev->barrier);

	/* Run the Event Loop. CFRunLoopRunInMode() will dispatch HID input
	   reports into the hid_report_callback(). */
	while (!dev->shutdown_thread && !dev->disconnected) {
		code = CFRunLoopRunInMode(dev->run_loop_mode, 1000/*sec*/, FALSE);
		/* Return if the device has been disconnected */
		if (code == kCFRunLoopRunFinished) {
			dev->disconnected = 1;
			break;
		}


		/* Break if The Run Loop returns Finished or Stopped. */
		if (code != kCFRunLoopRunTimedOut &&
		    code != kCFRunLoopRunHandledSource) {
			/* There was some kind of error. Setting
			   shutdown seems to make sense, but
			   there may be something else more appropriate */
			dev->shutdown_thread = 1;
			break;
		}
	}

	/* Now that the read thread is stopping, Wake any threads which are
	   waiting on data (in hid_read_timeout()). Do this under a mutex to
	   make sure that a thread which is about to go to sleep waiting on
	   the condition actually will go to sleep before the condition is
	   signaled. */
	pthread_mutex_lock(&dev->mutex);
	pthread_cond_broadcast(&dev->condition);
	pthread_mutex_unlock(&dev->mutex);

	/* Wait here until hid_close() is called and makes it past
	   the call to CFRunLoopWakeUp(). This thread still needs to
	   be valid when that function is called on the other thread. */
	pthread_barrier_wait(&dev->shutdown_barrier);

	return NULL;
}

/* hid_open_path()
 *
 * path must be a valid path to an IOHIDDevice in the IOService plane
 * Example: "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/EHC1@1D,7/AppleUSBEHCI/PLAYSTATION(R)3 Controller@fd120000/IOUSBInterface@0/IOUSBHIDDriver"
 */
hid_device * hid_open_path(std::string path)
{
	hid_device *dev = NULL;
	io_registry_entry_t entry = MACH_PORT_NULL;
	IOReturn ret;

	dev = new hid_device;

	/* Set up the HID Manager if it hasn't been done */
	if (hid_init() < 0)
		return NULL;

	/* Get the IORegistry entry for the given path */
	entry = IORegistryEntryFromPath(kIOMasterPortDefault, path.c_str());
	if (entry == MACH_PORT_NULL) {
		/* Path wasn't valid (maybe device was removed?) */
		goto return_error;
	}

	/* Create an IOHIDDevice for the entry */
	dev->device_handle = IOHIDDeviceCreate(kCFAllocatorDefault, entry);
	if (dev->device_handle == NULL) {
		/* Error creating the HID device */
		goto return_error;
	}

	/* Open the IOHIDDevice */
	ret = IOHIDDeviceOpen(dev->device_handle, kIOHIDOptionsTypeSeizeDevice);
	if (ret == kIOReturnSuccess) {
		char str[32];

		/* Create the buffers for receiving data */
		dev->max_input_report_len = (CFIndex) get_max_report_length(dev->device_handle);
		dev->input_report_buf = reinterpret_cast<uint8_t *>(calloc(dev->max_input_report_len, sizeof(uint8_t)));

		/* Create the Run Loop Mode for this device.
		   printing the reference seems to work. */
		sprintf(str, "HIDAPI_%p", dev->device_handle);
		dev->run_loop_mode =
			CFStringCreateWithCString(NULL, str, kCFStringEncodingASCII);

		/* Attach the device to a Run Loop */
		IOHIDDeviceRegisterInputReportCallback(
			dev->device_handle, dev->input_report_buf, dev->max_input_report_len,
			&hid_report_callback, dev);
		IOHIDDeviceRegisterRemovalCallback(dev->device_handle, hid_device_removal_callback, dev);

		/* Start the read thread */
		pthread_create(&dev->thread, NULL, read_thread, dev);

		/* Wait here for the read thread to be initialized. */
		pthread_barrier_wait(&dev->barrier);

		IOObjectRelease(entry);
		return dev;
	}
	else {
		goto return_error;
	}

return_error:
	if (dev->device_handle != NULL)
		CFRelease(dev->device_handle);

	if (entry != MACH_PORT_NULL)
		IOObjectRelease(entry);

	delete dev;
	return NULL;
}

int hid_device::set_report(IOHIDReportType type, const unsigned char *data, size_t length)
{
	const unsigned char *data_to_send;
	size_t length_to_send;
	IOReturn res;

	/* Return if the device has been disconnected. */
	if (disconnected)
		return -1;

	if (data[0] == 0x0) {
		/* Not using numbered Reports.
		   Don't send the report number. */
		data_to_send = data+1;
		length_to_send = length-1;
	}
	else {
		/* Using numbered Reports.
		   Send the Report Number */
		data_to_send = data;
		length_to_send = length;
	}

	if (!disconnected) {
		res = IOHIDDeviceSetReport(device_handle,
					   type,
					   data[0], /* Report ID*/
					   data_to_send, length_to_send);

		if (res == kIOReturnSuccess) {
			return length;
		}
		else
			return -1;
	}

	return -1;
}

int hid_device::hid_write(const unsigned char *data, size_t length)
{
	return set_report(kIOHIDReportTypeOutput, data, length);
}

/* Helper function, so that this isn't duplicated in hid_read(). */
int hid_device::return_data(unsigned char *data, size_t length)
{
	/* Copy the data out of the linked list item (rpt) into the
	   return buffer (data), and delete the liked list item. */
	input_report *rpt = input_reports;
	size_t len = (length < rpt->len)? length: rpt->len;
	memcpy(data, rpt->data, len);
	input_reports = rpt->next;
	free(rpt->data);
	free(rpt);
	return len;
}

int hid_device::cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	while (!input_reports) {
		int res = pthread_cond_wait(cond, mutex);
		if (res != 0)
			return res;

		/* A res of 0 means we may have been signaled or it may
		   be a spurious wakeup. Check to see that there's acutally
		   data in the queue before returning, and if not, go back
		   to sleep. See the pthread_cond_timedwait() man page for
		   details. */

		if (shutdown_thread || disconnected)
			return -1;
	}

	return 0;
}

int hid_device::cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
	while (!input_reports) {
		int res = pthread_cond_timedwait(cond, mutex, abstime);
		if (res != 0)
			return res;

		/* A res of 0 means we may have been signaled or it may
		   be a spurious wakeup. Check to see that there's acutally
		   data in the queue before returning, and if not, go back
		   to sleep. See the pthread_cond_timedwait() man page for
		   details. */

		if (shutdown_thread || disconnected)
			return -1;
	}

	return 0;

}

int hid_device::hid_read_timeout(unsigned char *data, size_t length, int milliseconds)
{
	int bytes_read = -1;

	/* Lock the access to the report list. */
	pthread_mutex_lock(&mutex);

	/* There's an input report queued up. Return it. */
	if (input_reports) {
		/* Return the first one */
		bytes_read = return_data(data, length);
		goto ret;
	}

	/* Return if the device has been disconnected. */
	if (disconnected) {
		bytes_read = -1;
		goto ret;
	}

	if (shutdown_thread) {
		/* This means the device has been closed (or there
		   has been an error. An error code of -1 should
		   be returned. */
		bytes_read = -1;
		goto ret;
	}

	/* There is no data. Go to sleep and wait for data. */

	if (milliseconds == -1) {
		/* Blocking */
		int res;
		res = cond_wait(&condition, &mutex);
		if (res == 0)
			bytes_read = return_data(data, length);
		else {
			/* There was an error, or a device disconnection. */
			bytes_read = -1;
		}
	}
	else if (milliseconds > 0) {
		/* Non-blocking, but called with timeout. */
		int res;
		struct timespec ts;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		TIMEVAL_TO_TIMESPEC(&tv, &ts);
		ts.tv_sec += milliseconds / 1000;
		ts.tv_nsec += (milliseconds % 1000) * 1000000;
		if (ts.tv_nsec >= 1000000000L) {
			ts.tv_sec++;
			ts.tv_nsec -= 1000000000L;
		}

		res = cond_timedwait(&condition, &mutex, &ts);
		if (res == 0)
			bytes_read = return_data(data, length);
		else if (res == ETIMEDOUT)
			bytes_read = 0;
		else
			bytes_read = -1;
	}
	else {
		/* Purely non-blocking */
		bytes_read = 0;
	}

ret:
	/* Unlock */
	pthread_mutex_unlock(&mutex);
	return bytes_read;
}

int hid_device::hid_read(unsigned char *data, size_t length)
{
	return hid_read_timeout(data, length, (blocking)? -1: 0);
}

int hid_device::hid_set_nonblocking(int nonblock)
{
	/* All Nonblocking operation is handled by the library. */
	blocking = !nonblock;
	return 0;
}

int hid_device::hid_send_feature_report(const unsigned char *data, size_t length)
{
	return set_report(kIOHIDReportTypeFeature, data, length);
}

int hid_device::hid_get_feature_report(unsigned char *data, size_t length)
{
	CFIndex len = length;
	IOReturn res;

	/* Return if the device has been unplugged. */
	if (disconnected)
		return -1;

	res = IOHIDDeviceGetReport(device_handle,
	                           kIOHIDReportTypeFeature,
	                           data[0], /* Report ID */
	                           data, &len);
	if (res == kIOReturnSuccess)
		return len;
	else
		return -1;
}


void hid_device::hid_close()
{
	/* Disconnect the report callback before close. */
	if (!disconnected) {
		IOHIDDeviceRegisterInputReportCallback(
			device_handle, input_report_buf, max_input_report_len,
			NULL, this);
		IOHIDDeviceRegisterRemovalCallback(device_handle, NULL, this);
		IOHIDDeviceUnscheduleFromRunLoop(device_handle, run_loop, run_loop_mode);
		IOHIDDeviceScheduleWithRunLoop(device_handle, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
	}

	/* Cause read_thread() to stop. */
	shutdown_thread = 1;

	/* Wake up the run thread's event loop so that the thread can exit. */
	CFRunLoopSourceSignal(source);
	CFRunLoopWakeUp(run_loop);

	/* Notify the read thread that it can shut down now. */
	pthread_barrier_wait(&shutdown_barrier);

	/* Wait for read_thread() to end. */
	pthread_join(thread, NULL);

	/* Close the OS handle to the device, but only if it's not
	   been unplugged. If it's been unplugged, then calling
	   IOHIDDeviceClose() will crash. */
	if (!disconnected) {
		IOHIDDeviceClose(device_handle, kIOHIDOptionsTypeSeizeDevice);
	}

	/* Clear out the queue of received reports. */
	pthread_mutex_lock(&mutex);
	while (input_reports) {
		return_data(NULL, 0);
	}
	pthread_mutex_unlock(&mutex);
	CFRelease(device_handle);

//	delete dev;
}

const char * hid_device::hid_error()
{
	return "HID error string not implemented";
}
