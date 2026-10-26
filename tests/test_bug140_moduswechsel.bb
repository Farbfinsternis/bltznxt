; BUG-140 - ein Moduswechsel setzt LoaderMatrix zurueck.
;
; blitz3d_close leert im Original die Tabelle der Loader-Matrizen bei jedem
; Graphics, Graphics3D und EndGraphics (gemessen an Blitz3D 11.8,
; build/bug140_20261026/lm: genau diese Ausgabe). Vorher galt eine eigene
; Matrix ueber den Moduswechsel hinweg weiter. Dabei werden wie im Original
; auch alle Images freigegeben.
Graphics3D 200,150,0,2
LoaderMatrix "x",2,0,0,0,1,0,0,0,1
m=LoadMesh("tests\assets\test_tri_cw.x")
Print MeshWidth(m)
Graphics3D 200,150,0,2
m=LoadMesh("tests\assets\test_tri_cw.x")
Print MeshWidth(m)
LoaderMatrix "x",3,0,0,0,1,0,0,0,1
EndGraphics
Graphics3D 200,150,0,2
m=LoadMesh("tests\assets\test_tri_cw.x")
Print MeshWidth(m)
