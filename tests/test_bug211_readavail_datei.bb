; BUG-211 - ReadAvail auf einer Datei
f$=SystemProperty("tempdir")+"bltznxt_bug211.dat"
w=WriteFile(f$)
For i=1 To 1000 : WriteByte w,i : Next
Print ReadAvail(w)
CloseFile w
r=ReadFile(f$)
Print ReadAvail(r)
x=ReadByte(r)
Print ReadAvail(r)
SeekFile r,999
Print ReadAvail(r)
x=ReadByte(r)
Print ReadAvail(r)+" "+Eof(r)
CloseFile r
o=OpenFile(f$)
Print ReadAvail(o)
CloseFile o
DeleteFile f$
