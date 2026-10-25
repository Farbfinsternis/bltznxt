; BUG-122 - SystemProperty liefert die bekannten Eigenschaften.
;
; Wie gxRuntime::systemProperty; gemessen an Blitz3D 11.8 unter Windows 11
; (build/bug122_20261025/sp). Bis dahin war jeder Wert leer, und Programme,
; die mit "appdir" oder "tempdir" Pfade bauen, fanden ihre Dateien nicht.
; Geprueft wird nur, was auf jedem Rechner gilt.
Print SystemProperty("cpu")
Print Right(SystemProperty("AppDir"), 1) = "\"
Print FileType(SystemProperty("appdir")) = 2
Print Right(SystemProperty("windowsdir"), 1) = "\"
Print FileType(SystemProperty("systemdir")) = 2
Print Right(SystemProperty("tempdir"), 1) = "\"
Print FileType(SystemProperty("tempdir")) = 2
Print SystemProperty("apphwnd") <> ""
Print SystemProperty("apphinstance") <> "0"
Print "<" + SystemProperty("gibtsnicht") + ">"
Print "<" + SystemProperty("direct3d7") + ">"
Graphics 100,100,0,2
Print SystemProperty("apphwnd") <> "0"
