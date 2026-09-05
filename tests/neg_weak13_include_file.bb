; WEAK-13 / BUG-14 — ein Fehler in der eingebundenen Datei muss diese Datei
; nennen, nicht die Hauptdatei.
Include "inc_weak13_broken.bb"
Print Kaputt(1)
