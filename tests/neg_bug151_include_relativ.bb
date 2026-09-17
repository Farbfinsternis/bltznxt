; BUG-151 - ein Include relativ zur einbindenden Datei lehnt das Original ab:
; "Unable to open include file" an der Stelle hinter dem Anfuehrungszeichen.
Include "inc_bug151\c.bb"
Print "nie"
