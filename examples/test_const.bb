; Test for Constants

Const MAX_PLAYER = 10
Const INITIAL_HP# = 100.5
Const GAME_TITLE$ = "BlitzNext Test"

DebugLog "Game: " + GAME_TITLE
DebugLog "Max Players: " + MAX_PLAYER
DebugLog "Initial HP: " + INITIAL_HP

TestConst()

Function TestConst()
	DebugLog "Inside function - Max Players: " + MAX_PLAYER
End Function
