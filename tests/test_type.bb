; Milestone 13: Type Declaration (Struct Parsing)
;
; Goal: parse Type...End Type without crash.
; No runtime output expected from the type declarations themselves.

; --- Multi-line form ---
Type Player
  Field x%, y%
  Field name$
  Field speed#
End Type

; --- Single-line (colon-separated) form from the roadmap ---
Type Vec2 : Field dx!, dy! : End Type

; --- Multiple fields on one Field line ---
Type Rect
  Field left%, top%, right%, bottom%
End Type

; Prove the program ran past the type declarations
Print "Types parsed OK"
