; Intro
.intro
	PAKDePak("dat\gfx\xc.bnb",1)
	ground_coll	=1
	sign_coll	=2
	ball_coll	=3
	
	Collisions ball_coll,ground_coll,2,2
	Collisions ball_coll,sign_coll,2,2
	
	introcam=CreateCamera()
	introlight1=CreateLight(2)

	PositionEntity introcam,0,1,-13
	PositionEntity introlight1,0,10,-10
	
	LightRange introlight1,20
	
	;----------
	; Letters
	xctex=LoadTexture(PAKLoadFile("xctex.bmp"))
	xcenv=LoadTexture(PAKLoadFile("xcenv.bmp"),64)
	
	TextureBlend xctex,1
	
	objcount=9
	Dim xc_msh(objcount)
	Dim xc_ry(objcount)
	Dim xc_isturn(objcount)
	
	For i=0 To objcount
		xc_ry(i)=0
		xc_isturn(i)=0
	Next
	
	xc_msh(0)=LoadMesh(PAKLoadFile("x.3ds"))
	xc_msh(1)=LoadMesh(PAKLoadFile("minus.3ds"))
	xc_msh(2)=LoadMesh(PAKLoadFile("c.3ds"))
	xc_msh(3)=LoadMesh(PAKLoadFile("e.3ds"))
	xc_msh(4)=LoadMesh(PAKLoadFile("l.3ds"))
	xc_msh(5)=CopyEntity(xc_msh(4))
	xc_msh(6)=CopyEntity(xc_msh(3))
	xc_msh(7)=LoadMesh(PAKLoadFile("n.3ds"))
	xc_msh(8)=CopyEntity(xc_msh(2))
	xc_msh(9)=CopyEntity(xc_msh(3))
		
	For i=0 To objcount
		EntityTexture xc_msh(i),xctex,0,0
		EntityTexture xc_msh(i),xcenv,0,1
		EntityFX xc_msh(i),1
		EntityShininess xc_msh(i),1
		ScaleEntity xc_msh(i),0.2,0.2,0.2
		RotateEntity xc_msh(i),0,-90,0
		PositionEntity xc_msh(i),(-9)+(i*2),0.2,0
		EntityType xc_msh(i),sign_coll
	Next
	
	;PointEntity introcam,xc_msh(5)
	
	;----------
	; Mirror
	ground01=CreatePlane()
	EntityColor ground01,0,0,0
	EntityType ground01,ground_coll

	;----------
	; Sprites
	intro_red=LoadSprite(PAKLoadFile("spark01.bmp"))
	intro_blue=LoadSprite(PAKLoadFile("spark02.bmp"))
	
	ScaleSprite intro_red,0.5,0.5
	ScaleSprite intro_blue,0.5,0.5
	
	PositionEntity intro_red,0,0,-50
	PositionEntity intro_blue,0,0,-50
	
	HideEntity intro_red
	HideEntity intro_blue
	
	;----------
	; Ball
	intro_ball=CreateSphere(12)
	intro_envtex=LoadTexture(PAKLoadFile("env.jpg"),64)
	intro_ball_tex=LoadTexture(PAKLoadFile("squares.jpg"))
	
	ScaleEntity intro_ball,0.5,0.5,0.5
	PositionEntity intro_ball,-17,0.5,-0.5

	EntityType intro_ball,ball_coll
	EntityTexture intro_ball,intro_ball_tex,0,0
	EntityShininess intro_ball,1
	EntityTexture intro_ball,intro_envtex,0,1
	EntityFX intro_ball,1
	
	;----------
	; Sparks
	For i=0 To objcount
		CreateSparks(15,EntityX(xc_msh(i)),EntityY(xc_msh(i)),EntityZ(xc_msh(i))-1,intro_red)
		CreateSparks(15,EntityX(xc_msh(i)),EntityY(xc_msh(i)),EntityZ(xc_msh(i))-1,intro_blue)
	Next
	
	;----------
	; Sound
	signhit=LoadSound("dat\sfx\blockhit01.wav")
	
	second_old=MilliSecs()
	second=0
	t=MilliSecs()
	fps=30
	
	;PlayMusic("dat\sfx\mus01.s3m")
	
	Delay 1000
	
	zaehler=0
	fullfps=0
	startzeit=MilliSecs()
	;----------
	; Main
	Repeat
		If KeyHit(1) Then Exit
		If MouseHit(1) Then Exit
		
		If fps<=30
			endsek=10
			ballspeed#=0.3
		Else
			endsek=7
			ballspeed#=0.2
		EndIf
		
		If second=endsek Then Exit
		
		If MilliSecs()-second_old>=1000
			second=second+1
			second_old=MilliSecs()
		EndIf
		
		UpdateSparks()
		
		If second>0	
			If EntityX(intro_ball)<17
				MoveEntity intro_ball,ballspeed#,0,0
				RotateEntity intro_ball,0,0,0
    			TurnEntity intro_ball,0,0,-9
    			RotateMesh intro_ball,EntityPitch(intro_ball),EntityYaw(intro_ball),EntityRoll(intro_ball)
				If EntityY(intro_ball)<0.5 Or EntityZ(intro_ball)<(-1.5)
					PositionEntity intro_ball,EntityX(intro_ball),0.5,-1.5
				EndIf
			EndIf
		
			coll_count=CountCollisions(intro_ball)
			If coll_count
				collided=EntityCollided(intro_ball,sign_coll)
				If collided
					out_sound=signhit
					For i=0 To objcount
						If collided=xc_msh(i)
							xc_isturn(i)=1
							CreateSparks(3,EntityX(xc_msh(i)),EntityY(xc_msh(i)),EntityZ(xc_msh(i))-1,intro_red)
							CreateSparks(3,EntityX(xc_msh(i)),EntityY(xc_msh(i)),EntityZ(xc_msh(i))-1,intro_blue)
						EndIf
					Next
				EndIf
			EndIf
		
			For i=0 To objcount
				If xc_isturn(i)=1
					TurnEntity xc_msh(i),0,3,0
					xc_ry(i)=xc_ry(i)+3
					If xc_ry(i)>=90 Then xc_isturn(i)=0
				EndIf
			Next
		EndIf
		
		If out_sound>0 And ChannelPlaying(outchannel)=False Then outchannel=PlaySound(out_sound)
		out_sound=0
		
		UpdateWorld() : RenderWorld()
		
		If MilliSecs()-t=>1000 Then
   			t=MilliSecs()
   			fps=fpstmp
  			fpstmp=0
 		End If
 		fpstmp=fpstmp+1
		fullfps=fullfps+fps
		zaehler=zaehler+1

		Flip
	Until MilliSecs()-startzeit>10000
	fps=fullfps/zaehler
	;----------
	; FreeMem
	FreeEntity introcam
	FreeEntity introlight1
	FreeEntity intro_red
	FreeEntity intro_blue
	FreeEntity intro_ball
	FreeEntity ground01

	For i=0 To objcount : FreeEntity xc_msh(i) : Next
	
	FreeTexture xctex
	FreeTexture xcenv
	FreeTexture intro_envtex
	FreeTexture intro_ball_tex
	
	For spark.tSpark=Each tSpark
		Delete spark
	Next
	
	Cls
	tx=(scr\w/2)-((Len(loadingtxt$)*char_w)/2)
	ty=(scr\h/2)-(char_h/2)
	WriteBitmapFont(tx,ty,loadingtxt$)
	Flip
Return