/* Adapted from libgw32c-0.4 */

/*
Platform SDK: Windows System Information 

See: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/sysinfo/sysinfo_5r76.asp


*/

#include <stdio.h>
#include <windows.h>

#define BUFSIZE 80

#define VIDSIZE  13
static const char * vid_cyrix =   "CyrixInstead";
static const char * vid_centaur = "CentaurHauls";
static const char * vid_rise =    "RiseRiseRise";
static const char * vid_intel =   "GenuineIntel";
static const char * vid_amd =     "AuthenticAMD";
static const char * vid_tmx86 =   "GenuineTMx86";
static const char * vid_geode =   "Geode by NSC";
static const char * vid_umc =     "UMC UMC UMC ";
static const char * vid_amd2 =    "AMD ISBETTER";
static const char * vid_amd3 =    "DEI         ";
static const char * vid_amd4 =    "NexGenerationAMD";
static const char * vid_nexgen =  "NexGenDriven";

#define cpuid(op,a,b,c,d)\
  __asm__("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (op));

static void GetVendorID (char *vid)
{
  unsigned long MaxEax, ebx, ecx, edx, zerobyte;

  cpuid (0, MaxEax, ebx, ecx, edx);
  memcpy (vid, &ebx, 4);
  memcpy (vid+4, &edx, 4);
  memcpy (vid+8, &ecx, 4);
  zerobyte = 0;
  memcpy (vid+12, &zerobyte, 1);
}

BOOL GetMachInfo(LPSTR MachineName, LPSTR ProcessorName)
{
   SYSTEM_INFO sysinf;
   int family;
   char VendorId [VIDSIZE+2];

   ZeroMemory(&sysinf, sizeof(SYSTEM_INFO));
   GetSystemInfo(&sysinf);
   family = sysinf.wProcessorLevel;

   switch (sysinf.wProcessorArchitecture) {
   	case PROCESSOR_ARCHITECTURE_UNKNOWN:
	   strcpy(MachineName, "unknown");
	   break;
	case PROCESSOR_ARCHITECTURE_INTEL:
	   strcpy(MachineName, "ix86");
	   break;
	case PROCESSOR_ARCHITECTURE_MIPS:
	   strcpy(MachineName, "mips");
	   break;
	case PROCESSOR_ARCHITECTURE_ALPHA:
	   strcpy(MachineName, "alpha");
	   break;
	case PROCESSOR_ARCHITECTURE_PPC:
	   strcpy(MachineName, "ppc");
	   break;
	case PROCESSOR_ARCHITECTURE_IA64:
	   strcpy(MachineName, "IA64");
	   break;
	case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
	   strcpy(MachineName, "IA32 on Win64");
	   break;
	case PROCESSOR_ARCHITECTURE_AMD64:
	   strcpy(MachineName, "amd64");
	   break;
	case PROCESSOR_ARCHITECTURE_SHX:
	   strcpy(MachineName, "sh");
	   break;
	case PROCESSOR_ARCHITECTURE_ARM:
	   strcpy(MachineName, "arm");
	   break;
	case PROCESSOR_ARCHITECTURE_ALPHA64:
	   strcpy(MachineName, "alpha64");
	   break;
	case PROCESSOR_ARCHITECTURE_MSIL:
	   strcpy(MachineName, "msil");
	   break;
	default: 
	   strcpy(MachineName, "unknown");
	   break;
	}

   if (sysinf.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	switch(family) {
    case 3:
	   strcpy(MachineName, "i386");
       break;
    case 4:
	   strcpy(MachineName, "i486");
       break;
    case 5:
	   strcpy(MachineName, "i586");
       break;
    case 6:
       strcpy(MachineName, "i686");
       break;
    case 7:
       strcpy(MachineName, "i786");
       break;
    case 15:
		strcpy(MachineName, "i786");
		break;
   default:
		strcpy(MachineName, "ix86");
   }
 		
	GetVendorID(VendorId);

   if (!strcmp(VendorId, vid_cyrix))
   	  strcpy(ProcessorName, "Cyrix");
   else if (!strcmp(VendorId, vid_centaur))
   	  strcpy(ProcessorName, "Centaur");
   else if (!strcmp(VendorId,  vid_rise))
   	  strcpy(ProcessorName, "Rise");
   else if (!strcmp(VendorId,  vid_intel))
   	  strcpy(ProcessorName, "Intel");
   else if (!strcmp(VendorId,  vid_amd))
   	  strcpy(ProcessorName, "AMD");
   else if (!strcmp(VendorId,  vid_tmx86))
   	  strcpy(ProcessorName, "Transmeta");
   else if (!strcmp(VendorId,  vid_geode))
   	  strcpy(ProcessorName, "Geode");
   else if (!strcmp(VendorId, vid_umc))
   	  strcpy(ProcessorName, "UMC");
   else if (!strcmp(VendorId, vid_amd2))
   	  strcpy(ProcessorName, "AMD");
   else if (!strcmp(VendorId, vid_amd3))
   	  strcpy(ProcessorName, "AMD");
   else if (!strcmp(VendorId, vid_amd4))
   	  strcpy(ProcessorName, "AMD");
   else if (!strcmp(VendorId, vid_nexgen))
   	  strcpy(ProcessorName, "NexGen");
   else 
   	  strcpy(ProcessorName, "Unknown");

   return TRUE;
}	


/*
Platform SDK: Windows System Information 

Adapted from:
http://msdn.microsoft.com/library/en-us/sysinfo/base/getting_the_system_version.asp
http://www.codeproject.com/system/winvertable.asp

Getting the System Version

The following example uses the GetVersionEx function to display the
version of the currently running operating system. 

Relying on version information is not the best way to test for a feature.
Instead, refer to the documentation for the feature of interest.
For more information on common techniques for feature detection, see
Operating System Version. 

If you must require a particular operating system, be sure to use it
as a minimum supported version, rather than design the test for the
one operating system. This way, your detection code will continue to
work on future versions of Windows. 
*/

#undef BUFSIZE
#define BUFSIZE 255

BOOL GetOsInfo(LPSTR OsName, LPSTR Release, LPSTR Version)
{
   OSVERSIONINFOEX osvi;
   BOOL bOsVersionInfoEx;
   DWORD BuildNumber;

   // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
   // If that fails, try using the OSVERSIONINFO structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   if( !(bOsVersionInfoEx = GetVersionEx ((LPOSVERSIONINFO) &osvi)) ) {
      // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionEx ( (LPOSVERSIONINFO) &osvi) ) 
         return FALSE;
   }

   BuildNumber = osvi.dwBuildNumber & 0xFFFF;
   switch (osvi.dwPlatformId) {
      // Tests for Windows NT product family.
      case VER_PLATFORM_WIN32_NT:
      // Test for the product.
		 if ( osvi.dwMajorVersion == 3 && osvi.dwMinorVersion == 51 ) 
            strcpy(OsName, "Microsoft Windows NT 3.51");
		 else if ( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0 ) 
            strcpy(OsName, "Microsoft Windows NT 4.0");
		 else if ( osvi.dwMajorVersion <= 4 && osvi.dwMinorVersion == 0 ) 
            strcpy(OsName, "Microsoft Windows NT");
		 else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
            strcpy(OsName, "Microsoft Windows Server&nbsp;2003 family, ");
         else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
            strcpy(OsName, "Microsoft Windows XP");
		 else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
            strcpy(OsName, "Microsoft Windows 2000");


         if( bOsVersionInfoEx ) { // Use information from GetVersionEx.
         // Test for the workstation type.
            if ( osvi.wProductType == VER_NT_WORKSTATION ) {
               if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
                  strcat(OsName,  " Home Edition" );
               else
                  strcat(OsName,  " Professional" );
            }
         // Test for the server type.
            else if ( osvi.wProductType == VER_NT_SERVER) {
				if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2) {
				   if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
        	          strcat(OsName,  " Datacenter Edition" );
            	   else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                	  if( osvi.dwMajorVersion == 4 )
                    	 strcat(OsName, " Advanced Server" );
	                  else
    	                 strcat(OsName,  " Enterprise Edition" );
        	       else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
            	      strcat(OsName,  " Web Edition" );
	               else
    	              strcat(OsName,  " Standard Edition" );
        	    }
	         else if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )  {
                  if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                     strcat(OsName, " Datacenter Server" );
                  else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     strcat(OsName, " Advanced Server" );
                  else
                     strcat(OsName, " Server" );
               }
             else  // Windows NT 4.0 {
                  if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
		     strcat(OsName, "Server 4.0, Enterprise Edition ");
                  else
                     strcat(OsName, "Server 4.0 " );
               }
		 }
		 else  { // Use the registry on early versions of Windows NT.
            HKEY hKey;
            char szProductType[BUFSIZE];
            DWORD dwBufLen=BUFSIZE;
            LONG lRet;

            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
               "SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
               0, KEY_QUERY_VALUE, &hKey );
            if( lRet != ERROR_SUCCESS )
                return FALSE;

            lRet = RegQueryValueEx( hKey, "ProductType", NULL, NULL,
               (LPBYTE) szProductType, &dwBufLen);
            if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE) )
                return FALSE;

            RegCloseKey( hKey );

            if ( lstrcmpi( "WINNT", szProductType) == 0 )
               strcat(OsName,  " Professional" );
            if ( lstrcmpi( "LANMANNT", szProductType) == 0 )
               strcat(OsName,  " Server" );
            if ( lstrcmpi( "SERVERNT", szProductType) == 0 )
               strcat(OsName,  " Advanced Server" );
         }
	  // Display version, service pack (if any), and build number.
			 strcat (OsName, " ");
			 strcat (OsName, osvi.szCSDVersion);
         break;
      // Test for the Windows 95 product family.
      case VER_PLATFORM_WIN32_WINDOWS:
		 if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) {
             strcpy(OsName, "Microsoft Windows 95");
             if (BuildNumber > 950 && BuildNumber <= 1080)
			 	strcat(OsName, " SP1");
             else if (BuildNumber > 1080)
			 	strcat(OsName, " OSR2");
/*
			 if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
                strcat(OsName, "OSR2 " );
*/
		 } 

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10) {
             strcpy(OsName, "Microsoft Windows 98");
             if (BuildNumber > 1998 && BuildNumber < 2183)
			 	strcat(OsName, " SP1");
             else if (BuildNumber >= 2183)
			 	strcat(OsName, " SE");
/*
             if ( osvi.szCSDVersion[1] == 'A' )
                strcat(OsName, "SE " );
*/
		 } 

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) {
             strcpy(OsName, "Microsoft Windows Millennium Edition ");
         } 
		 break;
   }
	sprintf(Release, "%lu.%lu.%lu", osvi.dwPlatformId,
	   osvi.dwMajorVersion,
       osvi.dwMinorVersion);
	sprintf(Version, "%lu",
       osvi.dwBuildNumber & 0xFFFF);
   return TRUE; 
}
