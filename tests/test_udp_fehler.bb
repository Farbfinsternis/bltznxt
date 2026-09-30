; UDP: ein ungueltiger Stream bricht ab wie im Debug-Modus des Originals
; ("UDP Stream does not exist", Laufzeitstring aus runtime.dll), auf stderr.
a = CreateUDPStream(28720)
CloseUDPStream a
Print "geschlossen"
Print RecvUDPMsg(a)
Print "nicht erreicht"
