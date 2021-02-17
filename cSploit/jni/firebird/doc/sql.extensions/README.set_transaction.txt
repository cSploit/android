More options exposed in the SET TRANSACTION command.

Syntax
========

SET TRANSACTION option_list

The command is used in DSQL to start a transaction without using the specialized
API call to create a new transaction. To the already existing options, the following
have been added. Notice this is not new functionality: it's available through the
TPB since years ago (they appear in ibase.h as items for TPBs). This extension
only makes those options available to clients that want to start a transaction by
executing a DSQL command, like isql's command prompt.

The new options exposed are:

NO AUTO UNDO: prevents the transaction from keeping an undo log that would be used
to undo its changes if it's rolled back. The net effect is undoing the changes, then
marking the transaction as committed. Without the undo logs, other transactions
reading the affected record will collect the garbage. This option is useful for
massive insertions when there's no need to roll back.

IGNORE LIMBO: ignores the records created by transactions in limbo. Typically a
transaction is in limbo when it's a multi-database transaction and the two phase
commit fails. This option is mostly used by gfix.

LOCK TIMEOUT nonneg_short_integer: it's the time (measured in seconds) that a
transaction waits for a lock in a record before giving up and reporting an error.


Author:
   Claudio Valderrama C.

N O T E S
=========

1. Lock timeout has to be zero or positive and will be rejected if the NO WAIT
option is specified.
2.- For transactions that only read data, no auto undo has zero impact.

Example
=======

Using an isql session:

set transaction no auto undo lock timeout 10;

