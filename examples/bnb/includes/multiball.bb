.handle_multiball
	For i=0 To 6
		If multiballs(i)>0
		RotateEntity multiballs(i),0,0,0
    	TurnEntity multiballs(i),(mb_dir_z(i)*(speed+mb_energie(i)))*15,0,(-mb_dir_x(i)*(speed+mb_energie(i)))*15
    	RotateMesh multiballs(i),EntityPitch(multiballs(i)),EntityYaw(multiballs(i)),EntityRoll(multiballs(i))
	
		mbx#=EntityX(multiballs(i))
		mby#=EntityY(multiballs(i))
		mbz#=EntityZ(multiballs(i))
		MoveEntity multiballs(i),mb_dir_x(i)*(speed+mb_energie(i)),-gravity,mb_dir_z(i)*(speed+mb_energie(i))

		If mbz#<EntityZ(pdl) Or mby#<0
			FreeEntity multiballs(i)
			performancecheck=MilliSecs()
			multiballs(i)=0		
			multiball=multiball-1
			
			If extrascore_time<=0
				scorepoints=scorepoints-10
				CreateScore(-10,mbx#,mbz#)
			EndIf
		
			mb_dir_x(i)=0 : mb_dir_z(i)=0
			Exit
		EndIf
	
		If mb_energie(i)>-0.008 Then mb_energie(i)=mb_energie(i)-0.008
		coll_count=CountCollisions(multiballs(i))
		If coll_count>0
			coll_nx#=CollisionNX(multiballs(i),coll_count)
			coll_nz#=CollisionNZ(multiballs(i),coll_count)
		
			If coll_nx Then mb_dir_x(i)=coll_nx
			If coll_nz Then mb_dir_z(i)=coll_nz

			collided=EntityCollided(multiballs(i),COLL_BLOCK)
	
			If collided
				ss=0 : zz=0 : flag=0
				Repeat
					If collided=block_msh(ss,zz)
						coll_count=0 
						aa#=block_alp(ss,zz)
						aa#=aa#-0.5
						
						If kill_blox=1
							ns1=ss-2
							nz1=zz-2
							ns2=ss+2
							nz2=zz+2
							If ns1<=0 Then ns1=0
							If nz1<=0 Then nz1=0
							rs1=ns1
							If ns2>=map_w Then ns2=map_w
							If nz2>=map_h Then nz2=map_h
							While nz1<nz2
								If block_msh(ns1,nz1)>0
									If block_typ(ns1,nz1)<14 Or block_typ(ns1,nz1)>19
										FreeEntity block_msh(ns1,nz1)
										block_count=block_count-1
										block_msh(ns1,nz1)=0
										block_alp(ns1,nz1)=0
										block_typ(ns1,nz1)=-1
										CreateSparks(specialfx*2,block_x(ns1,nz1)-8,2,block_z(ns1,nz1)+30,spark_red)
										CreateSparks(specialfx*2,block_x(ns1,nz1)-8,2,block_z(ns1,nz1)+30,spark_blue)
									EndIf
								EndIf
								ns1=ns1+1
								If ns1=ns2
									ns1=rs1
									nz1=nz1+1
								EndIf
							Wend
							PlaySound bomb
							rumble=1
							rumble_time=MilliSecs()
							kill_blox=0
							item$(curr_item)=""
							last_item=last_item-1
							If curr_item+1>last_item Then curr_item=0
							CreateScore(200,bx,bz)
						EndIf
						
						block_alp(ss,zz)=aa#
					
						If aa<=0.0
							out_sound=blockhit01
							Select block_typ(ss,zz)
								Case 1
									If extrascore_time<=0
										CreateScore(15,mbx#,mbz#)
										scorepoints=scorepoints+15
									Else
										CreateScore(30,mbx#,mbz#)
										scorepoints=scorepoints+30
										Msg("Bonus +30")
										out_sound=bonussnd
									EndIf	
									block_count=block_count-1
								
								Case 2
									If extrascore_time<=0
										CreateScore(50,mbx#,mbz#)
										scorepoints=scorepoints+50
										out_sound=bonussnd
										Msg("Bonus +50")
									Else
										CreateScore(100,mbx#,mbz#)
										CreateScore(100,mbx#,mbz#)
										scorepoints=scorepoints+100
										out_sound=bonussnd
										Msg("Bonus +100")
									EndIf
									block_count=block_count-1
								
								Case 3
									If extrascore_time<=0
										CreateScore(75,mbx#,mbz#)
										scorepoints=scorepoints+75
										out_sound=bonussnd
										Msg("Bonus +75")
									Else
										CreateScore(150,mbx#,mbz#)
										scorepoints=scorepoints+150
										out_sound=bonussnd
										Msg("Bonus +150")
									EndIf
									block_count=block_count-1
								
								Case 4
									If extrascore_time<=0
										CreateScore(100,mbx#,mbz#)
										scorepoints=scorepoints+100
										out_sound=bonussnd
										Msg("Bonus +100")
									Else
										CreateScore(200,mbx#,mbz#)
										scorepoints=scorepoints+200
										out_sound=bonussnd
										Msg("Bonus +200")
									EndIf
									block_count=block_count-1							
								
								Case 8
									balls=balls+1
									CreateScore(BONUS_EXTRABALL,mbx#,mbz#)
									block_count=block_count-1
									Msg("EXTRABALL!")
								
								Case 9
									CreateScore(BONUS_ATOMICBALL,mbx#,mbz#)
									block_count=block_count-1
									Msg("ATOMICBALL!")
									item$(last_item)="ATOMICBALL"
									item_num(last_item)=9
									curr_item=last_item
									last_item=last_item+1
																	
								Case 10
									CreateScore(BONUS_SPEEDBALL,mbx#,mbz#)
									mb_energie(i)=2.0
									block_count=block_count-1
									Msg(">>Speedball>>")
								
								Case 11
									CreateScore(BONUS_NEWBLOX,mbx#,mbz#)
									block_count=block_count-1
								
									ns=ss-2 : nz=zz-2
									ns2=ss+2 : nz2=zz+2
									If ns<0 Then ns=0
									If nz<0 Then nz=0
								
									If ns2>map_w Then ns2=map_w
									If nz2>map_h Then nz2=map_h
								
									dd#=MeshDepth(block)
									ww#=MeshWidth(block)
									x1#=EntityX(block)+(ns*ww#) : xx1#=x1#
									y1#=EntityY(block)
									z1#=EntityZ(block)+(nz*dd#)
									zaehler=0							
									While nz<nz2
										btex=Rnd(0,max_tex)
										If block_msh(ns,nz)>0
											FreeEntity block_msh(ns,nz)
											block_msh(ns,nz)=0
											block_count=block_count-1
										EndIf
									
										block_msh(ns,nz)=CopyEntity(block)
										EntityTexture block_msh(ns,nz),blocktex(btex)
										block_tex(ns,nz)=blocktex(btex)
										EntityType block_msh(ns,nz),COLL_BLOCK
									
										block_alp(ns,nz)=1
										block_x#(ns,nz)=x1-4
										block_y#(ns,nz)=y1
										block_z#(ns,nz)=z1-20
										block_typ(ns,nz)=1
										PositionEntity block_msh(ns,nz),x1,y1,z1-27
										
										CreateSparks(1,x1#-8,y1#+1,z1#-40,spark_blue)
										CreateSparks(1,x1#-8,y1#+1,z1#-40,spark_red)
																		
										ns=ns+1 : x1#=x1#+ww# : block_count=block_count+1
										zaehler=zaehler+1
										If ns=ns2
											ns=ss-2
											If ns<0 Then ns=0
											nz=nz+1
											If nz>map_w Then Exit
											z1#=z1#+dd#
											x1#=xx1#
										EndIf
									Wend
									Msg(Str(zaehler-1)+" NEW BLOX!")
								
								Case 12
									CreateScore(BONUS_PDLOOPS,mbx#,mbz#)
									ScaleEntity pdl,0.7,1,1
									pdloops_time=MilliSecs()
									CreateSparks(specialfx,EntityX(pdl),EntityY(pdl),EntityZ(pdl)+2,spark_blue)
									block_count=block_count-1
									Msg("small paddle")
								
								Case 13
									CreateScore(BONUS_TINYBALL,mbx#,mbz#)
									ScaleMesh ball,0.5,0.5,0.5
									tinyball_time=MilliSecs()
									CreateSparks(specialfx*2,mbx#,mby#,mbz#,spark_rainbow)
									block_count=block_count-1
									Msg("Tinyball")
								
								Case 19
									CreateScore(BONUS_WPADDLE,mbx#,mbz#)
									ScaleMesh pdl,2,1,1
									wpaddle_time=MilliSecs()
									CreateSparks(specialfx*2,EntityX(pdl),EntityZ(pdl)+2,EntityZ(pdl),spark_rainbow)
									block_count=block_count-1
									Msg("monsterpaddle")
								
								Case 20
									CreateScore(BONUS_SLOWDOWN,mbx#,mbz#)
									old_speed#=start_speed#
									start_speed#=start_speed#/2
									ball_slowdown=MilliSecs()
									CreateSparks(specialfx*2,mbx#,mby#,bz#,spark_rainbow)
									block_count=block_count-1
									Msg("Slowdown!")
								
								Case 21
									CreateScore(BONUS_BIGBALL,mbx#,mbz#)
									ScaleMesh ball,2,2,2
									bigball_time=MilliSecs()
									CreateSparks(specialfx*2,mbx#,mby#,bz#,spark_rainbow)
									block_count=block_count-1
									Msg("Bowling!")
								
								Case 22
									CreateScore(BONUS_EXTRASCORE,mbx#,mbz#)
									extrascore_time=MilliSecs()
									CreateSparks(specialfx*2,mbx#,mby#,mbz#,spark_rainbow)
									block_count=block_count-1
									Msg("Multiple Score!")
								
								Case 23
									CreateScore(BONUS_TIMEPLUS,bx#,bz#)
									leveltime=leveltime+15
									CreateSparks(specialfx*2,bx#,by#,bz#,spark_red)
									block_count=block_count-1
									Msg("gimme one more time!")
								
								Case 24
									CreateScore(BONUS_TIMEMIN,bx#,bz#)
									leveltime=leveltime-15
									CreateSparks(specialfx*2,bx#,by#,bz#,spark_red)
									block_count=block_count-1
									Msg("No more Time!")
							End Select
						
							CreateSparks(specialfx,mbx#,2,mbz#,spark_red)
							CreateSparks(specialfx,mbx#,2,mbz#,spark_blue)
							If block_msh(ss,zz)>0 Then FreeEntity block_msh(ss,zz)
							block_msh(ss,zz)=0
							killed=1
						
							flag=1
						EndIf
					
						If aa#>0.0
							If killed=0
								If extrascore_time<=0
									CreateScore(10,mbx#,mbz#)
									scorepoints=scorepoints+10
								Else
									CreateScore(20,mbx#,mbz#)
									scorepoints=scorepoints+20
								EndIf
							EndIf
							out_sound=blockhit02
							If block_msh(ss,zz)>0 Then FreeEntity block_msh(ss,zz)
							block_msh(ss,zz)=0
						
							Select block_typ(ss,zz)
								Case 1
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5
							
								Case 2
									block_msh(ss,zz)=CopyEntity(bonus_points)
									EntityTexture block_msh(ss,zz),altblocktex
									EntityShininess block_msh(ss,zz),1
									PositionEntity block_msh(ss,zz),block_x(ss,zz)+6,block_y(ss,zz),block_z(ss,zz)-18.5
							
								Case 3
									block_msh(ss,zz)=CopyEntity(bonus_points75)
									EntityTexture block_msh(ss,zz),altblocktex
									EntityShininess block_msh(ss,zz),1
									PositionEntity block_msh(ss,zz),block_x(ss,zz)+6,block_y(ss,zz),block_z(ss,zz)-18.5
							
								Case 4
									block_msh(ss,zz)=CopyEntity(bonus_points100)
									EntityTexture block_msh(ss,zz),altblocktex
									EntityShininess block_msh(ss,zz),1
									PositionEntity block_msh(ss,zz),block_x(ss,zz)+6,block_y(ss,zz),block_z(ss,zz)-18.5
							
								Case 5
									block_msh(ss,zz)=CopyEntity(extraball)
									EntityTexture block_msh(ss,zz),ball_tex,0,0
									If envmapping=True Then EntityTexture block_msh(ss,zz),envtex,0,1
									PositionEntity block_msh(ss,zz),bx,by,bz
							
								Case 6
									block_msh(ss,zz)=CopyEntity(extraball)
									EntityTexture block_msh(ss,zz),ball_tex,0,0
									If envmapping=True Then EntityTexture block_msh(ss,zz),envtex,0,1
									PositionEntity block_msh(ss,zz),bx,by,bz
							
								Case 7
									block_msh(ss,zz)=CopyEntity(extraball)
									EntityTexture block_msh(ss,zz),ball_tex,0,0
									If envmapping=True Then EntityTexture block_msh(ss,zz),envtex,0,1
									PositionEntity block_msh(ss,zz),bx,by,bz
							
								Case 8
									block_msh(ss,zz)=CopyEntity(extraball)
									PositionEntity block_msh(ss,zz),bx,1,bz
							
								Case 9
									; Atomball
									block_msh(ss,zz)=CopyEntity(extraball)
									EntityTexture block_msh(ss,zz),ball_tex,0,0
									If envmapping=True Then EntityTexture block_msh(ss,zz),envtex,0,1
									EntityColor block_msh(ss,zz),255,0,0
									PositionEntity block_msh(ss,zz),bx,by,bz
									block_count=block_count-1
							
								Case 10
									block_msh(ss,zz)=CopyEntity(extraball)
									EntityTexture extraball,ball_tex,0,1
									If envmapping=True Then EntityTexture extraball,envtex,0,1
									EntityAlpha extraball,0.7
									PositionEntity block_msh(ss,zz),bx,by,bz
							
								Case 11
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5

								Case 12
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5
							
								Case 13
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5
							
								Case 14
									block_count=block_count-1
									block_msh(ss,zz)=0
							
								Case 15
									block_count=block_count-1
									block_msh(ss,zz)=0
							
								Case 16
									block_count=block_count-1
									block_msh(ss,zz)=0
							
								Case 17
									block_count=block_count-1
									block_msh(ss,zz)=0
							
								Case 18
									block_count=block_count-1
									block_msh(ss,zz)=0
							
								Case 19
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5
								Case 20
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5
								Case 21
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5
								Case 22
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5
								Case 23
									; gimme one more time
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5
								Case 24
									; no more time
									block_msh(ss,zz)=CopyEntity(altblock)
									EntityTexture block_msh(ss,zz),altblocktex
									PositionEntity block_msh(ss,zz),block_x(ss,zz),block_y(ss,zz),block_z(ss,zz)+0.5
							End Select
							CreateSparks(specialfx+2,mbx#,2,mbz#,spark_red)
							killed=0
							flag=1
						EndIf
					EndIf
					
					ss=ss+1
					If ss=map_w
						ss=0 : zz=zz+1
					EndIf
					If zz=map_h Then flag=1
				Until flag=1
			EndIf
		
			If EntityCollided(multiballs(i),COLL_PADDLE)
				coll_count=0
				CreateSparks(specialfx/2,mbx,EntityY(pdl),mbz,smoke)
				CreateSparks(2,mbx,EntityY(pdl),mbz,spark_star)
				out_sound=paddlehit01
				
				If mb_dir_x(i)=0 Then mb_dir_x(i)=mb_dir_x(i)+Rnd(-0.2,0.2)
				If mb_dir_z(i)=0 Then mb_dir_z(i)=mb_dir_x(i)+Rnd(-0.2,0.2)
				
				MoveEntity multiballs(i),mb_dir_x(i)*(speed+mb_energie(i)),-gravity,mb_dir_z(i)*(speed+mb_energie(i))
			EndIf
		
			If EntityCollided(multiballs(i),COLL_WALL)
				If MilliSecs()-mwallcol<=50
					mb_dir_x(i)=mb_dir_x(i)+2
					mb_dir_z(i)=mb_dir_z(i)+2
					mwallcoll=MilliSecs()
				EndIf
				
				If mb_dir_x(i)=0 Then mb_dir_x(i)=mb_dir_x(i)+Rnd(-0.2,0.2)
				If mb_dir_z(i)=0 Then mb_dir_z(i)=mb_dir_x(i)+Rnd(-0.2,0.2)
				
				coll_count=0
				out_sound=boing
				If specialfx>1 Then CreateSparks(2,mbx#,2,mbz#,smoke)
			EndIf
		
			If EntityCollided(multiballs(i),COLL_BUMPER)
				If bumpercoll>0
					If MilliSecs()-mbumpercol<=50
						mb_dir_x(i)=mb_dir_x(i)+2
						mb_dir_z(i)=mb_dir_z(i)+2
						mbumpercoll=MilliSecs()
					EndIf
				EndIf
			
				coll_count=0
				out_sound=bumper_snd
				If extrascore_time<=0
					CreateScore(1,mbx#,mbz#)
					scorepoints=scorepoints+1
				Else
					CreateScore(2,mbx#,mbz#)
					scorepoints=scorepoints+2
				EndIf
			
				If specialfx>1 Then CreateSparks(4,mbx#,2,mbz#,spark_blue)
				If mb_energie(i)>0 Then mb_energie(i)=mb_energie(i)+0.01 Else mb_energie(i)=0.6
			EndIf
		EndIf
	
		If out_sound
			chann=PlaySound(out_sound)
			ChannelVolume chann,sound_vol#
		EndIf
	
		out_sound=0
		EndIf
	Next
Return