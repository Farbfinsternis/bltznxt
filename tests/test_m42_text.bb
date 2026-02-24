; Milestone 42: Text & Console Output
;
; Tests: Write (no newline), Locate (console cursor), Text (bitmap font)

Graphics 800, 600, 32, 0

; Write: two consecutive writes land on the same line (no newline between)
Write "Hello"
Write " World"
Print ""

; Text: draws with built-in 8x8 bitmap font — headless is a safe no-op
Text 100, 100, "Hi"
Print "Text OK"

; Text with centering flags
Text 400, 300, "Centered", 1, 1
Print "Center OK"

; Locate: repositions console cursor — no-op when not a TTY (headless)
Locate 0, 0
Print "Locate OK"

EndGraphics

Print "DONE"
