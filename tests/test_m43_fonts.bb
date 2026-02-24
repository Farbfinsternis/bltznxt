; Milestone 43: Fonts
;
; Without TTF/stb_truetype the runtime falls back to the built-in 8x8 bitmap
; font for all metrics.  All functions must not crash; query functions must
; return the expected 8x8 values.

Graphics 800, 600, 32, 0

; Use a deliberately non-existent name so the test is deterministic on every
; machine regardless of installed fonts — always falls back to 8×8 bitmap.
Local fnt% = LoadFont("__blitznxt_no_such_font__", 16, 0, 0, 0)

; SetFont(handle): activate the font
SetFont fnt

; Metrics — bitmap fallback always reports 8x8
Print FontHeight()
Print FontWidth()
Print StringWidth("Hello")
Print StringHeight("Hello")

; Text still works (uses built-in bitmap glyphs)
Text 10, 10, "Hi"
Print "Text OK"

; SetFont roadmap pattern: function call as argument (non-existent → fallback)
SetFont LoadFont("__blitznxt_no_such_font_2__", 12, 0, 0, 0)
Print "SetFont OK"

; FreeFont and restore default
FreeFont fnt
SetFont 0
Print FontHeight()

EndGraphics
Print "DONE"
