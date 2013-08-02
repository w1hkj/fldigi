# -*- conf -*-

# NSIS installer script based on example2.nsi from the nsis-2.44 distribution.
# Copyright (c) 2009 Stelios Bounanos, M0GLD.
# License: GPLv3+: GNU GPL version 3 or later.


# Variables
!define FLDIGI_DESCRIPTION "${FLDIGI_NAME} ${FLDIGI_VERSION}"
!define FLDIGI_STRING "${FLDIGI_NAME}-${FLDIGI_VERSION}"
!define FLARQ_DESCRIPTION "${FLARQ_NAME} ${FLARQ_VERSION}"
!define FLARQ_STRING "${FLARQ_NAME}-${FLARQ_VERSION}"
!ifdef HAVE_FLDIGI
    !define PRODUCT_BINARY "${FLDIGI_BINARY}"
    !define PRODUCT_NAME "${FLDIGI_NAME}"
    !define PRODUCT_VERSION "${FLDIGI_VERSION}"
    !define PRODUCT_STRING "${FLDIGI_STRING}"
    !define PRODUCT_DESCRIPTION "${FLDIGI_DESCRIPTION}"
!else ifdef HAVE_FLARQ
    !define PRODUCT_BINARY "${FLARQ_BINARY}"
    !define PRODUCT_NAME "${FLARQ_NAME}"
    !define PRODUCT_VERSION "${FLARQ_VERSION}"
    !define PRODUCT_STRING "${FLARQ_STRING}"
    !define PRODUCT_DESCRIPTION "${FLARQ_DESCRIPTION}"
!else
    !error "Either HAVE_FLDIGI or HAVE_FLARQ must be defined"
!endif

# Compression options
SetCompressor /SOLID lzma

# This function is called before displaying the first installer page.
# It aborts the installation if the Windows version is too old.
!include WinVer.nsh
Function .onInit
    ${IfNot} ${AtLeastWin2000}
        MessageBox MB_ICONSTOP "Sorry, your Windows version is too old.$\n${PRODUCT_NAME} requires Windows 2000 or later."
	Abort
    ${EndIf}
FunctionEnd

# The name of the installer
Name "${PRODUCT_DESCRIPTION}"

# The file to write
OutFile ${INSTALLER_FILE}

# The default installation directory
InstallDir $PROGRAMFILES\${PRODUCT_STRING}

# Registry key to check for directory (so if you install again, it will
# overwrite the old one automatically)
!define INSTALL_DIR_REG_KEY SOFTWARE\${PRODUCT_STRING}
InstallDirRegKey HKLM "${INSTALL_DIR_REG_KEY}" "Install_Dir"

# Request application privileges for Windows Vista
RequestExecutionLevel admin

# License
LicenseText "${PRODUCT_NAME} is distributed under the GNU GPL as detailed \
below. You must abide by the terms of this license if you modify or \
redistribute the program." "Continue"
LicenseData "${LICENSE_FILE}"
SubCaption 0 ": License Information"

# Other options
BrandingText " "
InstProgressFlags smooth
VIAddVersionKey ProductName "${PRODUCT_NAME}"
VIAddVersionKey ProductVersion "${PRODUCT_VERSION}"
VIAddVersionKey FileVersion "${PRODUCT_VERSION}"
VIAddVersionKey FileDescription "${FLDIGI_DESCRIPTION} installer"
VIAddVersionKey LegalCopyright "Fldigi developers"
VIAddVersionKey OriginalFilename "${INSTALLER_FILE}"
VIProductVersion "3.0.0.0"
WindowIcon off
XPStyle on

# Installer pages
Page license
Page components
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

# Registry uninstall path
!define REG_UNINSTALL_PATH Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_STRING}

# This is a hidden section and is always selected.  It writes the uninstall
# registry keys and uninstaller binary.
Section -install
    # Set output path to the installation directory.
    SetOutPath $INSTDIR
    # Write the installation paths into the registry
    WriteRegStr HKLM "${INSTALL_DIR_REG_KEY}" "Install_Dir" "$INSTDIR"
    # Write the uninstall keys for Windows
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "DisplayName" "${PRODUCT_DESCRIPTION}"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "DisplayIcon" '"$INSTDIR\${PRODUCT_BINARY}"'
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "HelpLink" "${SUPPORT_URL}"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "Publisher" "Fldigi developers"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "URLUpdateInfo" "${UPDATES_URL}"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
    WriteRegDWORD HKLM "${REG_UNINSTALL_PATH}" "NoModify" 1
    WriteRegDWORD HKLM "${REG_UNINSTALL_PATH}" "NoRepair" 1
    WriteUninstaller "uninstall.exe"
SectionEnd

!ifdef HAVE_FLDIGI
    Var WANT_FLDIGI
!endif
!ifdef HAVE_FLARQ
    Var WANT_FLARQ
!endif

# This section is present (and required) if the installer contains fldigi.
!ifdef HAVE_FLDIGI
    Section "Fldigi"
        SectionIn RO
        SetOutPath $INSTDIR
        File "${FLDIGI_BINARY}"
        !ifdef FLDIGI_LOCALE_DIR
	    File /r "${FLDIGI_LOCALE_PATH}/${FLDIGI_LOCALE_DIR}"
        !endif
        StrCpy $WANT_FLDIGI "true"
    SectionEnd
!endif

# This section is present if the installer contains flarq.  It is optional if
# the installer also contains fldigi.
!ifdef HAVE_FLARQ
    Section "Flarq"
        !ifndef HAVE_FLDIGI
            SectionIn RO
        !endif
        SetOutPath $INSTDIR
        File "${FLARQ_BINARY}"
        StrCpy $WANT_FLARQ "true"
    SectionEnd
!endif

# Start Menu path
!define SM_PATH_BASE $SMPROGRAMS\${PRODUCT_NAME}
!define SM_PATH ${SM_PATH_BASE}\${PRODUCT_STRING}

# The following sections are optional
Section "Start Menu Shortcuts"
    CreateDirectory "${SM_PATH}"
    !ifdef HAVE_FLDIGI
        ${If} $WANT_FLDIGI == 'true'
            CreateShortCut "${SM_PATH}\${FLDIGI_NAME}.lnk" "$INSTDIR\${FLDIGI_BINARY}" "" "$INSTDIR\${FLDIGI_BINARY}" 0
            CreateShortCut "${SM_PATH}\${FLDIGI_NAME} Beginners' Guide.lnk" "${GUIDE_URL}"
            CreateShortCut "${SM_PATH}\${FLDIGI_NAME} Documentation.lnk" "${FLDIGI_DOCS_URL}"
        ${EndIf}
    !endif
    !ifdef HAVE_FLARQ
        ${If} $WANT_FLARQ == 'true'
            CreateShortCut "${SM_PATH}\${FLARQ_NAME}.lnk" "$INSTDIR\${FLARQ_BINARY}" "" "$INSTDIR\${FLARQ_BINARY}" 0
            CreateShortCut "${SM_PATH}\${FLARQ_NAME} Documentation.lnk" "${FLARQ_DOCS_URL}"
        ${EndIf}
    !endif
    CreateShortCut "${SM_PATH}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Desktop Shortcuts"
    !ifdef HAVE_FLDIGI
        ${If} $WANT_FLDIGI == 'true'
            CreateShortCut "$DESKTOP\${FLDIGI_DESCRIPTION}.lnk" "$INSTDIR\${FLDIGI_BINARY}" "" \
                           "$INSTDIR\${FLDIGI_BINARY}" 0
        ${EndIf}
    !endif
    !ifdef HAVE_FLARQ
        ${If} $WANT_FLARQ == 'true'
            CreateShortCut "$DESKTOP\${FLARQ_DESCRIPTION}.lnk" "$INSTDIR\${FLARQ_BINARY}" "" \
                           "$INSTDIR\${FLARQ_BINARY}" 0
        ${EndIf}
    !endif
SectionEnd

# This is unselected by default
Section /o "Quick Launch Shortcuts"
    !ifdef HAVE_FLDIGI
        ${If} $WANT_FLDIGI == 'true'
            CreateShortCut "$QUICKLAUNCH\${FLDIGI_DESCRIPTION}.lnk" "$INSTDIR\${FLDIGI_BINARY}" "" \
                           "$INSTDIR\${FLDIGI_BINARY}" 0
        ${EndIf}
    !endif
    !ifdef HAVE_FLARQ
        ${If} $WANT_FLARQ == 'true'
            CreateShortCut "$QUICKLAUNCH\${FLARQ_DESCRIPTION}.lnk" "$INSTDIR\${FLARQ_BINARY}" "" \
                           "$INSTDIR\${FLARQ_BINARY}" 0
        ${EndIf}
    !endif
SectionEnd

# Uninstaller
Section "Uninstall"
    # Remove registry keys
    DeleteRegKey HKLM "${REG_UNINSTALL_PATH}"
    DeleteRegKey HKLM "${INSTALL_DIR_REG_KEY}"

    # Remove files and uninstaller
    !ifdef HAVE_FLDIGI
        Delete /REBOOTOK $INSTDIR\${FLDIGI_BINARY}
        !ifdef FLDIGI_LOCALE_DIR
            RMDir /r /REBOOTOK $INSTDIR\${FLDIGI_LOCALE_DIR}
        !endif
    !endif
    !ifdef HAVE_FLARQ
        Delete /REBOOTOK $INSTDIR\${FLARQ_BINARY}
    !endif
    Delete /REBOOTOK $INSTDIR\uninstall.exe

    # Remove shortcuts, if any
    Delete "${SM_PATH}\*.*"
    !ifdef HAVE_FLDIGI
        Delete "$DESKTOP\${FLDIGI_DESCRIPTION}.lnk"
        Delete "$QUICKLAUNCH\${FLDIGI_DESCRIPTION}.lnk"
    !endif
    !ifdef HAVE_FLARQ
        Delete "$DESKTOP\${FLARQ_DESCRIPTION}.lnk"
        Delete "$QUICKLAUNCH\${FLARQ_DESCRIPTION}.lnk"
    !endif

    # Remove directories used
    RMDir "${SM_PATH}"
    RMDir "${SM_PATH_BASE}"
    RMDir "$INSTDIR"
SectionEnd

# Offer to reboot the machine if the reboot flag is nonzero. This flag is set by
# commands that specify the /REBOOTOK switch if the BINARY_* files were in use
# during uninstallation. Stupid Windows.
Function un.onGUIEnd
    IfRebootFlag 0 noreboot
    MessageBox MB_YESNO|MB_ICONQUESTION \
               "A reboot is required to finish removing ${PRODUCT_NAME}. Do you wish to reboot now?" IDNO noreboot
    Reboot
    noreboot:
FunctionEnd

# Tell the user if we could not reboot for some reason.
Function un.onRebootFailed
    MessageBox MB_OK|MB_ICONSTOP "Reboot failed. Please reboot manually." /SD IDOK
FunctionEnd
