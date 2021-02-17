------------------
DISTINCT predicate
------------------

  Function:
    Specify a test of whether two row values are distinct.

  Author:
    Oleg Loa <loa@mail.ru>
    Dmitry Yemanov <yemanov@yandex.ru>

  Syntax rules:
    <value> IS [NOT] DISTINCT FROM <value>

  Scope:
    DSQL, PSQL

  Example(s):
    1. SELECT * FROM T1 JOIN T2 ON T1.NAME IS NOT DISTINCT FROM T2.NAME;
    2. SELECT * FROM T WHERE T.MARK IS DISTINCT FROM 'test'

  Note(s):
    1. A DISTINCT predicate evaluates very similar to an equality predicate with
       the only difference that two NULL values are considered not distinct. As a result,
       this predicate never evaluates to UNKNOWN truth value (the same as IS [NOT] NULL
       predicate behaves).
    2. The NOT DISTINCT predicate can be optimized via an index, if exists.
