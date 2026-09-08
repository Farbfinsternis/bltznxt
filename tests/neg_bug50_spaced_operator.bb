; BUG-50 - Die Aliase fassen nur unmittelbar benachbarte Zeichen zusammen.
; Mit Zwischenraum ist "= >" kein Operator; das Original lehnt ab
; ("Expecting expression", gemessen Blitz3D 11.8).
Print 5 = > 3
