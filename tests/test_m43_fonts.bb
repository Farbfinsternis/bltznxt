; Milestone 43: Fonts
;
; An unknown font name must not crash. Blitz3D gets a replacement font from
; GDI with the metrics of Arial; this test used to expect the built-in 8x8
; font, which was never measured (BUG-227). Measured on the original
; 2026-10-29: FontHeight 16, FontWidth 35, StringWidth("Hello") 29,
; StringHeight 16. StringWidth differs by a few pixels because BlitzNext does
; not draw through GDI.

Graphics 800, 600, 32, 0

Local fnt% = LoadFont("__blitznxt_no_such_font__", 16, 0, 0, 0)

; SetFont(handle): activate the font
SetFont fnt

Print FontHeight()
Print FontWidth()
Print Abs(StringWidth("Hello") - 29) <= 4
Print StringHeight("Hello")

; Text still works
Text 10, 10, "Hi"
Print "Text OK"

; SetFont roadmap pattern: function call as argument (non-existent → replacement font)
SetFont LoadFont("__blitznxt_no_such_font_2__", 12, 0, 0, 0)
Print "SetFont OK"

; FreeFont and restore the default font (GDI Courier 12, 13 pixels high - BUG-139)
FreeFont fnt
SetFont 0
Print FontHeight()

EndGraphics
Print "DONE"
