/***********************************************************************
 HIDAPI - Multi-Platform library for communication with HID devices.

 Alan Ott
 Signal 11 Software

 8/22/2009
 Linux Version - 6/2/2009

 Copyright 2009, All Rights Reserved.

 This software is licensed under the terms of the GNU General Public 
 License v3.

***********************************************************************/

#ifdef __WIN32__
# include "hid_win.cxx"
#else
# ifdef __APPLE__
#   include "hid_mac.cxx"
# else
#   include "hid_lin.cxx"
# endif
#endif

