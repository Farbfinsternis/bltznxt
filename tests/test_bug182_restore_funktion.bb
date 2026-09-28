; BUG-182: Restore in einer Funktion erreicht ein Label des Hauptprogramms -
; auch eines, das erst hinter der Funktion steht. Am Original gemessen:
; "x y x" (Read im Hauptprogramm, dann zweimal ueber die Funktion).

Function Ab$(wo$)
	If wo = "vorn" Then Restore vorn Else Restore hinten
	Read a$
	Return a
End Function

.vorn
Data "x"

Read b$
Print b + " " + Ab("hinten") + " " + Ab("vorn")
End

.hinten
Data "y"
