--------------------
IIF builtin function
--------------------

Function:
    Return a value of the first sub-expression if the given search condition
    evaluates to TRUE, otherwise return a value of the second sub-expression.

Author:
    Oleg Loa <loa@mail.ru>

Format:
    IIF ( <search condition>, <value expression>, <value expression> )

Syntax Rule(s):
    IIF (SC, V1, V2) is equivalent to the following <case specification>:
         CASE WHEN SC THEN V1 ELSE V2 END

Example(s):
    SELECT IIF(VAL > 0, VAL, -VAL) FROM OPERATION
