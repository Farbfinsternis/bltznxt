; WEAK-13 / BUG-14 — nach einem Include muessen Zeilennummern und Dateiname
; weiter auf die Quelle zeigen, aus der die Zeile wirklich stammt.
Include "inc_weak13_helper.bb"
Print Verdoppelt(21)
Print GibtEsNichtA(1)
