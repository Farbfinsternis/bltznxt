; PARTICLE PARTY INCLUDE
;
Global pno

Type party
	Field x#,y#,z#,ax#,ay#,az,vx#,vy#,vz#,gravity#,roll#,vroll#
	Field life#,delaytime#,fadebias#,scalefactor#,scale#	
	Field ent
End Type

Function spawn(x#,y#,z#,scale#,scalefactor#,life#,fadebias#,vx#,vy#,vz#,vroll#,gravity#)
	p.party=New party
	p\ent=CreateSprite()
	EntityBlend p\ent,3
	p\life=life
	p\vx=vx
	p\vy=vy
	p\gravity=gravity
	p\fadebias=fadebias
	p\vroll=vroll
	p\scalefactor=scalefactor
	p\scale=scale
	PositionEntity p\ent,x,y,z
	ScaleSprite p\ent,scale,scale
	Return p\ent
End Function

Function updateparty()
	pno=0
	For p.party=Each party
		If p\life=0
			FreeEntity p\ent
			Delete p
		Else
			pno=pno+1
			p\life=p\life-1
			p\vy=p\vy+p\gravity
			p\roll=p\roll+p\vroll
			p\scale=p\scale+p\scalefactor
			If p\scale<0 Then p\scale=0
			ScaleSprite p\ent,p\scale,p\scale
			EntityAlpha p\ent,p\life*p\fadebias
			RotateSprite p\ent,p\roll
			MoveEntity p\ent,p\vx,p\vy,p\vz

		EndIf
	Next
End Function