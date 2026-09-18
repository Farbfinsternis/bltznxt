; BUG-95: der Zaehler einer For-Schleife muss int oder float sein.
; Original: "index variable must be integer or real" (ForNode::semant).
; Vorher nahmen wir das an und scheiterten erst in g++ am ++ auf bbString.
For s$ = 1 To 3
Next
