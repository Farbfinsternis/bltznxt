; Multiplayer Globals
;-------------------------------------------------
;
Type tGList
	Field name$
	Field desc$
	Field pwd$
	Field ip$
	Field player
	Field map
	Field iscore
End Type
Global glist.tGList

Type tHTTP
	Field url$									; URL des Scriptservers
	Field port									; Port
	Field script$								; URL des Scripts
	Field host$									; Host des Scriptservers
End Type
Global sendit.tHTTP=New tHTTP

Type tGame
	Field name$
	Field desc$
	Field pwd$
	Field iscore
	Field ip$
	Field player
End Type
Global mp_game.tGame=New tGame
mp_game\player=1
mp_game\iscore=0

Type tGlobal
	Field username$
End Type
Global mp_users.tGlobal

localhost=True

If Not localhost
	sendit\url$="www.goto-xcellence.de"
	sendit\port=80
	sendit\script$="/bnb/connect.php"
	sendit\host$="www.goto-xcellence.de"
Else
	sendit\url$="localhost"
	sendit\port=80
	sendit\script$="/bnb/connect.php"
	sendit\host$="localhost"
EndIf

Global mp_gamecreated=False		; Spiel erstellt
Global mp_gameconnected=False	; mit Spiel verbunden
Global mp_gamestarttime			; Startzeit des Spiels

Global mp_selected_name$		; Name des gewaehlten Spiels
Global mp_selected_ip$			; IP des gewaehlten Spiels
Global mp_selected_desc$		; Beschreibung des geawehlten Spiels
Global mp_selected_iscore		; i.Score Flag des gewaehlten Spiels
Global mp_selected_pwd$			; Passwort des gewaehlten Spiels
Global mp_nickname$				; Name des Spielers

;-----------------------------------------------------------
; Globale Imagehandles
Global browser
Global mp_newgame
Global mp_newlist
Global mp_connect
Global mp_pointer
Global mp_busy
Global mp_busy_frame
Global mp_busy_time
Global mp_pcreategame
Global mp_cancel
Global mp_ok
Global mp_check
Global mp_error
Global mp_agree
Global mp_gamechat
Global mp_ready
Global mp_quit

;-----------------------------------------------------------
; GUI Variablen fuer den Gamebrowser

Global mp_ctime					; Cursorzeit
Global mp_cflag					; Cursorblinken
Global mp_textfield=1			; akt. Textfeld
Global mp_cursor=True			; schaltet Cursor ein/aus
Global mp_sel_game=-1			; ausgewaehltes Spiel
Global mp_ready_flag=False		; gibt an ob "Bereit" angeklickt wurde
Global mpfont
Global detailfont