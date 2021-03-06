;
; additional uninstaller instructions
;

; Remove file association xtc
  !define Index "Line${__LINE__}"
  ReadRegStr $1 HKCR ".xtc" ""
  StrCmp $1 "XTrackCAD.Design" 0 "${Index}-NoOwn" ; only do this if we own it
    ReadRegStr $1 HKCR ".xtc" "backup_val"
    StrCmp $1 "" 0 "${Index}-Restore" ; if backup="" then delete the whole key
      DeleteRegKey HKCR ".xtc"
      Goto "${Index}-NoOwn"
    "${Index}-Restore:"
      WriteRegStr HKCR ".xtc" "" $1
      DeleteRegValue HKCR ".xtc" "backup_val"

"${Index}-NoOwn:"
; Remove file association xtce
  !define /redef Index "Line${__LINE__}"
  ReadRegStr $1 HKCR ".xtce" ""
  StrCmp $1 "XTrackCAD.Design" 0 "${Index}-NoOwn" ; only do this if we own it
    ReadRegStr $1 HKCR ".xtce" "backup_xtce"
    StrCmp $1 "" 0 "${Index}-Restore" ; if backup="" then delete the whole key
      DeleteRegKey HKCR ".xtce"
      Goto "${Index}-NoOwn"
    "${Index}-Restore:"
      WriteRegStr HKCR ".xtce" "" $1
      DeleteRegValue HKCR ".xtce" "backup_xtce"

  DeleteRegKey HKCR "XTrackCAD.Design" ;Delete key with association settings

  System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
  "${Index}-NoOwn:"
  !undef Index

; Remove shortcuts, if any
SetShellVarContext all

!insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
RMDir /r "$SMPROGRAMS\$MUI_TEMP"
Delete "$desktop\XTrackCAD.lnk"
