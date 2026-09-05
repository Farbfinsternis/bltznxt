; BUG-30 — ohne Dim liest sich a(0) als Aufruf. Vorher gab das zwei
; irrefuehrende Meldungen ("Expected TO", "unexpected token '='"); jetzt eine,
; die sagt, was wirklich fehlt.
For a(0) = 1 To 3
Next
