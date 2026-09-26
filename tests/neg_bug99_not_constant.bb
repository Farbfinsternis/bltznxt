; BUG-99 - ein Funktionsaufruf ist nie konstant; das Original meldet
; "Expression must be constant". Vorher scheiterte erst g++ am constexpr.
Const c = Len("abc")
Print c
