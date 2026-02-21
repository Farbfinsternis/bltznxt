;
; BLITZ SYS INCLUDE - V1.04
; -------------------------------------------------------------------------------------------------
; Simple OS Function Support! - Written by Rob Hutchinson & Joseph Cox 2001
; For updates and reference, visit... http://www.loki-sd.pwp.blueyonder.co.uk/
; For help and support, email... loki.sd@blueyonder.co.uk
;
; Thanks to RobK for helping me with some C stuff when I got stuck. MUCH appreciated dude!!
;

Global sBlitzSysDLLCommandPrefixA101B6$ = "_"
Global sBlitzSysDLLNameA101B6$ = "blitzsys"
Global sBlitzSysDLLVersionA101B6$ = "1.04"
Global bChooseColorFailedFlagA101B6 = 0
Global iBlitzSysLastErrorNumberA101B6 = 0
Global sBlitzSysLastErrorDescriptionA101B6$ = ""

Function DLLBlitzSysInitialise()
	Return CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "InitialiseDLL")
End Function

Function DLLWritePrivateProfileString%(sFileName$,sSection$,sKey$,sValue$)
	Local iBankSize,mBankIn,iResult,iBankOffset
	
	; Gather the size of all 4 parameters + size values.
	iBankSize = Len(sFileName$) + Len(sSection$) + Len(sKey$) + Len(sValue$) + 4
	
	mBankIn  = CreateBank(iBankSize)
	PokeString(mBankIn,sFileName$,0)
	iBankOffset = Len(sFileName$) + 1
	PokeString(mBankIn,sSection$,iBankOffset)
	iBankOffset = iBankOffset + Len(sSection$) + 1
	PokeString(mBankIn,sKey$,iBankOffset)
	iBankOffset = iBankOffset + Len(sKey$) + 1
	PokeString(mBankIn,sValue$,iBankOffset)
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "WPPSWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

Function DLLGetPrivateProfileString$(sFileName$,sSection$,sKey$,sDefault$,iOutBufferSize = 1000)
	Local iBankSize,mBankIn,mBankOut,iResult,iBankOffset,sResult$
	
	; Gather the size of all 4 parameters + size values.
	iBankSize = Len(sFileName$) + Len(sSection$) + Len(sKey$) + Len(sDefault$) + 4
	
	mBankIn  = CreateBank(iBankSize)
	mBankOut = CreateBank(iOutBufferSize)

	PokeString(mBankIn,sFileName$,0)
	iBankOffset = Len(sFileName$) + 1
	PokeString(mBankIn,sSection$,iBankOffset)
	iBankOffset = iBankOffset + Len(sSection$) + 1
	PokeString(mBankIn,sKey$,iBankOffset)
	iBankOffset = iBankOffset + Len(sKey$) + 1
	PokeString(mBankIn,sDefault$,iBankOffset)
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GPPSWrapper",mBankIn,mBankOut)
	
	; Pull the resulting string out of the bank...
	If iResult <> 0
		sResult$ = PeekString$(mBankOut,0)
	Else
		sResult$ = sDefault$
	EndIf
	
	FreeBank mBankIn
	FreeBank mBankOut
	Return sResult$
End Function

; Constants for MessageBox()
Const MB_USERICON             = 128
Const MB_ICONASTERISK         = 64
Const MB_ICONEXCLAMATION      = $30
Const MB_ICONWARNING          = $30
Const MB_ICONERROR            = 16
Const MB_ICONHAND             = 16
Const MB_ICONQUESTION         = 32
Const MB_OK                   = 0
Const MB_ABORTRETRYIGNORE     = 2
Const MB_APPLMODAL            = 0
Const MB_DEFAULT_DESKTOP_ONLY = $20000
Const MB_HELP                 = $4000
Const MB_RIGHT                = $80000
Const MB_RTLREADING           = $100000
Const MB_TOPMOST              = $40000
Const MB_DEFBUTTON1           = 0
Const MB_DEFBUTTON2           = 256
Const MB_DEFBUTTON3	          = 512
Const MB_DEFBUTTON4           = $300
Const MB_ICONINFORMATION      = 64
Const MB_ICONSTOP             = 16
Const MB_OKCANCEL             = 1
Const MB_RETRYCANCEL          = $5
Const MB_SERVICE_NOTIFICATION = $40000
Const MB_SETFOREGROUND        = $10000
Const MB_SYSTEMMODAL          = 4096
Const MB_TASKMODAL            = $2000
Const MB_YESNO                = 4
Const MB_YESNOCANCEL          = 3

; Return values for MessageBox()
Const IDABORT  = 3
Const IDCANCEL = 2
Const IDCLOSE  = 8
Const IDHELP   = 9
Const IDIGNORE = 5
Const IDNO     = 7
Const IDOK     = 1
Const IDRETRY  = 4
Const IDYES    = 6


Function DLLMessageBox(sCaption$,sContents$,iFlags = MB_TOPMOST)
	Local iBankSize,mBankIn,iResult,iBankOffset
	
	; Gather the size of all 4 parameters + size values.
	iBankSize = Len(sCaption$) + Len(sContents$) + 2 + 4
	
	mBankIn  = CreateBank(iBankSize)

	PokeString(mBankIn,sCaption$,0)
	iBankOffset = Len(sCaption$) + 1
	PokeString(mBankIn,sContents$,iBankOffset)
	iBankOffset = iBankOffset + Len(sContents$) + 1
	PokeInt mBankIn,iBankOffset,iFlags
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "MsgBoxWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

Function DLLMessageBeep(iFlags = $FFFFFFFF)
	Local iBankSize,mBankIn,iResult,iBankOffset
	
	; Gather the size of all 4 parameters + size values.
	mBankIn  = CreateBank(4)
	PokeInt mBankIn,0,iFlags
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "MsgBeepWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

Function DLLFindWindow(sCaption$)
	Local iBankSize,mBankIn,iResult,iBankOffset
	
	; Gather the size of all 4 parameters + size values.
	iBankSize = Len(sCaption$) + 1
	
	mBankIn  = CreateBank(iBankSize)

	PokeString(mBankIn,sCaption$,0)
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "FindWindowWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

Function DLLPositionWindow(hwndHandle,iNewX,iNewY)
	Local iBankSize,mBankIn,iResult,iBankOffset
	
	mBankIn  = CreateBank(12)
	PokeInt mBankIn,0,hwndHandle
	PokeInt mBankIn,4,iNewX
	PokeInt mBankIn,8,iNewY
		
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "PositionWindowWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

; SetWindowPos() definitions..
Const HWND_BOTTOM        = 1
Const HWND_NOTOPMOST     = -2
Const HWND_TOP           = 0
Const HWND_TOPMOST       = -1

Const SWP_DRAWFRAME      = 32
Const SWP_FRAMECHANGED   = 32
Const SWP_HIDEWINDOW     = 128
Const SWP_NOACTIVATE     = 16
Const SWP_NOCOPYBITS     = 256
Const SWP_NOMOVE         = 2
Const SWP_NOSIZE         = 1
Const SWP_NOREDRAW       = 8
Const SWP_NOZORDER       = 4
Const SWP_SHOWWINDOW     = 64
Const SWP_NOOWNERZORDER  = 512
Const SWP_NOREPOSITION   = 512
Const SWP_NOSENDCHANGING = 1024

Function DLLSetWindowPos(hwndHandle,hWndInsertAfter,iNewX,iNewY,iNewW,iNewH,iFlags = 0)
	Local mBankIn,iResult
	
	mBankIn  = CreateBank(7 * 4)
	PokeInt mBankIn,0,hwndHandle
	PokeInt mBankIn,4,hWndInsertAfter
	PokeInt mBankIn,8,iNewX
	PokeInt mBankIn,12,iNewY
	PokeInt mBankIn,16,iNewW
	PokeInt mBankIn,20,iNewH
	PokeInt mBankIn,24,iFlags
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "SetWindowPosWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

Function DLLGetWindowX(hwndHandle)
	Local mBankIn,iResult
	
	mBankIn  = CreateBank(4)
	PokeInt mBankIn,0,hwndHandle
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetWindowXWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

Function DLLGetWindowY(hwndHandle)
	Local mBankIn,iResult
	
	mBankIn  = CreateBank(4)
	PokeInt mBankIn,0,hwndHandle
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetWindowYWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

Function DLLGetWindowWidth(hwndHandle)
	Local mBankIn,iResult
	
	mBankIn  = CreateBank(4)
	PokeInt mBankIn,0,hwndHandle
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetWindowWidthWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

Function DLLGetWindowHeight(hwndHandle)
	Local mBankIn,iResult
	
	mBankIn  = CreateBank(4)
	PokeInt mBankIn,0,hwndHandle
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetWindowHeightWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

; Constants for ShowWindow()
Const SW_HIDE            = 0
Const SW_NORMAL          = 1
Const SW_MAXIMIZE        = 3
Const SW_MINIMIZE        = 6
Const SW_RESTORE         = 9
Const SW_SHOW            = 5
Const SW_SHOWDEFAULT     = 10
Const SW_SHOWMAXIMIZED   = 3
Const SW_SHOWMINIMIZED   = 2
Const SW_SHOWMINNOACTIVE = 7
Const SW_SHOWNA          = 8
Const SW_SHOWNOACTIVATE  = 4
Const SW_SHOWNORMAL      = 1

Function DLLShowWindow(hwndHandle,iFlags)
	Local iBankSize,mBankIn,iResult
	
	mBankIn  = CreateBank(8)
	PokeInt mBankIn,0,hwndHandle
	PokeInt mBankIn,4,iFlags
		
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "ShowWindowWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

; GetOpenFileName consts (useful ones only!)...
Const OFN_ALLOWMULTISELECT     = 512      ; Allows you to multiselect files (not implemented in BlitzSys properly YET!)
Const OFN_CREATEPROMPT         = $2000    ; Prompts the user as to whether they want to create a file that doesnt exist.
Const OFN_FILEMUSTEXIST        = $1000    ; File must exist for it to be returned.
Const OFN_HIDEREADONLY         = 4        ; Hides the read only button in the dialog...
Const OFN_NOCHANGEDIR          = 8        ; Stops the user from changing the initial directory.
Const OFN_NONETWORKBUTTON      = $20000   ; Hides and disables the network button.
Const OFN_NOREADONLYRETURN     = $8000    ; Stops the requester returning readonly files..
Const OFN_NOVALIDATE           = 256      ; If selected, no check will be done for invalid characters.
Const OFN_OVERWRITEPROMPT      = 2        ; Prompt for overwrite file...
Const OFN_PATHMUSTEXIST        = $800     ; Specifies that the path MUST exist for it to be able to be selected.
Const OFN_READONLY             = 1        ; Makes the read only checkbox in the dialog box to be checked immediately.

Function DLLGetOpenFileName$(sTitle$,sInitialDir$,sFilter$,iFlags = 0,iOutBufferSize = 512)
	Local iBankSize,mBankIn,mBankOut,iResult,sResult$
	
	iBankSize = Len(sTitle$) + Len(sInitialDir$) + Len(sFilter$) + 4 + 4
	
	mBankIn  = CreateBank(iBankSize)
	mBankOut = CreateBank(iOutBufferSize)

	PokeInt(mBankIn,0,iFlags)
	PokeString(mBankIn,sInitialDir$,4)
	iBankOffset = Len(sInitialDir$) + 1 + 4
	PokeString(mBankIn,sTitle$,iBankOffset)
	iBankOffset = iBankOffset + Len(sTitle$) + 1
	PokeString(mBankIn,sFilter$,iBankOffset)

	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetOpenFileNameWrapper",mBankIn,mBankOut)
	
	If iResult = True
		sResult$ = PeekString$(mBankOut,0)
	EndIf
	
	FreeBank mBankIn
	FreeBank mBankOut
	
	Return sResult$
End Function

Function DLLGetSaveFileName$(sTitle$,sInitialDir$,sFilter$,iFlags = 0,iOutBufferSize = 512)
	Local iBankSize,mBankIn,mBankOut,iResult,sResult$
	
	iBankSize = Len(sTitle$) + Len(sInitialDir$) + Len(sFilter$) + 4 + 4
	
	mBankIn  = CreateBank(iBankSize)
	mBankOut = CreateBank(iOutBufferSize)

	PokeInt(mBankIn,0,iFlags)
	PokeString(mBankIn,sInitialDir$,4)
	iBankOffset = Len(sInitialDir$) + 1 + 4
	PokeString(mBankIn,sTitle$,iBankOffset)
	iBankOffset = iBankOffset + Len(sTitle$) + 1
	PokeString(mBankIn,sFilter$,iBankOffset)

	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetSaveFileNameWrapper",mBankIn,mBankOut)
	
	If iResult = True
		sResult$ = PeekString$(mBankOut,0)
	EndIf
	
	FreeBank mBankIn
	FreeBank mBankOut
	
	Return sResult$
End Function

;
; These are the constants for the DLLGetSpecialFolder$() command.  Pass
; any one of these in-place of iFlags to return the path of the specified folder on the system.
;

Const CSIDL_DESKTOP                 = 0
Const CSIDL_INTERNET                = 1
Const CSIDL_PROGRAMS                = 2
Const CSIDL_CONTROLS                = 3
Const CSIDL_PRINTERS                = 4
Const CSIDL_PERSONAL                = 5
Const CSIDL_FAVORITES               = 6
Const CSIDL_STARTUP                 = 7
Const CSIDL_RECENT                  = 8
Const CSIDL_SENDTO                  = 9
Const CSIDL_BITBUCKET               = 16
Const CSIDL_STARTMENU               = 17
Const CSIDL_NETWORK                 = 18
Const CSIDL_NETHOOD                 = 19
Const CSIDL_FONTS                   = 20
Const CSIDL_TEMPLATES               = 21
Const CSIDL_COMMON_STARTMENU        = 22
Const CSIDL_COMMON_PROGRAMS         = 23
Const CSIDL_COMMON_STARTUP          = 24
Const CSIDL_COMMON_DESKTOPDIRECTORY = 25
Const CSIDL_APPDATA                 = 26
Const CSIDL_PRINTHOOD               = 27
Const CSIDL_LOCAL_APPDATA           = 28
Const CSIDL_ALTSTARTUP              = 29
Const CSIDL_COMMON_ALTSTARTUP       = 30
Const CSIDL_COMMON_FAVORITES        = 31
Const CSIDL_INTERNET_CACHE          = 32
Const CSIDL_COOKIES                 = 33
Const CSIDL_HISTORY                 = 34
Const CSIDL_COMMON_APPDATA          = 35
Const CSIDL_WINDOWS                 = 36
Const CSIDL_SYSTEM                  = 37
Const CSIDL_PROGRAM_FILES           = 38
Const CSIDL_MYPICTURES              = 39
Const CSIDL_PROFILE                 = 40
Const CSIDL_SYSTEMX86               = 41
Const CSIDL_PROGRAM_FILESX86        = 42
Const CSIDL_PROGRAM_FILES_COMMON    = 43
Const CSIDL_PROGRAM_FILES_COMMONX86 = 44
Const CSIDL_COMMON_TEMPLATES        = 45
Const CSIDL_COMMON_DOCUMENTS        = 46
Const CSIDL_COMMON_ADMINTOOLS       = 47
Const CSIDL_ADMINTOOLS              = 48
Const CSIDL_CONNECTIONS             = 49

Const CSIDL_FLAG_CREATE             = 32768
Const CSIDL_FLAG_DONT_VERIFY        = 16384
Const CSIDL_FLAG_MASK               = 65280

Const CSIDL_DESKTOPDIRECTORY        = $10
Const CSIDL_DRIVES                  = $11

Function DLLGetSpecialFolder$(iFlags,iOutBufferSize = 512)
	Local iBankSize,mBankIn,mBankOut,iResult,sResult$
	
	mBankIn  = CreateBank(4)
	mBankOut = CreateBank(iOutBufferSize)

	PokeInt mBankIn,0,iFlags
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetSpecialFolderWrapper",mBankIn,mBankOut)
	sResult$ = PeekString$(mBankOut,0)

	FreeBank mBankIn
	FreeBank mBankOut
	Return sResult$
End Function

; Post Message Definitions.
; If anyone can be arsed to convert the flags for these calls, please feel free to and email
; them to me :)
Const WM_NULL = 0
Const WM_APP = $8000
Const WM_ACTIVATE = 6
Const WM_ACTIVATEAPP = 28
Const WM_ASKCBFORMATNAME = 780
Const WM_CANCELJOURNAL = 75
Const WM_CANCELMODE = 31
Const WM_CAPTURECHANGED = 533
Const WM_CHANGECBCHAIN = 781
Const WM_CHAR =258
Const WM_CHARTOITEM = 47
Const WM_CHILDACTIVATE = 34
Const WM_CHOOSEFONT_GETLOGFONT = 1025
Const WM_CHOOSEFONT_SETLOGFONT = 1125
Const WM_CHOOSEFONT_SETFLAGS = 1126
Const WM_CLEAR = 771
Const WM_CLOSE = 16
Const WM_COMMAND = 273
Const WM_COMPACTING = 65
Const WM_COMPAREITEM = 57
Const WM_CONTEXTMENU = 123
Const WM_COPY = 769
Const WM_COPYDATA = 74
Const WM_CREATE = 1
Const WM_CTLCOLORBTN = 309
Const WM_CTLCOLORDLG = 310
Const WM_CTLCOLOREDIT = 307
Const WM_CTLCOLORLISTBOX = 308
Const WM_CTLCOLORMSGBOX = 306
Const WM_CTLCOLORSCROLLBAR = 311
Const WM_CTLCOLORSTATIC = 312
Const WM_CUT = 768
Const WM_DEADCHAR = 259
Const WM_DELETEITEM = 45
Const WM_DESTROY = 2
Const WM_DESTROYCLIPBOARD = 775
Const WM_DEVICECHANGE = 537
Const WM_DEVMODECHANGE = 27
Const WM_DISPLAYCHANGE = 126
Const WM_DRAWCLIPBOARD = 776
Const WM_DRAWITEM = 43
Const WM_DROPFILES = 563
Const WM_ENABLE = 10
Const WM_ENDSESSION = 22
Const WM_ENTERIDLE = 289
Const WM_ENTERMENULOOP = 529
Const WM_ENTERSIZEMOVE = 561
Const WM_ERASEBKGND = 20
Const WM_EXITMENULOOP = 530
Const WM_EXITSIZEMOVE = 562
Const WM_FONTCHANGE = 29
Const WM_GETDLGCODE = 135
Const WM_GETFONT = 49
Const WM_GETHOTKEY = 51
Const WM_GETICON = 127
Const WM_GETMINMAXINFO = 36
Const WM_GETTEXT = 13
Const WM_GETTEXTLENGTH = 14
Const WM_HELP = 83
Const WM_HOTKEY = 786
Const WM_HSCROLL = 276
Const WM_HSCROLLCLIPBOARD = 782
Const WM_ICONERASEBKGND = 39
Const WM_IME_CHAR = 646
Const WM_IME_COMPOSITION = 271
Const WM_IME_COMPOSITIONFULL = 644
Const WM_IME_CONTROL = 643
Const WM_IME_ENDCOMPOSITION = 270
Const WM_IME_KEYDOWN = 656
Const WM_IME_KEYUP = 657
Const WM_IME_NOTIFY = 642
Const WM_IME_SELECT = 645
Const WM_IME_SETCONTEXT = 641
Const WM_IME_STARTCOMPOSITION = 269
Const WM_INITDIALOG = 272
Const WM_INITMENU = 278
Const WM_INITMENUPOPUP = 279
Const WM_INPUTLANGCHANGE = 81
Const WM_INPUTLANGCHANGEREQUEST = 80
Const WM_KEYDOWN = 256
Const WM_KEYUP = 257
Const WM_KILLFOCUS = 8
Const WM_LBUTTONDBLCLK = 515
Const WM_LBUTTONDOWN = 513
Const WM_LBUTTONUP = 514
Const WM_MBUTTONDBLCLK = 521
Const WM_MBUTTONDOWN = 519
Const WM_MBUTTONUP = 520
Const WM_MDIACTIVATE = 546
Const WM_MDICASCADE = 551
Const WM_MDICREATE = 544
Const WM_MDIDESTROY = 545
Const WM_MDIGETACTIVE = 553
Const WM_MDIICONARRANGE = 552
Const WM_MDIMAXIMIZE = 549
Const WM_MDINEXT = 548
Const WM_MDIREFRESHMENU = 564
Const WM_MDIRESTORE = 547
Const WM_MDISETMENU = 560
Const WM_MDITILE = 550
Const WM_MEASUREITEM = 44
Const WM_MENUCHAR = 288
Const WM_MENUSELECT = 287
Const WM_MOUSEACTIVATE = 33
Const WM_MOUSEMOVE = 512
Const WM_MOUSEHOVER = $2a1
Const WM_MOUSELEAVE = $2a3
Const WM_MOUSEWHEEL = $20A
Const WM_MOVE = 3
Const WM_MOVING = 534
Const WM_NCACTIVATE = 134
Const WM_NCCALCSIZE = 131
Const WM_NCCREATE = 129
Const WM_NCDESTROY = 130
Const WM_NCHITTEST = 132
Const WM_NCLBUTTONDBLCLK = 163
Const WM_NCLBUTTONDOWN = 161
Const WM_NCLBUTTONUP = 162
Const WM_NCMBUTTONDBLCLK = 169
Const WM_NCMBUTTONDOWN = 167
Const WM_NCMBUTTONUP = 168
Const WM_NCMOUSEMOVE = 160
Const WM_NCPAINT = 133
Const WM_NCRBUTTONDBLCLK = 166
Const WM_NCRBUTTONDOWN = 164
Const WM_NCRBUTTONUP = 165
Const WM_NEXTDLGCTL = 40
Const WM_NOTIFY = 78
Const WM_NOTIFYFORMAT = 85
Const WM_PAINT = 15
Const WM_PAINTCLIPBOARD = 777
Const WM_PAINTICON = 38
Const WM_PALETTECHANGED = 785
Const WM_PALETTEISCHANGING = 784
Const WM_PARENTNOTIFY = 528
Const WM_PASTE = 770
Const WM_PENWINIRST = 896
Const WM_PENWINLAST = 911
Const WM_POWER = 72
Const WM_POWERBROADCAST = 536
Const WM_PRINT = 791
Const WM_PRINTCLIENT = 792
Const WM_PSD_ENVSTAMPRECT = 1029
Const WM_PSD_FULLPAGERECT = 1025
Const WM_PSD_GREEKTEXTRECT = 1028
Const WM_PSD_MARGINRECT = 1027
Const WM_PSD_MINMARGINRECT = 1026
Const WM_PSD_PAGESETUPDLG = 1024
Const WM_PSD_YAFULLPAGERECT = 1030
Const WM_QUERYDRAGICON = 55
Const WM_QUERYENDSESSION = 17
Const WM_QUERYNEWPALETTE = 783
Const WM_QUERYOPEN = 19
Const WM_QUEUESYNC = 35
Const WM_QUIT = 18
Const WM_RBUTTONDBLCLK = 518
Const WM_RBUTTONDOWN = 516
Const WM_RBUTTONUP = 517
Const WM_RENDERALLFORMATS = 774
Const WM_RENDERFORMAT = 773
Const WM_SETCURSOR = 32
Const WM_SETFOCUS = 7
Const WM_SETFONT = 48
Const WM_SETHOTKEY = 50
Const WM_SETICON = 128
Const WM_SETREDRAW = 11
Const WM_SETTEXT = 12
Const WM_SETTINGCHANGE = 26
Const WM_SHOWWINDOW = 24
Const WM_SIZE = 5
Const WM_SIZECLIPBOARD = 779
Const WM_SIZING = 532
Const WM_SPOOLERSTATUS = 42
Const WM_STYLECHANGED = 125
Const WM_STYLECHANGING = 124
Const WM_SYSCHAR = 262
Const WM_SYSCOLORCHANGE = 21
Const WM_SYSCOMMAND = 274
Const WM_SYSDEADCHAR = 263
Const WM_SYSKEYDOWN = 260
Const WM_SYSKEYUP = 261
Const WM_TCARD = 82
Const WM_TIMECHANGE = 30
Const WM_TIMER = 275
Const WM_UNDO = 772
Const WM_USER = 1024
Const WM_USERCHANGED = 84
Const WM_VKEYTOITEM = 46
Const WM_VSCROLL = 277
Const WM_VSCROLLCLIPBOARD = 778
Const WM_WINDOWPOSCHANGED = 71
Const WM_WINDOWPOSCHANGING = 70
Const WM_WININICHANGE = 26
Const WM_KEYFIRST = 256
Const WM_KEYLAST = 264
Const WM_MOUSEFIRST = 512
Const WM_MOUSELAST = 521
Const WM_NEXTMENU = $0213

; System Commands:
Const SC_CLOSE        = 61536
Const SC_CONTEXTHELP  = 61824
Const SC_DEFAULT      = 61792
Const SC_HOTKEY       = 61776
Const SC_HSCROLL      = 61568
Const SC_KEYMENU      = 61696
Const SC_MAXIMIZE     = 61488
Const SC_ZOOM         = 61488
Const SC_MINIMIZE     = 61472
Const SC_ICON         = 61472
Const SC_MONITORPOWER = 61808
Const SC_MOUSEMENU    = 61584
Const SC_MOVE         = 61456
Const SC_NEXTWINDOW   = 61504
Const SC_PREVWINDOW   = 61520
Const SC_RESTORE      = 61728
Const SC_SCREENSAVE   = 61760
Const SC_SIZE         = 61440
Const SC_TASKLIST     = 61744
Const SC_VSCROLL      = 61552
Const SC_ARRANGE      = 61712


Function DLLPostMessage(hwndHandle,iMsg,iFlags1 = 0,iFlags2 = 0)
	Local iBankSize,mBankIn,iResult
	
	mBankIn  = CreateBank(16)
	PokeInt mBankIn,0,hwndHandle
	PokeInt mBankIn,4,iMsg
	PokeInt mBankIn,8,iFlags1
	PokeInt mBankIn,12,iFlags2
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "PostMessageWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

; ############################### COLOUR PICKER FUNCTIONS ################################

; Constant Assigns...
Const CC_FULLOPEN        = 2
Const CC_PREVENTFULLOPEN = 4

Function DLLChooseColor(mBankColours,iFlags = CC_FULLOPEN)
	Local iBankSize,mBankOut,iResult,iResultColor
	
	mBankOut = CreateBank(4)
	PokeInt mBankColours,17 * 4,iFlags
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "ChooseColorWrapper",mBankColours,mBankOut)
	
	If iResult Then iResultColor = PeekInt(mBankOut,0)
	bChooseColorFailedFlagA101B6 = iResult
	
	FreeBank mBankOut
	Return iResultColor
End Function

Function DLLLastChooseColourSucceeded()
	Return bChooseColorFailedFlagA101B6
End Function

Function DLLSimpleChooseColor(iInitRed = 0,iInitGreen = 0,iInitBlue = 0,iFlags = CC_FULLOPEN)
	Local mBankIn = DLLCreateColorBank()
	
	DLLSetBankColor(mBankIn,1,iInitRed,iInitGreen,iInitBlue)
	Return DLLChooseColor(mBankIn,iFlags)
End Function

Function DLLCreateColorBank()
	Return CreateBank(18 * 4)
End Function

Function DLLFreeColorBank(mBankOfColors)
	If mBankOfColors Then FreeBank mBankOfColors
End Function

Function DLLSetBankColor(mBankOfColors,iBankColourID,iRed = 0,iGreen = 0,iBlue = 0)
	iBankColourID = iBankColourID - 1
	If iBankColourID < 0 Then iBankColourID = 0
	If iBankColourID > 16 Then iBankColourID = 16
	
	PokeInt mBankOfColors,iBankColourID * 4,DLLPackColor(iRed,iGreen,iBlue)	
End Function

Function DLLGetBankColor(mBankOfColors,iBankColourID)
	iBankColourID = iBankColourID - 1
	If iBankColourID < 0 Then iBankColourID = 0
	If iBankColourID > 16 Then iBankColourID = 16

	Return PeekInt(mBankOfColors,iBankColourID * 4)
End Function

Function DLLPackColor(iRed = 0,iGreen = 0,iBlue = 0)
	Local iPackRed = (iRed And $000000FF)
	Local iPackGreen = (iGreen And $000000FF) Shl 8
	Local iPackBlue = (iBlue And $000000FF) Shl 16
	
	Return iPackBlue Or iPackGreen Or iPackRed
End Function

Function DLLGetColorRed(iMyColor)
	Return (iMyColor And $000000FF)
End Function

Function DLLGetColorGreen(iMyColor)
	Return (iMyColor And $0000FF00) Shr 8
End Function

Function DLLGetColorBlue(iMyColor)
	Return (iMyColor And $00FF0000) Shr 16
End Function

Function DLLSetClipboardText(sText$)
	Local mBankIn,iResult
	
	mBankIn = CreateBank(Len(sText$) + 1)
	PokeString(mBankIn,sText$)
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "SetClipboardDataWrapper",mBankIn)
	
	FreeBank mBankIn
	Return iResult
End Function

Function DLLGetClipboardText$(iBuffSize = 32768)
	Local mBankIn,iResult,sResult$ = ""
	
	mBankOut = CreateBank(iBuffSize)
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetClipboardDataWrapper",0,mBankOut)
	If iResult Then sResult$ = PeekString(mBankOut)

	FreeBank mBankOut
	Return sResult$
End Function

Function DLLEmptyClipboard()
	Return CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "EmptyClipboardWrapper")
End Function

Function DLLGetClipboardTextDataSize()
	Return CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetClipboardDataSizeWrapper")
End Function

Function DLLEZGetClipboardText$()
	Return DLLGetClipboardText$(DLLGetClipboardTextDataSize())
End Function

Const CF_BOTH                 = 3
Const CF_TTONLY               = $40000
Const CF_EFFECTS              = 256
Const CF_FIXEDPITCHONLY       = $4000
Const CF_FORCEFONTEXIST       = $10000
Const CF_INITTOLOGFONTSTRUCT  = 64
Const CF_LIMITSIZE            = $2000
Const CF_NOOEMFONTS           = $800
Const CF_NOFACESEL            = $80000
Const CF_NOSCRIPTSEL          = $800000
Const CF_NOSTYLESEL           = $100000
Const CF_NOSIZESEL            = $200000
Const CF_NOSIMULATIONS        = 4096
Const CF_NOVECTORFONTS        = $800
Const CF_NOVERTFONTS          = $1000000
Const CF_PRINTERFONTS         = 2
Const CF_SCALABLEONLY         = $20000
Const CF_SCREENFONTS          = 1
Const CF_SCRIPTSONLY          = $400
Const CF_SELECTSCRIPT         = $400000
Const CF_WYSIWYG              = $8000

Const FW_DONTCARE   = 0
Const FW_THIN       = 100
Const FW_EXTRALIGHT = 200
Const FW_ULTRALIGHT = 200
Const FW_LIGHT      = 300
Const FW_NORMAL     = 400
Const FW_REGULAR    = 400
Const FW_MEDIUM     = 500
Const FW_SEMIBOLD   = 600
Const FW_DEMIBOLD   = 600
Const FW_BOLD       = 700
Const FW_EXTRABOLD  = 800
Const FW_ULTRABOLD  = 800
Const FW_HEAVY      = 900
Const FW_BLACK      = 900

Function DLLChooseFont(iFontBank,iFlags = CF_SCREENFONTS,iMinSize = 8,iMaxSize = 16)
	Local mBankIn,iResult
	
	mBankIn  = CreateBank(16)
	PokeInt mBankIn,0,iFlags
	PokeInt mBankIn,4,iMinSize
	PokeInt mBankIn,8,iMaxSize
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "ChooseFontWrapper",mBankIn,iFontBank)

	FreeBank mBankIn
	Return iResult
End Function

Const FONT_ATTRIBUTES_WIDTH          = 1
Const FONT_ATTRIBUTES_HEIGHT         = 2
Const FONT_ATTRIBUTES_ESCAPEMENT     = 3
Const FONT_ATTRIBUTES_ORIENTATION    = 4
Const FONT_ATTRIBUTES_WEIGHT         = 5
Const FONT_ATTRIBUTES_ITALIC         = 6
Const FONT_ATTRIBUTES_UNDERLINE      = 7
Const FONT_ATTRIBUTES_STRIKEOUT      = 8
Const FONT_ATTRIBUTES_CHARACTERSET   = 9
;Const FONT_ATTRIBUTES_OUTPRECISION   = 10
;Const FONT_ATTRIBUTES_CLIPPRECISION  = 11
Const FONT_ATTRIBUTES_QUALITY        = 12
Const FONT_ATTRIBUTES_PITCHANDFAMILY = 13
Const FONT_ATTRIBUTES_COLOR          = 14
Const FONT_ATTRIBUTES_FACE           = 15

Const ANSI_CHARSET        = 0
Const DEFAULT_CHARSET     = 1
Const SYMBOL_CHARSET      = 2
Const SHIFTJIS_CHARSET    = 128
Const HANGEUL_CHARSET     = 129
Const GB2312_CHARSET      = 134
Const CHINESEBIG5_CHARSET = 136
Const GREEK_CHARSET       = 161
Const TURKISH_CHARSET     = 162
Const HEBREW_CHARSET      = 177
Const ARABIC_CHARSET      = 178
Const BALTIC_CHARSET      = 186
Const RUSSIAN_CHARSET     = 204
Const THAI_CHARSET        = 222
Const EASTEUROPE_CHARSET  = 238

Const OEM_CHARSET = 255

Const OUT_DEFAULT_PRECIS   = 0
Const OUT_STRING_PRECIS    = 1
Const OUT_CHARACTER_PRECIS = 2
Const OUT_STROKE_PRECIS    = 3
Const OUT_TT_PRECIS        = 4
Const OUT_DEVICE_PRECIS    = 5
Const OUT_RASTER_PRECIS    = 6
Const OUT_TT_ONLY_PRECIS   = 7
Const OUT_OUTLINE_PRECIS   = 8

Const CLIP_DEFAULT_PRECIS   = 0
Const CLIP_CHARACTER_PRECIS = 1
Const CLIP_STROKE_PRECIS    = 2
Const CLIP_MASK             = 15
Const CLIP_LH_ANGLES        = 16
Const CLIP_TT_ALWAYS        = 32
Const CLIP_EMBEDDED         = 128

Const DEFAULT_QUALITY        = 0
Const DRAFT_QUALITY          = 1
Const PROOF_QUALITY          = 2
Const NONANTIALIASED_QUALITY = 3
Const ANTIALIASED_QUALITY    = 4
Const DEFAULT_PITCH          = 0
Const FIXED_PITCH            = 1
Const VARIABLE_PITCH         = 2

Const FF_DECORATIVE = 80
Const FF_DONTCARE   = 0
Const FF_MODERN     = 48
Const FF_ROMAN      = 16
Const FF_SCRIPT     = 64
Const FF_SWISS      = 32

Const FONT_COLOR_BLACK   = $00000000
Const FONT_COLOR_MAROON  = $00000080
Const FONT_COLOR_GREEN   = $00008000
Const FONT_COLOR_OLIVE   = $00008080
Const FONT_COLOR_NAVY    = $00800000
Const FONT_COLOR_PURPLE  = $00800080
Const FONT_COLOR_TEAL    = $00808000
Const FONT_COLOR_GRAY    = $00808080
Const FONT_COLOR_SILVER  = $00C0C0C0
Const FONT_COLOR_RED     = $000000FF
Const FONT_COLOR_LIME    = $0000FF00
Const FONT_COLOR_YELLOW  = $0000FFFF
Const FONT_COLOR_BLUE    = $00FF0000
Const FONT_COLOR_FUCHSIA = $00FF00FF
Const FONT_COLOR_AQUA    = $00FFFF00
Const FONT_COLOR_WHITE   = $00FFFFFF

Function DLLGetFontAttribute$(iFontBank,iFontAttribute)
	If iFontBank <> 0
		Select iFontAttribute
			Case FONT_ATTRIBUTES_HEIGHT
				Return PeekInt(iFontBank,0)
			Case FONT_ATTRIBUTES_WIDTH
				Return PeekInt(iFontBank,4)
			Case FONT_ATTRIBUTES_ESCAPEMENT
				Return PeekInt(iFontBank,8)
			Case FONT_ATTRIBUTES_ORIENTATION
				Return PeekInt(iFontBank,12)
			Case FONT_ATTRIBUTES_WEIGHT
				Return PeekInt(iFontBank,16)
			Case FONT_ATTRIBUTES_ITALIC
				Return PeekByte(iFontBank,20)
			Case FONT_ATTRIBUTES_UNDERLINE
				Return PeekByte(iFontBank,21)
			Case FONT_ATTRIBUTES_STRIKEOUT
				Return PeekByte(iFontBank,22)
			Case FONT_ATTRIBUTES_CHARACTERSET
				Return PeekByte(iFontBank,23)
;			Case FONT_ATTRIBUTES_OUTPRECISION
;				Return PeekByte(iFontBank,24)
;			Case FONT_ATTRIBUTES_CLIPPRECISION
;				Return PeekByte(iFontBank,25)
			Case FONT_ATTRIBUTES_QUALITY
				Return PeekByte(iFontBank,26)
			Case FONT_ATTRIBUTES_PITCHANDFAMILY
				Return PeekByte(iFontBank,27)
			Case FONT_ATTRIBUTES_COLOR
				Return PeekInt(iFontBank,61)
			Case FONT_ATTRIBUTES_FACE
				Return PeekString(iFontBank,28)
			Default
				Return 0
		End Select
	EndIf
End Function

Function DLLSetFontAttribute(iFontBank,iFontAttribute,iValue$)
	If iFontBank <> 0
		Select iFontAttribute
			Case FONT_ATTRIBUTES_HEIGHT
				PokeInt(iFontBank,0,Float(iValue) * 1.33333334)
			Case FONT_ATTRIBUTES_WIDTH
				PokeInt(iFontBank,4,iValue)
			Case FONT_ATTRIBUTES_ESCAPEMENT
				PokeInt(iFontBank,8,iValue)
			Case FONT_ATTRIBUTES_ORIENTATION
				PokeInt(iFontBank,12,iValue)
			Case FONT_ATTRIBUTES_WEIGHT
				PokeInt(iFontBank,16,iValue)
			Case FONT_ATTRIBUTES_ITALIC
				PokeByte(iFontBank,20,iValue)
			Case FONT_ATTRIBUTES_UNDERLINE
				PokeByte(iFontBank,21,iValue)
			Case FONT_ATTRIBUTES_STRIKEOUT
				PokeByte(iFontBank,22,iValue)
			Case FONT_ATTRIBUTES_CHARACTERSET
				PokeByte(iFontBank,23,iValue)
;			Case FONT_ATTRIBUTES_OUTPRECISION
;				PokeByte(iFontBank,24,iValue)
;			Case FONT_ATTRIBUTES_CLIPPRECISION
;				PokeByte(iFontBank,25,iValue)
			Case FONT_ATTRIBUTES_QUALITY
				PokeByte(iFontBank,26,iValue)
			Case FONT_ATTRIBUTES_PITCHANDFAMILY
				PokeByte(iFontBank,27,iValue)
			Case FONT_ATTRIBUTES_COLOR
				PokeInt(iFontBank,61,iValue)
			Case FONT_ATTRIBUTES_FACE
				PokeString(iFontBank,Left$(iValue,32),28)
		End Select
	EndIf
End Function

Function DLLCreateFontBank()
	Return CreateBank(65)
End Function

Function DLLFreeFontBank(iFontBank)
	If iFontBank <> 0 Then FreeBank iFontBank
	Return True
End Function

Function DLLSetWindowZOrder(windowHandle,windowAfterHandle)
	Local x,y,w,h
	x = DLLGetWindowX(windowHandle)
	y = DLLGetWindowY(windowHandle)
	w = DLLGetWindowWidth(windowHandle)
	h = DLLGetWindowHeight(windowHandle)
	Return DLLSetWindowPos(windowHandle,windowAfterHandle,x,y,w,h)
End Function

Function DLLCRC32Bank(iBankToCRC)
	Return CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "CRCBuffer",iBankToCRC)
End Function

Function DLLCRC32File(sFileName$)
	Local mBankIn,iCRC
	
	mBankIn  = CreateBank(Len(sFileName$) + 1)
	PokeString mBankIn,sFileName$
	
	iCRC = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "CRCFile",mBankIn)

	FreeBank mBankIn
	Return iCRC
End Function

;
; These are the constants for the  DLLGetVolumeInfo()  command.  Pass  any  of  these 
; in-place of iFlags to return the  specified  information.  VOL_GETVOLUMESERIAL  and
; VOL_GETVOLUMEFLAGS return an integer value, as opposed to the others' string values.
;
Const VOL_GETVOLUMENAME       = 0
Const VOL_GETVOLUMEFILESYSTEM = 1
Const VOL_GETVOLUMESERIAL     = 2
Const VOL_GETVOLUMEFLAGS	  = 3

;
; AND one of these constants with DLLGetVolumeInfo(drive$,VOL_GETVOLUMEFLAGS) and
; check to see if it's TRUE.  If so,  then  the drive/volume/file-system supports
; the queried flag.
;
Const FS_CASE_SENSITIVE            = $1
Const FS_CASE_IS_PRESERVED         = $2
Const FS_UNICODE_STORED_ON_DISK    = $4
Const FS_PERSISTENT_ACLS           = $8
Const FS_FILE_COMPRESSION          = $10
Const FS_VOLUME_IS_COMPRESSED      = $8000
Const FILE_VOLUME_QUOTAS           = $20
Const FILE_SUPPORTS_SPARSE_FILES   = $40
Const FILE_SUPPORTS_REPARSE_POINTS = $80
Const FILE_NAMED_STREAMS 			     = $40000
Const FILE_SUPPORTS_ENCRYPTION     = $20000
Const FILE_SUPPORTS_OBJECT_IDS     = $10000

Function DLLGetVolumeInfo$(sDrive$,iFlags,iOutBufferSize = 512)
	Local iBankSize,mBankIn,mBankOut,iResult,sResult$
	
	mBankIn  = CreateBank(iOutBufferSize)
	mBankOut = CreateBank(iOutBufferSize)

	PokeInt mBankIn,0,iFlags
	PokeString(mBankIn,sDrive$,4)
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "GetDriveInfoWrapper",mBankIn,mBankOut)
	sResult$ = PeekString$(mBankOut,0)

	FreeBank mBankIn
	FreeBank mBankOut

	Select iFlags
		Case VOL_GETVOLUMENAME, VOL_GETVOLUMEFILESYSTEM
			Return sResult$

		Default
			Return iResult
			
	End Select
End Function


Const BIF_RETURNONLYFSDIRS  = 1
Const BIF_DONTGOBELOWDOMAIN = 2 
Const BIF_RETURNFSANCESTORS = 8
Const BIF_NEWDIALOGSTYLE    = $40
Const BIF_EDITBOX           = $10
Const BIF_VALIDATE          = $20
Const BIF_USENEWUI          = (BIF_NEWDIALOGSTYLE Or BIF_EDITBOX)

Const BIF_BROWSEFORCOMPUTER  = $1000
Const BIF_BROWSEFORPRINTER   = $2000 
Const BIF_BROWSEINCLUDEFILES = $3000

Function DLLBrowseForFolder$(sTitle$ = "Choose a folder:",iFlags = BIF_NEWDIALOGSTYLE Or BIF_RETURNONLYFSDIRS,iOutBufferSize = 512)
	Local iBankSize,mBankIn,mBankOut,iResult,sResult$ = ""
	
	iBankSize = Len(sTitle$) + 5
	
	mBankIn  = CreateBank(iBankSize)
	mBankOut = CreateBank(iOutBufferSize)

	PokeInt mBankIn,0,iFlags
	PokeString(mBankIn,sTitle$,4)
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "BrowseForFolderWrapper",mBankIn,mBankOut)
	
	; Pull the resulting string out of the bank...
	If iResult <> 0
		sResult$ = PeekString$(mBankOut,0)
	EndIf
	
	FreeBank mBankIn
	FreeBank mBankOut
	Return sResult$
End Function

Const DC_DRIVERVERSION  = 0
Const DC_TECHNOLOGY     = 2     ,DT_PLOTTER = 0,DT_RASDISPLAY = 1,DT_RASPRINTER = 2,DT_RASCAMERA = 3,DT_CHARSTREAM = 4,DT_METAFILE = 5,DT_DISPFILE = 6
Const DC_HORZSIZE       = 4
Const DC_VERTSIZE       = 6
Const DC_HORZRES        = 8
Const DC_VERTRES        = 10
Const DC_LOGPIXELSX     = 88
Const DC_LOGPIXELSY     = 90
Const DC_BITSPIXEL      = 12
Const DC_PLANES         = 14
Const DC_NUMBRUSHES     = 16
Const DC_NUMPENS        = 18
Const DC_NUMFONTS       = 22
Const DC_NUMCOLORS      = 24
Const DC_NUMMARKERS     = 20
Const DC_ASPECTX        = 40
Const DC_ASPECTY        = 42
Const DC_ASPECTXY       = 44
Const DC_PDEVICESIZE    = 26
Const DC_CLIPCAPS       = 36
Const DC_SIZEPALETTE    = 104
Const DC_NUMRESERVED    = 106
Const DC_COLORRES       = 108
Const DC_VREFRESH       = 116
Const DC_DESKTOPHORZRES = 118
Const DC_DESKTOPVERTRES = 117
Const DC_BLTALIGNMENT   = 119
Const DC_RASTERCAPS     = 38    ,RC_BANDING = 2,RC_BITBLT = 1,RC_BITMAP64 = 8,RC_DI_BITMAP = 128,RC_DIBTODEV = 512,RC_FLOODFILL = 4096,RC_GDI20_OUTPUT = 16,RC_PALETTE = 256,RC_SCALING = 4,RC_STRETCHBLT = 2048,RC_STRETCHDIB = 8192,RC_DEVBITS = $8000,RC_OP_DX_OUTPUT = $4000
Const DC_CURVECAPS      = 28    ,CC_NONE = 0,CC_CIRCLES = 1,CC_PIE = 2,CC_CHORD = 4,CC_ELLIPSES = 8,CC_WIDE = 16,CC_STYLED = 32,CC_WIDESTYLED = 64,CC_INTERIORS = 128,CC_ROUNDRECT = 256
Const DC_LINECAPS       = 30    ,LC_NONE = 0,LC_POLYLINE = 2,LC_MARKER = 4,LC_POLYMARKER = 8,LC_WIDE = 16,LC_STYLED = 32,LC_WIDESTYLED = 64,LC_INTERIORS = 128
Const DC_POLYGONALCAPS  = 32    ,RC_BIGFONT = 1024,RC_GDI20_STATE = 32,RC_NONE = 0,RC_SAVEBITMAP = 64,PC_NONE = 0,PC_POLYGON = 1,PC_POLYPOLYGON = 256,PC_PATHS = 512,PC_RECTANGLE = 2,PC_WINDPOLYGON = 4,PC_SCANLINE = 8,PC_TRAPEZOID = 4,PC_WIDE = 16,PC_STYLED = 32,PC_WIDESTYLED = 64,PC_INTERIORS = 128
Const DC_TEXTCAPS       = 34    ,TC_OP_CHARACTER = 1,TC_OP_STROKE = 2,TC_CP_STROKE = 4,TC_CR_90 = 8,TC_CR_ANY = 16,TC_SF_X_YINDEP = 32,TC_SA_DOUBLE = 64,TC_SA_INTEGER = 128,TC_SA_CONTIN = 256,TC_EA_DOUBLE = 512,TC_IA_ABLE = 1024,TC_UA_ABLE = 2048,TC_SO_ABLE = 4096,TC_RA_ABLE = 8192,TC_VA_ABLE = 16384,TC_RESERVED = 32768,TC_SCROLLBLT = 65536

Function DLLDesktopDeviceCaps(iCaps)
	Local mBankIn,iResult
	
	mBankIn  = CreateBank(4)
	PokeInt mBankIn,0,iCaps
	
	iResult = CallDLL(sBlitzSysDLLNameA101B6,sBlitzSysDLLCommandPrefixA101B6 + "DesktopDeviceCaps",mBankIn)

	FreeBank mBankIn
	Return iResult
End Function

Function DLLDesktopWidth()
	Return DLLDesktopDeviceCaps(DC_HORZRES)
End Function

Function DLLDesktopHeight()
	Return DLLDesktopDeviceCaps(DC_VERTRES)
End Function

Function DLLDesktopDepth()
	Return DLLDesktopDeviceCaps(DC_BITSPIXEL)
End Function

Function DLLFindBlitzRuntimeHwnd(sAppName$,sClosePrompt$ = "")
	Local windowHandle,sAppTempName$,sRandomString$ = " "
	
	SeedRnd MilliSecs()
	
	For a = 0 To 50
		sRandomString$ = sRandomString$ + Chr(Rnd(65,90))
	Next
	sAppTempName$ = sAppName$ + sRandomString$
	AppTitle sAppTempName$
	windowHandle = DLLFindWindow(sAppTempName$)
	SeedRnd 0
	
	; If this happens, there is a REAL problem, so closing the program should be fine..
	If windowHandle = False Then RuntimeError("Blitz window could not be found! Program Terminating!")
	AppTitle sAppName$,sClosePrompt$
	Return windowHandle
End Function

; ########################################################################################

; Null terminated string poke... dont think blitz has a function for this?
Function PokeString(mBankAddr,sStringOut$,iBufferOffset = 0)
	For n = 1 To Len(sStringOut$)
		PokeByte mBankAddr,iBufferOffset,Asc(Mid$(sStringOut$,n,1))
		iBufferOffset = iBufferOffset + 1
	Next
	PokeByte mBankAddr,iBufferOffset,0 ; Null terminate
End Function

; Null terminated string peek... dont think blitz has a function for this?
Function PeekString$(mBankAddr,iBufferOffset = 0)
	Local sOutStr$ = "",iByte
	
	For n = 0 To BankSize(mBankAddr)
		iByte = PeekByte(mBankAddr,iBufferOffset)
		If iByte <> 0 
			sOutStr$ = sOutStr$ + Chr(iByte)
		Else
			Exit
		EndIf
		iBufferOffset = iBufferOffset + 1
	Next

	Return sOutStr$
End Function

; DEBUG PURPOSES ONLY!
Function OutBank(sFile$,mBank)
	Local fhl
	fhl = WriteFile(sFile$)
	WriteBytes mBank,fhl,0,BankSize(mBank)
	CloseFile fhl
End Function