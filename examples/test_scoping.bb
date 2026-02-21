; Test for Global vs Local scoping

Global gVar = 100
Local lVar = 50

DebugLog "Main Flow:"
DebugLog "  Global gVar: " + gVar ; Should be 100
DebugLog "  Local lVar: " + lVar   ; Should be 50

TestScoping()

Function TestScoping()
	DebugLog "Inside Function:"
	DebugLog "  Global gVar: " + gVar ; Should be 100
	
	Local lVar = 200
	DebugLog "  Inside Local lVar: " + lVar ; Should be 200
End Function

DebugLog "Back in Main:"
DebugLog "  Global gVar after function: " + gVar
DebugLog "  Local lVar after function: " + lVar ; Should still be 50
