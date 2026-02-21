; Test script for SystemProperty and IO
Print "Testing SystemProperty..."

windir$ = SystemProperty("windowsdir")
sysdir$ = SystemProperty("systemdir")
tmpdir$ = SystemProperty("tempdir")
appdir$ = SystemProperty("appdir")

Print "Windows: " + windir$
Print "System:  " + sysdir$
Print "Temp:    " + tmpdir$
Print "App:     " + appdir$
Print "AppData: " + SystemProperty("appdata")
Print "Local:   " + SystemProperty("localappdata")
Print "Docs:    " + SystemProperty("documents")
Print "Desktop: " + SystemProperty("desktop")

If windir$ = "" Then RuntimeError "windowsdir failed"
If sysdir$ = "" Then RuntimeError "systemdir failed"
If tmpdir$ = "" Then RuntimeError "tempdir failed"

; Use TempDir for file test to avoid permission issues
Print ""
Print "Testing File I/O in Temp..."
bnbcfg$ = "bltznext_test.cfg"
path$ = tmpdir$ + bnbcfg$

If FileType(path$) <> 1
    Print "Creating file: " + path$
    outfile = WriteFile(path$)
    If outfile = 0 Then RuntimeError "WriteFile failed"
    WriteLine outfile, "Current Dir: " + CurrentDir$()
    WriteLine outfile, "Hello from BltzNxt!"
    CloseFile outfile
    Print "File written."
Else
    Print "File already exists. Deleting for re-test..."
    DeleteFile path$
    Print "Deleted. Run again to test creation."
EndIf

Print "Done."
Delay 2000
End
