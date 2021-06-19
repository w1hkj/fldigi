/***********************************************************************
 HIDAPI - Multi-Platform library for communication with HID devices.

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

#include <wchar.h>

#define HID_API_VERSION_MAJOR 0
#define HID_API_VERSION_MINOR 10
#define HID_API_VERSION_PATCH 1

/* Helper macros */
#define HID_API_AS_STR_IMPL(x) #x
#define HID_API_AS_STR(x) HID_API_AS_STR_IMPL(x)
#define HID_API_TO_VERSION_STR(v1, v2, v3) HID_API_AS_STR(v1.v2.v3)

#define HID_API_VERSION_STR HID_API_TO_VERSION_STR(HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH)

#ifdef __WIN32__
#  include "hid_win.h"
#  else
#  ifdef __APPLE__
#    include "hid_mac.h"
#  else
#    include "hid_lin.h"
#  endif
#endif

