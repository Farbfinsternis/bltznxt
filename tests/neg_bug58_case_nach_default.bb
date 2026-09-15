; BUG-58: Default ist der letzte Teil eines Select - danach verlangt die
; Referenz End Select
Select 2
  Default : Print "sonst"
  Case 1 : Print "eins"
End Select
