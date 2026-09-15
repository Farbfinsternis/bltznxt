; BUG-58: vor dem ersten Case steht nur Case, Default oder End Select
Select 1
  Print "vor case"
  Case 1 : Print "eins"
End Select
