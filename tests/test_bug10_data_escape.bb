; BUG-10 — Data-Strings muessen fuer den C++-Code escaped werden
Data "C:\temp"
Data "a\b\c"
Data "back\slash"
Data 42, 3.5
Data "plain"

Local s$
Read s$ : Print s
Print Len(s)
Read s$ : Print s
Read s$ : Print s
Local i% : Read i% : Print i
Local f# : Read f# : Print f
Read s$ : Print s
