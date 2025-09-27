; Coral Cryptocurrency Windows Installer
!define APPNAME "Coral Cryptocurrency"
!define APPVERSION "1.0.0"
!define APPDIR "Coral"

Name "${APPNAME}"
OutFile "Coral-${APPVERSION}-Windows-Setup.exe"
InstallDir "$PROGRAMFILES64\${APPDIR}"

Page directory
Page instfiles

Section "Install"
    SetOutPath $INSTDIR
    File "bin\coral-test.exe"
    File "bin\coral-wallet.bat"
    File "bin\coral-mine.bat"

    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Coral Wallet.lnk" "$INSTDIR\coral-wallet.bat"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Coral Mining.lnk" "$INSTDIR\coral-mine.bat"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"

    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\coral-test.exe"
    Delete "$INSTDIR\coral-wallet.bat"
    Delete "$INSTDIR\coral-mine.bat"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"

    Delete "$SMPROGRAMS\${APPNAME}\Coral Wallet.lnk"
    Delete "$SMPROGRAMS\${APPNAME}\Coral Mining.lnk"
    Delete "$SMPROGRAMS\${APPNAME}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${APPNAME}"
SectionEnd
