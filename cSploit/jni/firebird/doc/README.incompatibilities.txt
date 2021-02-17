********************************************************************************
  LIST OF KNOWN INCOMPATIBILITIES
  between versions 2.x and 3.0
********************************************************************************

This document describes all the changes that make v3.0 incompatible in any way
as compared with the previous releases and hence could affect your databases and
applications.

Please read the below descriptions carefully before upgrading your software to
the new Firebird version.

INSTALLATION/CONFIGURATION
--------------------------

  * Priorly deprecated parameters CompleteBooleanEvaluation, OldColumnNaming and
    OldSetClauseSemantics of firebird.conf are not supported anymore and have been removed.
	
  * Parameters UsePriorityScheduler, PrioritySwitchDelay, PriorityBoost, LegacyHash,
    LockGrantOrder of firebird.conf have no use anymore and therefore were removed.

SQL SYNTAX
--------------------------

  * A number of new reserved keywords are introduced. The full list is available
    here: /doc/sql.extentions/README.keywords. Please ensure your DSQL
    statements and procedure/trigger sources don't contain those keywords as
    identifiers. Otherwise, you'll need to either use them quoted (in Dialect 3
    only) or rename them.

  * Improperly mixed explicit and implicit joins are not supported anymore, as per
    the SQL specification. It also means that in the explicit A JOIN B ON <condition>,
    the condition is not allowed to reference any stream except A and B.
    See ticket CORE-2812 in the Firebird tracker for more details.

SQL EXECUTION RESULTS
--------------------------

  * [to be written]

UTILITIES
--------------------------

  * [to be written]

API
--------------------------

  * [to be written]

SECURITY
--------------------------

  * [to be written]
