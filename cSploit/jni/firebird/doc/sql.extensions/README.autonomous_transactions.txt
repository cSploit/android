=======================
Autonomous Transactions
=======================

Function:

Runs a piece of code in an autonomous (independent) transaction. It's useful in cases where you
need to raise an exception but do not want the database changes to be rolled-back.

If exceptions are raised inside the body of an autonomous transaction block, the changes are
rolled-back. If the block runs till the end, the transaction is committed.

The new transaction is initiated with the same isolation level of the existing one.
Should be used with caution to avoid deadlocks.

Author:
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>

Syntax:

in autonomous transaction
do
    <simple statement | compound statement>

Example:

create table log (
    "date" timestamp,
    msg varchar(60)
);

create exception e_conn 'Connection rejected';

set term !;

create trigger t_conn on connect
as
begin
    if (current_user = 'BAD_USER') then
    begin
        in autonomous transaction
        do
        begin
            insert into log ("date", msg) values (current_timestamp, 'Connection rejected');
        end

        exception e_conn;
    end
end!

set term ;!

