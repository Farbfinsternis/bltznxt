; BUG-58: exp() prueft zuerst das vorgefundene Token - ein Next in einem While
; ist "'Next' without 'For'", nicht "Expecting 'Wend'"
While 0
  Next
Wend
