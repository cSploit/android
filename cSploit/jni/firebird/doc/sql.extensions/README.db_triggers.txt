------------------
Database triggers
------------------

Author:
	Adriano dos Santos Fernandes <adrianosf@uol.com.br>

Syntax:
	<database-trigger> ::=
		{CREATE | RECREATE | CREATE OR ALTER}
			TRIGGER <name>
			[ACTIVE | INACTIVE]
			ON <event>
			[POSITION <n>]
		AS
		BEGIN
			...
		END

	<event> ::=
		  CONNECT
		| DISCONNECT
		| TRANSACTION START
		| TRANSACTION COMMIT
		| TRANSACTION ROLLBACK

Syntax rules:
	1) Database triggers type can't be changed.

Utilities support:
	New parameters were added to GBAK, ISQL and NBACKUP to not run database triggers.
	These parameters could only be used by database owner and SYSDBA:
	GBAK -nodbtriggers
	ISQL -nodbtriggers 
	NBACKUP -T

Permissions:
	Only database owner and SYSDBA can do database triggers DDL.

Events descriptions:

- CONNECT
	Database connection established
	A transaction is started
	Triggers are fired - uncaught exceptions rollback the transaction,
disconnect the attachment and are returned to the client
	The transaction is committed

- DISCONNECT
	A transaction is started
	Triggers are fired - uncaught exceptions rollback the transaction,
disconnect the attachment and are swallowed
	The transaction is committed
	The attachment is disconnected

- TRANSACTION START
	Triggers are fired in the newly user created transaction - uncaught
exceptions are returned to the client and the transaction is rolled-back.

- TRANSACTION COMMIT
	Triggers are fired in the committing transaction - uncaught exceptions
rollback the triggers savepoint, the commit command is aborted and the
exception is returned to the client.
	Note: for two-phase transactions the triggers are fired in "prepare"
and not in commit.

- TRANSACTION ROLLBACK
	Triggers are fired in the rolling-back transaction - changes done will
be rolled-back togheter with the transaction and exceptions are swallowed
