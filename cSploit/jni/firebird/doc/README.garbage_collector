-----------------------------------------------------------
Firebird 2.0 garbage collector 
-----------------------------------------------------------

  Engine knows about each record versions produced by update\delete statement
and can remove them as soon as oldest snapshot transaction (OST) allowed this. 
This eliminates the need to read pages with these versions again by user request 
(i.e. SELECT COUNT(*) FROM table) and avoids situation when these versions are
never read (of course sweep always removed all unused record versions). Also 
there is high probability that needed pages still reside in the buffer cache 
thus there are less additional disk IO needed. 

  Between notifying garbage collector about page with unused versions and time
when garbage collector will read this page new transaction can update record 
and garbage collector can't cleanup this record if this new transaction number 
is above OST or still active. In this case engine again notifies garbage collector 
with this page number and it will clean up garbage at some time later (old garbage 
collector will "forget" about this page until user reads it again)

  Both cooperative and background garbage collection are now possible. To manage it 
new configuration parameter "GCPolicy" was introduced. It can be set to:

  a) cooperative - garbage collection performed only in cooperative mode, such 
     as before IB6. Each user request is responsible for removing unused record
     versions. This is how engine works before IB6. This is only option for 
     Classic Server mode. Engine doesn't track versions produced as result of 
     update and delete statements
    
  b) background - garbage collection performed only by background thread, such 
     as in IB6 and later. No user requests remove unused record versions. 
     Instead user request notifies dedicated garbage collector thread with page 
     number where unused record version is detected. Also engine remembers page numbers
     where update\delete statement created backversions.

  c) combined - both background and cooperative garbage collection are performed.

  Default is "combined" for SuperServer. ClassicServer ignores this parameter and
always works in "cooperative" mode
