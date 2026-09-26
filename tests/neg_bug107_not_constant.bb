; BUG-107 - ein Data-Wert muss konstant sein; eine Variable meldet das
; Original mit "Data expression must be constant". Vorher war "x" hier ein
; unbekannter Befehl.
x = 1
Data x
