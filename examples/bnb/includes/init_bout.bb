; Objekte initialisieren
;-----------------------------------------------------------
; Game Variablen
Global speed#=0.2											; Ballgeschwindigkeit
Global start_speed#=0.2
Global old_speed#=0.0
Global scorepoints=0										; Punktezahl
Global balls=5												; Anzahl der Baelle
Global levelstart=True										; Levelstart Flag
Global won=False											; gewonnen
Global lose=False											; verloren
Global block_count=0										; Anzahl der Bloecke
Global level=1												; aktuelles Level
Global diffc=0												; Schwierigkeitsstufe
Global starttime											; Startzeit des Levels
Global endtime												; Endzeit des Levels
Global hear=True											; BG-Music Flag
Global energie#=0.0											; Speed-Multiplier
Global gravity#=0.5											; Gravitation
Global messagetime=0										; MSec-Zaehler fuer Messages
Global message$=""
Global spheresegs=6
Global leveltime=180
Global music_play
Global music$
Global game_esc=False
Global bc
Global playername$=""
Global gamekey=214

Global dimg=LoadImage("dat\gfx\gfx\demotxt.bmp")

busy=LoadAnimImage("dat\gfx\gfx\busy.png",200,150,0,16)
busyframe=0
; PAK-Files entpacken
Global arial=LoadFont("Arial",18,1,0,0) : SetFont arial
Global loadingpic
Select scr\w
	Case 640
		loadingpic=LoadImage("dat\gfx\gfx\loading.jpg") : MaskImage loadingpic,255,0,255
	Case 800
		loadingpic=LoadImage("dat\gfx\gfx\loading800.jpg") : MaskImage loadingpic,255,0,255
	Case 1024
		loadingpic=LoadImage("dat\gfx\gfx\loading1024.jpg") : MaskImage loadingpic,255,0,255
	Default
		loadingpic=LoadImage("dat\gfx\gfx\loading.jpg") : MaskImage loadingpic,255,0,255
End Select

; Bitmaps entpacken
Cls
DrawImage loadingpic,0,0
If demoflag=True Then DrawImage dimg,5,5

Color 255,255,255
Text scr\w/2-100,(scr\h/2)-50,"...bitmaps"
Flip
;PAKDePak("dat\gfx\gfx.bnb",131)

; Sprites entpacken
Cls
DrawImage loadingpic,0,0
If demoflag=True Then DrawImage dimg,5,5

Color 255,255,255
Text scr\w-100,(scr\h/2)-50,"...sprites"
Flip
PAKDePak("dat\gfx\spr.bnb",10)

; Texturen entpacken
Cls
DrawImage loadingpic,0,0
If demoflag=True Then DrawImage dimg,5,5

Color 255,255,255
Text scr\w-100,(scr\h/2)-50,"...textures"
Flip
PAKDePak("dat\gfx\tex.bnb",180)

; Meshes entpacken
Cls
DrawImage loadingpic,0,0
If demoflag=True Then DrawImage dimg,5,5

Color 255,255,255
Text scr\w-100,(scr\h/2)-50,"...meshes"
Flip
PAKDePak("dat\gfx\msh.bnb",231)

;----------
; Bitmaptext Globals

Global font$="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
font$= font$+"0123456789ƒ÷‹‰ˆ¸ﬂ !"+Chr(34)+"$%&/"
font$= font$+"([{}])=?\'*+~#-_:.,;<>|@"
Global font_len=Len(font$)
Global char_w=13
Global char_h=13
Global fontimg=LoadAnimImage("dat\gfx\gfx\fnt13spx.bmp",char_w,char_h,0,font_len)

;---------
; Bonusvariablen
; Multiball
Global multiball=-1											; Multiball Flag
Dim multiballs(6)											; sechs moegl. Baelle
Dim multiballs_shadow(6)									; plus 6 Schatten
Dim mb_dir_x#(6)											; X-Vektoren
Dim mb_dir_z#(6)											; Y-Vektoren
Dim mb_energie#(6)

Const BONUS_EXTRABALL		= 255
Const BONUS_SPEEDBALL		= 256
Const BONUS_ATOMICBALL		= 257
Const BONUS_MULTI1			= 258
Const BONUS_MULTI3			= 259
Const BONUS_MULTI5			= 260
Const BONUS_PDLOOPS			= 261
Const BONUS_TINYBALL		= 262
Const BONUS_NEWBLOX			= 263
Const BONUS_WPADDLE			= 264
Const BONUS_SLOWDOWN		= 265
Const BONUS_BIGBALL			= 266
Const BONUS_EXTRASCORE		= 267
Const BONUS_TIMEPLUS		= 268
Const BONUS_TIMEMIN			= 269

Global pdloops_time=0
Global tinyball_time=0
Global wpaddle_time=0
Global wpaddle_count=0
Global ball_slowdown=0
Global bigball_time=0
Global extrascore_time=0

;If Instr(cmd$,"noenvmapping")>0 Then envmapping=False		; EnvMapping ausschalten
;If Instr(cmd$,"nomirror")>0 Then nomirror=True
;If Instr(cmd$,"nomusic")>0 Then hear=False					; Music abschalten

;-----------------------------------------------------------
; Inventar
Dim item$(100)
Dim item_num(100)
Global curr_item
Global last_item

;-----------------------------------------------------------
; Shop
Type tShop
	Field img
	Field x,y,w,h
	Field name$
	Field price
	Field num
End Type
Global shopentries.tShop

shopentries.tShop=New tShop
shopentries\img=LoadImage("dat\gfx\gfx\shop_item_atomic.bmp")
shopentries\x=((scr\w/2)-320)+19
shopentries\y=((scr\h/2)-240)+108
shopentries\w=240
shopentries\h=16
shopentries\name$="ATOMICBALL"
shopentries\price=30000
shopentries\num=1

shopentries.tShop=New tShop
shopentries\img=LoadImage("dat\gfx\gfx\shop_item_extraball.bmp")
shopentries\x=((scr\w/2)-320)+19
shopentries\y=((scr\h/2)-240)+140
shopentries\w=240
shopentries\h=16
shopentries\name$="EXTRABALL"
shopentries\price=40000
shopentries\num=2

shopentries.tShop=New tShop
shopentries\img=LoadImage("dat\gfx\gfx\shop_item_15secs.bmp")
shopentries\x=((scr\w/2)-320)+19
shopentries\y=((scr\h/2)-240)+172
shopentries\w=240
shopentries\h=16
shopentries\name$="TIME +15SECS"
shopentries\price=20000
shopentries\num=3

shopentries.tShop=New tShop
shopentries\img=LoadImage("dat\gfx\gfx\shop_item_widepaddle.bmp")
shopentries\x=((scr\w/2)-320)+19
shopentries\y=((scr\h/2)-240)+204
shopentries\w=240
shopentries\h=16
shopentries\name$="WIDEPADDLE"
shopentries\price=15000
shopentries\num=4

shopentries.tShop=New tShop
shopentries\img=LoadImage("dat\gfx\gfx\shop_item_slowball.bmp")
shopentries\x=((scr\w/2)-320)+19
shopentries\y=((scr\h/2)-240)+236
shopentries\w=240
shopentries\h=16
shopentries\name$="SLOWMOTION"
shopentries\price=17000
shopentries\num=5

;-----------------------------------------------------------
; Highscores
Dim scorebars_y(10)

Dim scorenames$(10)
Dim scorevalues(10)
Dim scorelevel(10)

For i=0 To 9
	scorenames$(i)=""
	scorevalues(i)=0
	scorelevel(i)=0
Next

;-----------------------------------------------------------

Global balls_img=LoadImage("dat\gfx\gfx\ballcount.bmp") : MaskImage balls_img,0,0,0
Global panel_img=LoadImage("dat\gfx\gfx\panel.bmp") : MaskImage panel_img,0,0,50
Global getready=LoadImage("dat\gfx\gfx\getready.bmp")
Global nextstage=LoadImage("dat\gfx\gfx\nextstage.bmp")
Global timeover=LoadImage("dat\gfx\gfx\timeover.bmp")

; Tueren laden
Global dl_w,dr_w,dl_x,dr_x
Select scr\w
	Case 640
		door_left=LoadImage("dat\gfx\gfx\dl480.png") : MaskImage door_left,0,0,50
		door_right=LoadImage("dat\gfx\gfx\dr480.png") : MaskImage door_right,0,0,50
		dl_w=ImageWidth(door_left)
		dr_w=ImageWidth(door_right)
		dl_x=-dl_w
		dr_x=scr\w+dr_w
		
	Case 800
		door_left=LoadImage("dat\gfx\gfx\dl600.png") : MaskImage door_left,0,0,50
		door_right=LoadImage("dat\gfx\gfx\dr600.png") : MaskImage door_right,0,0,50
		dl_w=ImageWidth(door_left)
		dr_w=ImageWidth(door_right)
		dl_x=-dl_w
		dr_x=scr\w+dr_w

	Case 1024
		door_left=LoadImage("dat\gfx\gfx\dl768.png") : MaskImage door_left,0,0,50
		door_right=LoadImage("dat\gfx\gfx\dr768.png") : MaskImage door_right,0,0,50
		dl_w=ImageWidth(door_left)
		dr_w=ImageWidth(door_right)
		dl_x=-dl_w
		dr_x=scr\w+dr_w
		
	Default
		door_left=LoadImage("dat\gfx\gfx\dl480.png") : MaskImage door_left,0,0,50
		door_right=LoadImage("dat\gfx\gfx\dr480.png") : MaskImage door_right,0,0,50
		dl_w=316
		dr_w=375
		dl_x=-dl_w
		dr_x=scr\w+dr_w
		
End Select
Global door_snd=LoadSound("dat\sfx\door.wav")

;-----------------------------------------------------------
; Funken
Type tSpark
	Field spark
	Field x#,y#,z#
	Field alpha#
	Field dx#,dy#,dz#
	Field typ
End Type
Global spark.tSpark

;-----------------------------------------------------------
; Rauch
Type tSmoke
	Field smoke
	Field x#,y#,z#
	Field alpha#
	Field dx#,dz#
End Type
Global smk.tSmoke

;-----------------------------------------------------------
; 3D Score
Type tScore3D
	Field pnt
	Field x#,y#,z#
	Field alpha#
End Type
Global pnts.tScore3D

;-----------------------------------------------------------
; Blitz
Type tBlitz
	Field sfx_x#
	Field sfx_y#
	Field sfx_z#
	Field zx#,zy#,zz#
	Field alpha#
	Field lightning
	Field coll_entity
End Type
Global sfx_blitz.tBlitz

Global sfx_piv=CreatePivot()
EntityRadius sfx_piv,2
EntityPickMode sfx_piv,1

;-----------------------------------------------------------
; Kameras
Type tCamera
	Field x#,y#,z#
	Field cam
	Field rx#,ry#,rz#
End Type
Global cam.tCamera

;-----------------------------------------------------------
; Lichter
Type tLight
	Field x#,y#,z#
	Field light
	Field r,g,b
	Field range
End Type
Global light.tLight

;-----------------------------------------------------------
; Blockmap

Global block=LoadMesh(PAKLoadFile("block.3ds"))
Global altblock=LoadMesh(PAKLoadFile("hedra.3ds"))

Global envtex=LoadTexture(PAKLoadFile("env.jpg"),64)

Global altblocktex=LoadTexture(PAKLoadFile("hedra.jpg"))

If envmapping=True Then TextureBlend envtex,3
TextureBlend altblocktex,1

ScaleMesh block,0.2,0.2,0.2
ScaleMesh altblock,0.2,0.2,0.2

EntityPickMode block,3
EntityPickMode altblock,3

HideEntity block
HideEntity altblock

Global map_w=14
Global map_h=14

Dim add_obj(map_w,map_h)

Dim map(map_w,map_h)										; Mapdaten
Dim block_msh(map_w,map_h)									; Meshhandles
Dim block_col(map_w,map_h)									; Blockfarbe
Dim block_typ(map_w,map_h)									; normal oder Bonus
Dim block_alp#(map_w,map_h)									; Blockalpha
Dim block_x#(map_w,map_h)									; Block X Koordinate
Dim block_y#(map_w,map_h)									; Block Y Koordinate
Dim block_z#(map_w,map_h)									; Block Z Koordinate
Dim block_tex(map_w,map_h)

Dim blocktex(100)											; Block Texturen
Global max_tex=16											; max. Anzahl der Text.
For i=0 To max_tex
	blocktex(i)=LoadTexture(PAKLoadFile("blocktex"+Str(i)+".jpg"))
	TextureBlend blocktex(i),1
Next

;-----------------------------------------------------------
; Paddle
Global pdl=LoadMesh(PAKLoadFile("paddle.3ds"))
Global pdltex=LoadTexture(PAKLoadFile("paddle.bmp"))

EntityTexture pdl,pdltex,0,0
EntityShininess pdl,1
ScaleMesh pdl,0.1,0.1,0.1
PositionEntity pdl,0,0,0

Global pdl_orig#=MeshWidth(pdl)

;-----------------------------------------------------------

; Spielball
Global ball=CreateSphere(spheresegs)
Global mball=CreateSphere(spheresegs)
Global ball_tex=LoadTexture(PAKLoadFile("squares.jpg"))

NameEntity ball,"BALL"

EntityTexture ball,ball_tex,0,0
EntityTexture mball,ball_tex,0,0

If envmapping=True
	EntityTexture ball,envtex,0,1
	EntityFX ball,1
	EntityTexture mball,envtex,0,1
	EntityFX mball,1
EndIf

ScaleEntity ball,0.7,0.7,0.7
PositionEntity ball,EntityX(pdl),1,EntityZ(pdl)+MeshDepth(pdl)+4
ScaleEntity mball,0.7,0.7,0.7

; Variablen fuer den Ball
Global dir_x#=0.0												; Bewegungsrichtung X
Global dir_z#=0.0												; Bewegungsrichtung Z
Global bx#,by#,bz#

Global shadow=LoadSprite(PAKLoadFile("ballshadow.tga"),3)		; Ball-Schatten
ScaleSprite shadow,0.8,0.8
SpriteViewMode shadow,2
EntityAlpha shadow,0.5
RotateEntity shadow,90,0,0
HideEntity shadow
HideEntity mball

;-----------------------------------------------------------
; Tisch

Global tbtx1=LoadTexture(PAKLoadFile("table.bmp"))			; Tabletexture 1
Global tbl01=LoadMesh(PAKLoadFile("tableborder2.3ds"))		; Tablerand
Global tbl02=LoadMesh(PAKLoadFile("tableground2.3ds"),tbl01)	; Tableboden

EntityTexture tbl02,tbtx1
EntityTexture tbl01,tbtx1

ScaleMesh tbl01,0.2,0.2,0.2
ScaleMesh tbl02,0.2,0.2,0.2

If nomirror=False
	EntityAlpha tbl02,0.5
	mirror=CreateMirror()
EndIf

;-----------------------------------------------------------
; Kamera erzeugen
cam.tCamera = New tCamera

cam\cam		= CreateCamera()
cam\x		= 0.0
cam\y		= 20.10
cam\z		= -10.0
cam\rx		= 31.50

CameraClsMode cam\cam,False,True
CameraZoom cam\cam,1.5
PositionEntity cam\cam,cam\x,cam\y,cam\z
RotateEntity cam\cam,cam\rx,0,0

;----------
; 3D Overlay
Global guicam=CreateCamera()
PositionEntity guicam,0,0,0.5

CameraClsMode guicam,False,True
CameraRange guicam,.1,1000 
Global overlay=CreatePivot() 
aspect#=Float(scr\h)/scr\w 
PositionEntity overlay,-1,aspect,1.5
scale#=2.0/scr\w 
ScaleEntity overlay,scale,-scale,-scale

Global ballcam=CreateCamera()
Global bcamimg=LoadImage("dat\gfx\gfx\ballcam.bmp")
Global ballcam_x=scr\w
Global ballcam_y=100
Global ballcam_mode=0			; 0=schliessen, 1=oeffnen
CameraViewport ballcam,0,0,0,0

;-----------------------------------------------------------
; Licht erzeugen
light.tLight=New tLight

light\light	= CreateLight(2)
light\x		= EntityX(tbl02)
light\y		= 100
light\z		= 50
light\range = 100

PositionEntity light\light,light\x,light\y,light\z
LightRange light\light,light\range

light.tLight=New tLight

light\light	= CreateLight(2)
light\x		= EntityX(pdl)
light\y		= 40
light\z		= EntityZ(pdl)
light\range = 40

PositionEntity light\light,light\x,light\y,light\z
LightRange light\light,light\range

AmbientLight 150,150,150

;-----------------------------------------------------------
; Collisions festlegen
Const COLL_BLOCK		= 1
Const COLL_BALL			= 2
Const COLL_WALL			= 3
Const COLL_GROUND		= 4
Const COLL_PADDLE		= 5
Const COLL_BUMPER		= 6
Const COLL_MBALL		= 7
Const COLL_SFX			= 8

EntityType tbl01,COLL_WALL
EntityType tbl02,COLL_GROUND
EntityType pdl,COLL_PADDLE
EntityType ball,COLL_BALL
EntityType block,COLL_BLOCK
EntityType altblock,COLL_BLOCK
EntityType mball,COLL_MBALL
EntityType sfx_piv,COLL_SFX

Collisions COLL_SFX,COLL_BLOCK,2,3
Collisions COLL_BALL,COLL_BLOCK,2,3
Collisions COLL_BALL,COLL_WALL,2,3
Collisions COLL_BALL,COLL_PADDLE,2,3
Collisions COLL_PADDLE,COLL_WALL,2,1
Collisions COLL_BALL,COLL_GROUND,2,3
Collisions COLL_BALL,COLL_BUMPER,2,3

Collisions COLL_MBALL,COLL_BLOCK,2,3
Collisions COLL_MBALL,COLL_WALL,2,3
Collisions COLL_MBALL,COLL_PADDLE,2,3
Collisions COLL_MBALL,COLL_GROUND,2,3
Collisions COLL_MBALL,COLL_BUMPER,2,3
Collisions COLL_BALL,COLL_MBALL,2,3
Collisions COLL_MBALL,COLL_BALL,2,3

;-----------------------------------------------------------
; Sound und Gfx-FX
; Sprites laden
Global spark_red=LoadSprite(PAKLoadFile("spark01.bmp"))		; Funken rot
Global spark_blue=LoadSprite(PAKLoadFile("spark02.bmp"))	; Funken blau
Global spark_star=LoadSprite(PAKLoadFile("spark03.bmp"))
Global spark_rainbow=LoadSprite(PAKLoadFile("spark04.bmp"))
Global spark_green=LoadSprite(PAKLoadFile("spark05.bmp"))
Global smoke=LoadSprite(PAKLoadFile("smoke01.bmp"))			; Ball-Rauch
Global paddle_coll=LoadSprite(PAKLoadFile("blue.tga"),3)
Global spark_verlauf=LoadSprite(PAKLoadFile("strudel.tga"),3)

Global smallspark=LoadSprite("dat\gfx\gfx\smallspark.bmp")

Global pnt10min=LoadSprite(PAKLoadFile("pnt-10.bmp"))		; -10 Punkte
Global pnt1=LoadSprite(PAKLoadFile("pnt1.bmp"))				; 1 Punkt
Global pnt2=LoadSprite(PAKLoadFile("pnt2.bmp"))				; 2 Punkte
Global pnt10=LoadSprite(PAKLoadFile("pnt10.bmp"))			; 10 Punkte
Global pnt15=LoadSprite(PAKLoadFile("pnt15.bmp"))			; 15 Punkte
Global pnt20=LoadSprite(PAKLoadFile("pnt20.bmp"))			; 20 Punkte
Global pnt30=LoadSprite(PAKLoadFile("pnt30.bmp"))			; 30 Punkte
Global pnt50=LoadSprite(PAKLoadFile("pnt50.bmp"))			; 50 Punkte
Global pnt75=LoadSprite(PAKLoadFile("pnt75.bmp"))			; 75 Punkte
Global pnt100=LoadSprite(PAKLoadFile("pnt100.bmp"))			; 100 Punkte
Global pnt150=LoadSprite(PAKLoadFile("pnt150.bmp"))			; 150 Punkte
Global pnt200=LoadSprite(PAKLoadFile("pnt200.bmp"))			; 200 Punkte

Global spr_speedball=LoadSprite(PAKLoadFile("speedball.bmp"))
Global spr_extraball=LoadSprite(PAKLoadFile("extraball.bmp"))
Global spr_atomicball=LoadSprite(PAKLoadFile("atomicball.bmp"))
Global multi1=LoadSprite(PAKLoadFile("multiball1.bmp"))
Global multi3=LoadSprite(PAKLoadFile("multiball3.bmp"))
Global multi5=LoadSprite(PAKLoadFile("multiball5.bmp"))
Global pdloops=LoadSprite(PAKLoadFile("oops.bmp"))
Global tinyball=LoadSprite(PAKLoadFile("tiny.bmp"))
Global blockrespawn=LoadSprite(PAKLoadFile("newblox.bmp"))
Global widepaddle=LoadSprite(PAKLoadFile("widepaddle.bmp"))
Global slowdown=LoadSprite(PAKLoadFile("slowdown.bmp"))
Global bigball=LoadSprite(PAKLoadFile("bigball.bmp"))
Global extrascore=LoadSprite(PAKLoadFile("extrascore.bmp"))
Global moretime=LoadSprite(PAKLoadFile("moretime.bmp"))
Global nomoretime=LoadSprite(PAKLoadFile("nomoretime.bmp"))

HideEntity spark_red
HideEntity spark_blue
HideEntity spark_star
HideEntity spark_rainbow
HideEntity smoke
HideEntity paddle_coll
HideEntity spark_verlauf
HideEntity spark_green

HideEntity smallspark

HideEntity pnt10min
HideEntity pnt1
HideEntity pnt2
HideEntity pnt15
HideEntity pnt20
HideEntity pnt30
HideEntity pnt50
HideEntity pnt75
HideEntity pnt100
HideEntity pnt150
HideEntity pnt200

HideEntity spr_speedball
HideEntity spr_extraball
HideEntity spr_atomicball
HideEntity multi1
HideEntity multi3
HideEntity multi5
HideEntity pdloops
HideEntity tinyball
HideEntity blockrespawn
HideEntity widepaddle
HideEntity slowdown
HideEntity bigball
HideEntity extrascore
HideEntity moretime
HideEntity nomoretime

; Sprites skalieren
ScaleSprite spark_red,0.8,0.8
ScaleSprite spark_star,0.2,0.2
ScaleSprite spark_rainbow,0.2,0.2
ScaleSprite spark_green,0.2,0.2
ScaleSprite smoke,0.6,0.6
ScaleSprite paddle_coll,0.5,0.5

ScaleSprite smallspark,0.2,0.2

ScaleSprite pnt1,2,1
ScaleSprite pnt2,2,1
ScaleSprite pnt10,2,1
ScaleSprite pnt15,2,1
ScaleSprite pnt20,2,1
ScaleSprite pnt30,2,1
ScaleSprite pnt10min,2,1
ScaleSprite pnt50,2,1
ScaleSprite pnt75,2,1
ScaleSprite pnt100,2,1
ScaleSprite pnt150,2,1
ScaleSprite pnt200,2,1

ScaleSprite spr_extraball,7,1
ScaleSprite spr_speedball,7,1
ScaleSprite spr_atomicball,7,1
ScaleSprite multi1,7,1
ScaleSprite multi3,7,1
ScaleSprite multi5,7,1
ScaleSprite pdloops,7,1
ScaleSprite tinyball,7,1
ScaleSprite blockrespawn,7,1
ScaleSprite widepaddle,7,1
ScaleSprite slowdown,7,1
ScaleSprite bigball,7,1
ScaleSprite extrascore,7,1
ScaleSprite moretime,7,1
ScaleSprite nomoretime,7,1

; Viewmode setzen
SpriteViewMode spark_red,1
SpriteViewMode spark_blue,1
SpriteViewMode spark_star,1
SpriteViewMode spark_rainbow,1
SpriteViewMode spark_green,1
SpriteViewMode smoke,1

SpriteViewMode pnt10min,1
SpriteViewMode pnt10,1
SpriteViewMode pnt15,1
SpriteViewMode pnt50,1
SpriteViewMode pnt75,1
SpriteViewMode pnt100,1

PositionEntity pnt10min,0,100,-100
PositionEntity pnt10,0,100,-100
PositionEntity pnt15,0,100,-100
PositionEntity pnt50,0,100,-100
PositionEntity pnt75,0,100,-100
PositionEntity pnt100,0,100,-100

;-----------------------------------------------------------
; Sounds
Global blockhit01=LoadSound("dat\sfx\blockhit01.wav")
Global blockhit02=LoadSound("dat\sfx\punch1.wav")
Global paddlehit01=LoadSound("dat\sfx\paddlehit01.wav")
Global lose_snd=LoadSound("dat\sfx\lose.wav")
Global win_snd=LoadSound("dat\sfx\win.wav")
Global bumper_snd=LoadSound("dat\sfx\bumper.wav")
Global boing=LoadSound("dat\sfx\boing.wav")
Global respawn=LoadSound("dat\sfx\loseball.wav")
Global shopentry=LoadSound("dat\sfx\shopentry.wav")
Global bonussnd=LoadSound("dat\sfx\bonuspoints.wav")
Global bomb=LoadSound("dat\sfx\bomb.wav")

SoundPan blockhit01,0
SoundPan paddlehit01,0
SoundPitch paddlehit01,33000
SoundVolume boing,0.5
SoundVolume respawn,1.0

;-----------------------------------------------------------
; Bonusobjekte
; Ball+50 - 50 Punkte
Global bonus_points=LoadMesh(PAKLoadFile("bonus_points2.3ds"))
Global bonus_points75=LoadMesh(PAKLoadFile("bonus_points75.3ds"))
Global bonus_points100=LoadMesh(PAKLoadFile("bonus_points100.3ds"))

HideEntity bonus_points
HideEntity bonus_points75
HideEntity bonus_points100

Global extraball=CreateSphere(12)
EntityTexture extraball,ball_tex,0,0
EntityTexture extraball,envtex,0,1
ScaleEntity extraball,0.9,0.9,0.9
HideEntity extraball

Global bumper=LoadMesh(PAKLoadFile("bumper.3ds"))
Global bumper_tex=LoadTexture(PAKLoadFile("bumper.bmp"))
EntityTexture bumper,bumper_tex
HideEntity bumper

Global blocker=LoadMesh(PAKLoadFile("blocker.3ds"))
Global blocker_tex=LoadTexture(PAKLoadFile("blocker.bmp"))
TextureBlend blocker_tex,1
EntityTexture blocker,blocker_tex
HideEntity blocker

Global blocker45=LoadMesh(PAKLoadFile("blocker45.3ds"))
EntityTexture blocker45,blocker_tex
HideEntity blocker45

Global blockerz=LoadMesh(PAKLoadFile("blockerz.3ds"))
EntityTexture blockerz,blocker_tex
HideEntity blockerz

Global wecker=LoadMesh(PAKLoadFile("wecker.3ds"))
Global wecker_tex=LoadTexture(PAKLoadFile("wecker.bmp"),4)
Global wecker_piv=CreatePivot()
EntityParent wecker,wecker_piv

EntityTexture wecker,wecker_tex
HideEntity wecker

ScaleMesh bonus_points,0.2,0.2,0.2
ScaleMesh bonus_points75,0.2,0.2,0.2
ScaleMesh bonus_points100,0.2,0.2,0.2

EntityFX bonus_points,1
EntityFX bonus_points75,1
EntityFX bonus_points100,1

ScaleMesh bumper,0.2,0.3,0.2
ScaleMesh blocker,0.2,0.2,0.2
ScaleMesh blocker45,0.2,0.2,0.2
ScaleMesh blockerz,0.2,0.2,0.2
ScaleMesh wecker,0.07,0.07,0.07

EntityAlpha bonus_points,0.7
EntityAlpha bonus_points75,0.7
EntityAlpha bonus_points100,0.7

EntityShininess bumper,1
EntityShininess wecker,0.5

EntityType bonus_points,COLL_BLOCK
EntityType bonus_points75,COLL_BLOCK
EntityType bonus_points100,COLL_BLOCK
EntityType extraball,COLL_BLOCK
EntityType bumper,COLL_BUMPER
EntityType blocker,COLL_WALL
EntityType blocker45,COLL_WALL
EntityType blockerz,COLL_WALL
EntityType wecker,COLL_BLOCK

;-----------------------------------------------------------
; Konfig
Type tButton
	Field x,y
	Field w,h
	Field num
End Type
Global gad.tButton

;-----------------------------------------------------------
; saved games
Dim games$(500)
Dim row(500)
Global flistgame=0

;-----------------------------------------------------------
; GUI FX
Global rad=LoadSprite(PAKLoadFile("rad.bmp"))
ScaleEntity rad,4,4,4
SpriteViewMode rad,2
EntityAlpha rad,0.05
EntityParent rad,overlay
PositionEntity rad,scr\w,scr\h,1
	
Global rad2=LoadSprite(PAKLoadFile("rad.bmp"))
ScaleEntity rad2,4,4,4
SpriteViewMode rad2,2
EntityAlpha rad2,0.05
EntityParent rad2,overlay
PositionEntity rad2,0,scr\h,1
	
Global mz3d=LoadMesh(PAKLoadFile("mz.3ds"))
Global mztx=LoadTexture(PAKLoadFile("mz.bmp"))
EntityTexture mz3d,mztx,0,0
EntityShininess mz3d,0.8
	
ScaleEntity mz3d,0.005,0.005,0.005
RotateEntity mz3d,0,0,45
EntityParent mz3d,overlay
	
Global mzlight=CopyEntity(smoke)
EntityParent mzlight,overlay
ScaleSprite mzlight,.08,.08
SpriteViewMode mzlight,2

;-----------------------------------------------------------
; i.Score Daten
Dim iscore_name$(10)		; Namen online
Dim iscore_score(10)		; Scores online
Dim iscore_level(10)		; Levels online
Dim iscore_skill(10)		; Skills online

Dim local_name$(10)			; Namen lokal
Dim local_score(10)			; Scores lokal
Dim local_level(10)			; Levels lokal
Dim local_skill(10)			; Skills lokal