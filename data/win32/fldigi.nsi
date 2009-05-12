# -*- conf -*-

# NSIS installer script based on example2.nsi from the nsis-2.44 distribution.
# Copyright (c) 2009 Stelios Bounanos, M0GLD.


# Compression options
SetCompressor /SOLID lzma

# The name of the installer
Name "${PROGRAM_NAME} ${PROGRAM_VERSION}"

# The file to write
OutFile ${INSTALLER_FILE}

# The default installation directory
InstallDir $PROGRAMFILES\${PROGRAM_NAME}-${PROGRAM_VERSION}

# Registry key to check for directory (so if you install again, it will
# overwrite the old one automatically)
!define INSTALL_DIR_REG_KEY SOFTWARE\${PROGRAM_NAME}-${PROGRAM_VERSION}
InstallDirRegKey HKLM "${INSTALL_DIR_REG_KEY}" "Install_Dir"

# Request application privileges for Windows Vista
RequestExecutionLevel admin

# License
LicenseText "${PROGRAM_NAME} is distributed under the GNU GPL as detailed \
below. You must abide by the terms of this license if you modify or \
redistribute the program." "Continue"
LicenseData "${LICENSE_FILE}"
SubCaption 0 ": License Information"

# Other options
BrandingText " "
InstProgressFlags smooth
VIAddVersionKey ProductName "${PROGRAM_NAME}"
VIAddVersionKey ProductVersion "${PROGRAM_VERSION}"
VIAddVersionKey FileVersion "${PROGRAM_VERSION}"
VIAddVersionKey FileDescription "${PROGRAM_NAME} ${PROGRAM_VERSION} installer"
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
!define REG_UNINSTALL_PATH Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}-${PROGRAM_VERSION}

# The stuff to install
Section "${PROGRAM_NAME}"
    SectionIn RO
    # Set output path to the installation directory.
    SetOutPath $INSTDIR
    # List files to be installed here
    File "${BINARY}"
    # Write the installation path into the registry
    WriteRegStr HKLM "${INSTALL_DIR_REG_KEY}" "Install_Dir" "$INSTDIR"

    # Write the uninstall keys for Windows
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "DisplayName" "${PROGRAM_NAME} ${PROGRAM_VERSION}"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "DisplayVersion" "${PROGRAM_VERSION}"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "HelpLink" "${SUPPORT_URL}"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "Publisher" "Fldigi developers"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "URLUpdateInfo" "${UPDATES_URL}"
    WriteRegStr HKLM "${REG_UNINSTALL_PATH}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "${REG_UNINSTALL_PATH}" "NoModify" 1
    WriteRegDWORD HKLM "${REG_UNINSTALL_PATH}" "NoRepair" 1
    WriteUninstaller "uninstall.exe"
SectionEnd

# Start Menu path
!define SM_PATH_BASE $SMPROGRAMS\${PROGRAM_NAME}
!define SM_PATH ${SM_PATH_BASE}\${PROGRAM_NAME}-${PROGRAM_VERSION}

# The following sections are optional
Section "Start Menu Shortcuts"
    CreateDirectory "${SM_PATH}"
    CreateShortCut "${SM_PATH}\${PROGRAM_NAME}.lnk" "$INSTDIR\${BINARY}" "" "$INSTDIR\${BINARY}" 0
    CreateShortCut "${SM_PATH}\Beginners' Guide.lnk" "${GUIDE_URL}"
    CreateShortCut "${SM_PATH}\Documentation.lnk" "${DOCS_URL}"
    CreateShortCut "${SM_PATH}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Desktop Shortcut"
    CreateShortCut "$DESKTOP\${PROGRAM_NAME} ${PROGRAM_VERSION}.lnk" "$INSTDIR\${BINARY}" "" "$INSTDIR\${BINARY}" 0
SectionEnd

# This is unselected by default
Section /o "Quick Launch Shortcut"
    CreateShortCut "$QUICKLAUNCH\${PROGRAM_NAME} ${PROGRAM_VERSION}.lnk" "$INSTDIR\${BINARY}" "" "$INSTDIR\${BINARY}" 0
SectionEnd

# Uninstaller
Section "Uninstall"
    # Remove registry keys
    DeleteRegKey HKLM "${REG_UNINSTALL_PATH}"
    DeleteRegKey HKLM "${INSTALL_DIR_REG_KEY}"

    # Remove files and uninstaller
    Delete $INSTDIR\${BINARY}
    Delete $INSTDIR\uninstall.exe

    # Remove shortcuts, if any
    Delete "${SM_PATH}\*.*"
    Delete "$DESKTOP\${PROGRAM_NAME} ${PROGRAM_VERSION}.lnk"
    Delete "$QUICKLAUNCH\${PROGRAM_NAME} ${PROGRAM_VERSION}.lnk"

    # Remove directories used
    RMDir "${SM_PATH}"
    RMDir "${SM_PATH_BASE}"
    RMDir "$INSTDIR"
SectionEnd
