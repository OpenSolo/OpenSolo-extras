!define version "0.3.0"
!define app_name "3DRGimbalFactoryApplication"

;Include Modern UI
!include "MUI2.nsh"

Name "3DR Gimbal Factory Application"
OutFile "Install${app_name}_${version}.exe"

; Default installation folder
InstallDir "$PROGRAMFILES\AES\${app_name}"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "SOFTWARE\AES\${app_name}" "Install_Dir"

;Request application privileges for Windows Vista
RequestExecutionLevel admin

XPStyle on

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_WELCOMEPAGE_TITLE "3DR Gimbal Factory Application Installation"
  !define MUI_COMPONENTSPAGE_NODESC

  !define MUI_LICENSEPAGE_CHECKBOX

  !define MUI_FINISHPAGE_RUN "$INSTDIR\${app_name}.exe"
  !define MUI_FINISHPAGE_RUN_TEXT "Run ${app_name}"

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  ;!insertmacro MUI_PAGE_LICENSE "License.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  ;!insertmacro MUI_PAGE_STARTMENU "Application" $StartMenuFolder
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"


Function .onInit

FunctionEnd


Function un.onInit

FunctionEnd


; The stuff to install
Section "3DR Gimbal Factory Application"

    ; Add disk space requirements
    AddSize 37247

    SetOutPath "$INSTDIR"
    File /a "..\..\build-3DRGimbalFactoryApplication-Desktop_Qt_5_4_0_MSVC2010_OpenGL_32bit-Release\release\3DRGimbalFactoryApplication.exe"
    File /a "C:\Qt\5.4\msvc2010_opengl\bin\icudt53.dll"
    File /a "C:\Qt\5.4\msvc2010_opengl\bin\icuin53.dll"
    File /a "C:\Qt\5.4\msvc2010_opengl\bin\icuuc53.dll"
    File /a "C:\Qt\5.4\msvc2010_opengl\bin\Qt5Core.dll"
    File /a "C:\Qt\5.4\msvc2010_opengl\bin\Qt5Gui.dll"
    File /a "C:\Qt\5.4\msvc2010_opengl\bin\Qt5SerialPort.dll"
    File /a "C:\Qt\5.4\msvc2010_opengl\bin\Qt5Widgets.dll"
    File /a "vcredist_x86.exe"
    SetOutPath "$INSTDIR\platforms"
    File /a "C:\Qt\5.4\msvc2010_opengl\plugins\platforms\qwindows.dll"

    ; Write the installation path and version into the registry
    WriteRegStr HKLM "SOFTWARE\AES\${app_name}" "Install_Dir" "$INSTDIR"
    WriteRegStr HKLM "SOFTWARE\AES\${app_name}" "Version"     "${version}"
    
    ; Write the uninstall keys for Windows
    WriteRegStr HKLM   "Software\Microsoft\Windows\CurrentVersion\Uninstall\${app_name}" "DisplayName" "${app_name}"
    WriteRegStr HKLM   "Software\Microsoft\Windows\CurrentVersion\Uninstall\${app_name}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${app_name}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${app_name}" "NoRepair" 1

    WriteUninstaller "$INSTDIR\uninstall.exe"
    
    ; Now try to install the visual studio 2010 redistributable packages
    ReadRegDWORD $0 HKLM "Software\Microsoft\VisualStudio\10.0\VC\VCRedist\x86" "Installed"
    IntCmp $0 1 skipVCRedist 0 0
        ; The VS2010 redistributable package isn't installed.  Ask the user if they want to install it
        MessageBox MB_YESNO "The Visual Studio 2010 redistributable packages do not seem to be installed.  If they are not installed, this application will not be able to run.  Would you like to install them now?" IDNO skipVCRedist
    
        ExecWait "$INSTDIR\vcredist_x86.exe" $1
        
        IntCmp $1 0 skipVCRedist 0 0
            MessageBox MB_OK "The Visual Studio 2010 redistributable packages did not install correctly.  They are needed for the application to work.  Please install them separately for the application to work"
    
    skipVCRedist:

SectionEnd ; end the section


; add shortcuts
Section "Shortcuts"

    CreateDirectory "$SMPROGRAMS\${app_name}"

    CreateShortCut "$DESKTOP\${app_name}.lnk" "$INSTDIR\${app_name}.exe"
    CreateShortCut "$SMPROGRAMS\${app_name}\${app_name}.lnk" "$INSTDIR\${app_name}.exe"
    CreateShortCut "$SMPROGRAMS\${app_name}\Uninstall ${app_name}.lnk" "$INSTDIR\uninstall.exe"

SectionEnd

; Uninstaller
Section "Uninstall"

    DeleteRegKey HKLM "SOFTWARE\AES\${app_name}"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${app_name}"

    Delete "$DESKTOP\${app_name}.lnk"

    RMDir /r "$SMPROGRAMS\${app_name}"
    RMDir /r /REBOOTOK "$INSTDIR"

SectionEnd
