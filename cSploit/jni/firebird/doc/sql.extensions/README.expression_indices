------------------
Expression indices
------------------

  Function:
    Allow to index arbitrary expressions applied to the row values, hence allowing
    indexed access paths to be used for expression-based predicates.

  Author:
    Oleg Loa <loa@mail.ru>
    Dmitry Yemanov <yemanov@yandex.ru>

  Syntax rules:
    CREATE [UNIQUE] [{ASC[ENDING] | DESC[ENDING]}] INDEX <index_name> ON <table_name>
      COMPUTED [BY] ( <value_expression> )

  Scope:
    DSQL (DDL)

  Example(s):
    1. CREATE INDEX IDX1 ON T1 COMPUTED BY ( UPPER(COL1 COLLATE PXW_CYRL) );
       SELECT * FROM T1 WHERE UPPER(COL1 COLLATE PXW_CYRL) = '<capital cyrillic letters sequence>'
       -- PLAN (T1 INDEX (IDX1))
    2. CREATE INDEX IDX2 ON T2 COMPUTED BY ( EXTRACT(YEAR FROM COL2) || EXTRACT(MONTH FROM COL2) );
       SELECT * FROM T2 ORDER BY EXTRACT(YEAR FROM COL2) || EXTRACT(MONTH FROM COL2)
       -- PLAN (T2 ORDER IDX2)

  Note(s):
    1. An expression used in the index declaration must match a predicate expression precisely
       to allow the engine to choose an indexed access path. Otherwise, the given index won't be used
       for a retrieval/sorting.
    2. Expression indices have exactly the same features and limitations as usual indices, with the
       only exception that, by definition, they cannot be composite (multi-segment).
