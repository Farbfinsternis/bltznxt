; Milestone 38 — Graphics Mode Init
; Opens a windowed 800x600 window, verifies the query functions, then closes.
; WaitKey returns immediately in the headless test runner (non-interactive stdin).

AppTitle "BlitzNext M38 Test"

Graphics 800, 600, 32, 0              ; windowed

Print "GraphicsWidth = " + Str(GraphicsWidth())   ; 800
Print "GraphicsHeight = " + Str(GraphicsHeight())  ; 600
Print "GraphicsDepth = " + Str(GraphicsDepth())    ; 32

Local rate% = GraphicsRate()
Print "GraphicsRate OK"               ; 0 on headless; real Hz on a display

Local total% = TotalVidMem()
Print "TotalVidMem OK"               ; 536870912 (512 MB stub)

Local avail% = AvailVidMem()
Print "AvailVidMem OK"               ; 536870912

GraphicsMode 640, 480, 32, 0         ; re-enter at a different resolution
Print "GraphicsMode OK"

EndGraphics
Print "EndGraphics OK"

Print "DONE"
