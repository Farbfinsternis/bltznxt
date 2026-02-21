; GUI Variablen
Type tScreen
	Field w,h,d,m
	Field title$
End Type 
Global scr.tScreen=New tScreen

Type tMenubutton
	Field x,y
	Field w,h
	Field txt$
	Field status
	Field num
End Type
Global mgad.tMenubutton

Type tButton
	Field x,y
	Field w,h
	Field txt$
	Field status
	Field num
End Type
Global gad.tButton

Type tBorder
	Field x,y
	Field w,h
	Field typ
	Field txt$
	Field num
End Type
Global border.tBorder

Type tLine
	Field x,y
	Field w
	Field typ
	Field num
End Type
Global line3d.tLine

Type tPanel
	Field x,y
	Field w,h
	Field title$
	Field num
	Field th
End Type
Global panel.tPanel

Type tInput
	Field in$
	Field x,y
	Field w,h
	Field cpos
	Field title$
	Field num
	Field active
	Field msec
	Field cursor
End Type
Global inp.tInput

Type tCheckBox
	Field x,y,w,h
	Field status
	Field txt$
	Field num
End Type
Global checkbox.tCheckBox

Type tSlider
	Field x,y,w
	Field max
	Field current
	Field current_value
	Field num
	Field txt$
End Type
Global slider.tSlider

;-----------------------------------------------------------
; GUI Funktionen
;-----------------------------------------------------------
; Menuebuttons
;
; Menuebutton erzeugen
Function GUICreateMenuButton(num,x,y,width,txt$)
	mgad.tMenubutton=New tMenubutton
	mgad\x=x
	mgad\y=y
	mgad\w=width
	mgad\h=StringHeight("X")+4
	mgad\txt$=txt$
	mgad\status=0
	mgad\num=num
End Function

; Menuebutton loeschen
Function GUIKillMenuButton(num)
	For mgad.tMenubutton=Each tMenubutton
		If mgad\num=num
			Delete mgad
			Exit
		EndIf
	Next
End Function

; Menuebuttons checken
Function GUICheckMenuButtons(x,y)
	result=-1
	For mgad.tMenubutton=Each tMenubutton
		x1=mgad\x
		y1=mgad\y
		x2=x1+mgad\w
		y2=y1+mgad\h
		If x>x1 And y>y1 And x<x2 And y<y2
			mgad\status=1
			If MouseDown(1)
				mgad\status=2
				result=mgad\num
			EndIf
		Else
			mgad\status=0
		EndIf
	Next
	Return result
End Function

; Menuebuttons neuzeichnen
Function GUIRedrawMenuButtons()
	For mgad.tMenubutton=Each tMenubutton
		x1=mgad\x
		y1=mgad\y
		ww=mgad\w
		hh=mgad\h
		x2=x1+ww
		y2=y1+hh
		t$=mgad\txt$
		tx=x1+(ww/2)
		ty=y1+(hh/2)
		
		Select mgad\status
			Case 0
				Color 192,192,192
				Rect x1,y1,ww,hh,1
				Color 127,127,127
				Line x2-2,y1+2,x2-2,y2-2
				Color 255,255,255
				Line x2-1,y1+1,x2-1,y2-1
				Color 0,0,0
				Text tx,ty,t$,1,1
			Case 1
				Color 192,192,192
				Rect x1,y1,ww,hh,1
				Color 127,127,127
				Rect x1,y1,ww,hh,0
				Color 255,255,255
				Line x1,y1,x2-1,y1
				Line x1,y1,x1,y2-1
				Color 0,0,0
				Text tx,ty,t$,1,1
			Case 2
				Color 0,0,200
				Rect x1,y1,ww,hh,1
				Color 255,255,255
				Rect x1,y1,ww,hh,0
				Color 127,127,127
				Line x1,y1,x2-1,y1
				Line x1,y1,x1,y2-1
				Color 255,255,255
				Text tx,ty,t$,1,1
		End Select
	Next
End Function

;-----------------------------------------------------------
; normale Buttons
Function GUICreateButton(num,x,y,width,height,txt$)
	gad.tButton=New tButton
	gad\x=x
	gad\y=y
	gad\w=width
	gad\h=height
	gad\num=num
	gad\txt$=txt$
	gad\status=0
End Function

Function GUIKillButton(num)
	For gad.tButton=Each tButton
		If num=gad\num
			Delete gad
			Exit
		EndIf
	Next
End Function

Function GUIRedrawButton()
	For gad.tButton=Each tButton
		x1=gad\x
		y1=gad\y
		ww=gad\w
		hh=gad\h
		t$=gad\txt$
		x2=x1+ww
		y2=y1+hh
		tx=x1+(ww/2)
		ty=y1+(hh/2)
		Select gad\status
			Case 0
				Color 192,192,192
				Rect x1,y1,ww,hh,1
				Color 127,127,127
				Rect x1,y1,ww,hh,0
				Color 255,255,255
				Line x1,y1,x2-1,y1
				Line x1,y1,x1,y2-1
				Color 0,0,0
				Text tx,ty,t$,1,1
			Case 1
				Color 192,192,192
				Rect x1,y1,ww,hh,1
				Color 255,255,255
				Rect x1,y1,ww,hh,0
				Color 127,127,127
				Line x1,y1,x2-1,y1
				Line x1,y1,x1,y2-1
				Color 0,0,0
				Text tx,ty,t$,1,1
			Case 2
				Color 0,0,200
				Rect x1,y1,ww,hh,1
				Color 255,255,255
				Rect x1,y1,ww,hh,0
				Color 127,127,127
				Line x1,y1,x2-1,y1
				Line x1,y1,x1,y2-1
				Color 255,255,255
				Text tx,ty,t$,1,1
		End Select
	Next
End Function

Function GUICheckButton(x,y)
	r=-1
	For gad.tButton=Each tButton
		x1=gad\x
		y1=gad\y
		x2=x1+gad\w
		y2=y1+gad\h
		If x>x1 And y>y1 And x<x2 And y<y2
			gad\status=1
			If MouseDown(1)
				gad\status=2
				r=gad\num
			EndIf
		Else
			gad\status=0
		EndIf
	Next
	Return r
End Function

;-----------------------------------------------------------
; Checkboxen
Function GUICreateCheckBox(num,x,y,width,height,status,txt$)
	checkbox.tCheckBox=New tCheckBox
	checkbox\x=x
	checkbox\y=y
	checkbox\w=width
	checkbox\h=height
	checkbox\status=status
	checkbox\num=num
	checkbox\txt$=txt$
End Function

Function GUIKillCheckBox(num)
	For checkbox.tCheckBox=Each tCheckBox
		If num=checkbox\num
			Delete checkbox
			Exit
		EndIf
	Next
End Function

Function GUIRedrawCheckBox()
	For checkbox.tCheckBox=Each tCheckBox
		x1=checkbox\x
		y1=checkbox\y
		ww=checkbox\w
		hh=checkbox\h
		x2=x1+ww
		y2=y1+hh
		st=checkbox\status
		t$=checkbox\txt$
		
		Color 192,192,192
		Rect x1,y1,ww,hh,1
		Color 255,255,255
		Rect x1,y1,ww,hh,0
		Color 127,127,127
		Line x1,y1,x2-1,y1
		Line x1,y1,x1,y2-1
		If st=1
			Color 0,0,200
			Rect x1+2,y1+2,ww-4,hh-4,1
		EndIf
		If Not t$=""
			Color 0,0,0
			Text x2+4,y1+(hh/2),t$,0,1
		EndIf
	Next
End Function

Function GUICheckCheckBox(x,y)
	For checkbox.tCheckBox=Each tCheckBox
		x1=checkbox\x
		y1=checkbox\y
		sw=StringWidth(checkbox\txt$)
		x2=x1+checkbox\w+sw
		y2=y1+checkbox\h
		If x>x1 And y>y1 And x<x2 And y<y2
			checkbox\status=1-checkbox\status
			Return checkbox\num
		EndIf
	Next
End Function

Function GUIGetCheckBoxStatus(num)
	For checkbox.tCheckBox=Each tCheckBox
		If num=checkbox\num Then Return checkbox\status
	Next
End Function

Function GUISetCheckBoxStatus(num,status)
	For checkbox.tCheckBox=Each tCheckBox
		If num=checkbox\num Then checkbox\status=status
	Next
End Function

;-----------------------------------------------------------
; Border und Linien
;
; Border erstellen
Function GUICreateBorder(num,x,y,width,height,typ,title$)
	border.tBorder=New tBorder
	border\x=x
	border\y=y
	border\w=width
	border\h=height
	border\typ=typ
	border\txt$=title$
	border\num=num
End Function

Function GUIKillBorder(num)
	For border.tBorder=Each tBorder
		If border\num=num
			Delete border
			Exit
		EndIf
	Next
End Function

Function GUIRedrawBorder()
	For border.tBorder=Each tBorder
		x1=border\x
		y1=border\y
		ww=border\w
		hh=border\h
		t$=border\txt$
		tt=border\typ
		x2=x1+ww
		y2=y1+hh
		tx=x1+4
		ty=y1
		Select tt
			Case 0
				Color 127,127,127
				Rect x1,y1,ww,hh,0
				Color 255,255,255
				Rect x1+1,y1+1,ww,hh,0
			Case 1
				Color 255,255,255
				Rect x1,y1,ww,hh,0
				Color 127,127,127
				Rect x1+1,y1+1,ww,hh,0
			Case 2
				Color 127,127,127
				Rect x1,y1,ww,hh,0
				Color 255,255,255
				Line x1,y1,x2-1,y1
				Line x1,y1,x1,y2-1
			Case 3
				Color 255,255,255
				Rect x1,y1,ww,hh,0
				Color 127,127,127
				Line x1,y1,x2-1,y1
				Line x1,y1,x1,y2-1
		End Select
		If Not t$=""
			tw=StringWidth(t$)+4
			th=StringHeight(t$)
			Color 192,192,192
			Rect x1+2,y1-(th/2),tw,th,1
			Color 255,255,255
			Text tx,ty,t$,0,1
		EndIf
	Next
End Function

Function GUICreateLine(num,x,y,width,typ)
	line3d.tLine=New tLine
	line3d\num=num
	line3d\x=x
	line3d\y=y
	line3d\w=width
	line3d\typ=typ
End Function

Function GUIKillLine(num)
	For line3d.tLine=Each tLine
		If line3d\num=num
			Delete line3d
			Exit
		EndIf
	Next
End Function

Function GUIRedrawLine()
	For line3d.tLine=Each tLine
		x1=line3d\x
		y1=line3d\y
		ww=line3d\w
		x2=x1+ww
		Select line3d\typ
			Case 0
				Color 127,127,127
				Line x1,y1,x2-1,y1
				Color 255,255,255
				Line x1+1,y1+1,x2,y1+1
			Case 1
				Color 255,255,255
				Line x1,y1,x2-1,y1
				Color 127,127,127
				Line x1+1,y1+1,x2,y1+1
		End Select
	Next			
End Function

;-----------------------------------------------------------
; Panels
Function GUICreatePanel(num,x,y,width,height,title$)
	panel.tPanel=New tPanel
	panel\x=x
	panel\y=y
	panel\w=width
	panel\h=height
	panel\title$=title$
	panel\num=num
	panel\th=StringHeight(title$)
End Function

Function GUIKillPanel(num)
	For panel.tPanel=Each tPanel
		If panel\num=num
			Delete panel
			Exit
		EndIf
	Next
End Function

Function GUIRedrawPanel()
	For panel.tPanel=Each tPanel
		x1=panel\x
		y1=panel\y
		ww=panel\w
		hh=panel\h
		x2=x1+ww
		y2=y1+hh
		t$=panel\title$
		th=panel\th
		tx=x1+8 : ty=y1+4+(th/2)
		
		Color 192,192,192
		Rect x1,y1,ww,hh,1
		
		Color 127,127,127
		Rect x1,y1,ww,hh,0
		Rect x1+1,y1+1,ww-2,hh-2,0
		Line x1+4,y1+th+8,x2-5,y1+th+8
		
		Color 255,255,255
		Line x1,y1,x2-1,y1
		Line x1,y1+1,x2-2,y1+1
		Line x1,y1,x1,y2-1
		Line x1+1,y1,x1+1,y2-2
		Line x1+5,y1+th+9,x2-4,y1+th+9
		Color 0,0,0
		Rect x1-1,y1-1,ww+2,hh+2,0
		Text tx,t$,n$,0,1		
	Next
End Function

;-----------------------------------------------------------
; Inputfelder
Function GUICreateInput(num,x,y,width,height,startwert$,title$)
	inp.tInput=New tInput
	inp\num=num
	inp\x=x
	inp\y=y
	inp\w=width
	inp\h=height
	inp\title$=title$
	inp\active=0
	inp\cpos=0
	inp\msec=MilliSecs()
	inp\cursor=0
	inp\in$=startwert$
End Function

Function GUIKillInput(num)
	For inp.tInput=Each tInput
		If inp\num=num
			Delete inp
			Exit
		EndIf
	Next
End Function

Function GUICheckInput(x,y,direct)
	If direct<=0 And MouseDown(1)
		For inp.tInput=Each tInput
			x1=inp\x
			y1=inp\y
			x2=x1+inp\w+StringWidth(inp\title$)
			y2=y1+inp\h
			If x>x1 And y>y1 And x<x2 And y<y2
				inp\active=1
				Exit
			EndIf
		Next
	Else
		For inp.tInput=Each tInput
			If inp\num=direct
				inp\active=1
				activenum=inp\num
				Exit
			EndIf
		Next
	EndIf
		
	key=GetKey()
	If key
		For inp.tInput=Each tInput
			If inp\active=1
				in$=inp\in$
				If key=8 And Len(in$)>=1 Then in$=Left(in$,Len(in$)-1)
				If key=13
					inp\active=0
				EndIf
				If Len(in$)<6
					If key=46 Or key=44 Then in$=in$+Chr(46)
					If key>47 And key<58 Then in$=in$+Chr(key)
					If Float(in$)>1.0 Then in$="1.0"
					If Float(in$)<0.0 Then in$="0.0" 
					inp\in$=in$
					Exit
				EndIf
			EndIf
		Next
	EndIf
	Return key
End Function

Function GUIRedrawInput()
	For inp.tInput=Each tInput
		x1=inp\x
		y1=inp\y
		ww=inp\w
		hh=inp\h
		n$=inp\in$
		t$=inp\title$
		ac=inp\active
		;cp=inp\cpos
		x2=x1+ww
		y2=y1+hh
		cp=x1+StringWidth(n$)+2
		
		Color 192,192,192
		Rect x1,y1,ww,hh,1
		Color 255,255,255
		Rect x1,y1,ww,hh,0
		Color 127,127,127
		Line x1,y1,x2-1,y1
		Line x1,y1,x1,y2-1
		If MilliSecs()-inp\msec>=500
			inp\cursor=1-inp\cursor
			inp\msec=MilliSecs()
		EndIf
		
		Color 0,0,0
		Text x1+2,y1+(hh/2),n$,0,1
		Text x2+4,y1+(hh/2),t$,0,1
		If inp\cursor=1 And inp\active=1
			Color 0,0,200
			Rect cp,y1+2,StringWidth("X"),hh-4,1
		EndIf
	Next
End Function

Function GUIGetInput$(num)
	For inp.tInput=Each tInput
		If inp\num=num Then Return inp\in$
	Next
End Function

;-----------------------------------------------------------
; Alles neuzeichnen und Maus pruefen
Function GUIRedraw()
	GUIRedrawMenuButtons()
	GUIRedrawButton()
	GUIRedrawCheckBox()
	GUIRedrawInput()
	GUIRedrawBorder()
	GUIRedrawLine()
End Function