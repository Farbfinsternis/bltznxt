; Leuchtturm - Klang (Schritt 6).
;
; Alles klingt dort, wo es passiert: der Listener sitzt an der Kamera, jeder
; Klang wird mit EmitSound an eine Entity gehaengt - Schuesse an die
; Muendung der Waffe, der Schub an die fliegende Rakete, Treffer an die
; Scheibe, eine Explosion an einen Pivot an ihrem Ort (Effekt_Klang). Die
; Klaenge sind mit Load3DSound geladen; der Schub der Rakete ist mit
; LoopSound auf Wiederholung gestellt und laeuft, bis sie explodiert.
;
; Abfall: rolloff 0.25 - eine Explosion am anderen Ende der Arena (20 m) ist
; noch mit einem Sechstel zu hoeren. Doppler: Blitz3D misst Geschwindigkeiten
; je UpdateWorld; bei 60 Takten je Sekunde ergibt doppler 60 den
; physikalischen Effekt - eine vorbeifliegende Rakete faellt hoerbar ab.
;
; Klaenge aus werkzeug/klaenge.py. Ohne Klang_Laden (oder ohne Audiogeraet)
; bleibt alles still, das Spiel laeuft trotzdem.
;
;   Klang_Laden kamera, ordner$
;   kanal = Klang(snd, entity)

Const KL_ROLLOFF# = 0.25
Const KL_DOPPLER# = 60

Global kl_hoerer
Global kl_mg, kl_rl, kl_rakete, kl_explosion, kl_rail
Global kl_treffer, kl_zerplatzen, kl_wieder, kl_wechsel, kl_leer

Function Klang_Laden(kamera, ordner$)
	kl_hoerer = CreateListener(kamera, KL_ROLLOFF, KL_DOPPLER)
	kl_mg = Load3DSound(ordner + "/mg.wav")
	kl_rl = Load3DSound(ordner + "/rl.wav")
	kl_rakete = Load3DSound(ordner + "/rakete.wav")
	kl_explosion = Load3DSound(ordner + "/explosion.wav")
	kl_rail = Load3DSound(ordner + "/rail.wav")
	kl_treffer = Load3DSound(ordner + "/treffer.wav")
	kl_zerplatzen = Load3DSound(ordner + "/zerplatzen.wav")
	kl_wieder = Load3DSound(ordner + "/wieder.wav")
	kl_wechsel = Load3DSound(ordner + "/wechsel.wav")
	kl_leer = Load3DSound(ordner + "/leer.wav")
	If kl_rakete Then LoopSound kl_rakete
	SoundVolume kl_treffer, 0.6
End Function

Function Klang(snd, e)
	If snd = 0 Or kl_hoerer = 0 Or e = 0 Then Return 0
	Return EmitSound(snd, e)
End Function
