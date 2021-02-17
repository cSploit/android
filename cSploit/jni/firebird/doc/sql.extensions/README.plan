-----------
PLAN clause
-----------

  Function:
    Allows to specify a used-supplied access path for a select-based SQL statement.

  Syntax rules:
    PLAN ( { <stream_retrieval> | <sorted_streams> | <joined_streams> } )

    <stream_retrieval> ::= { <natural_scan> | <indexed_retrieval> | <navigational_scan> }

    <natural_scan> ::= <stream_alias> NATURAL
    <indexed_retrieval> ::= <stream_alias> INDEX ( <index_name> [, <index_name> ...] )
    <navigational_scan> ::= <stream_alias> ORDER <index_name> [ INDEX ( <index_name> [, <index_name> ...] ) ]

    <sorted_streams> ::= SORT ( <stream_retrieval> )

    <joined_streams> ::= JOIN ( <stream_retrieval>, <stream_retrieval> [, <stream_retrieval> ...] )
    	| [SORT] MERGE ( <sorted_streams>, <sorted_streams> )

  Description:
    Natural scan means that all rows are fetched in their natural storage order,
    which requires to read all pages and validate any search criteria afterward.

    Indexed retrieval uses an index range scan to find rowids which match the given
    search criteria. The found matches are combined in a sparse bitmap which is sorted
    by page numbers, so every data page will be read only once. After that the table
    pages are read and required rows are fetched from them.

    Navigational scan uses an index to return rows in the given order, if such an
    operation is appropriate. The index b-tree is walked from the leftmost node to the
    rightmost one. If any search criteria is used on a column being ordered by, then the
    navigation is limited to some subtree path, depending on a predicate. If any search
    criteria is used on other columns which are indexed, then a range index scan is performed
    in advance and every fetched key has its rowid validated against the resulting bitmap.
    Then a data page is read and required row is fetched. Note that navigational scan produces
    random page I/O as reads are not optimized.
    
    A sort operation performs an external sort of the given stream retrieval.

    A join can be performed either via the nested loops algorithm (JOIN plan) or via
    the sort merge algorithm (MERGE plan). An inner nested loop join may contain as many
    streams as required to be joined (as all of them are equivalent), whilst an outer
    nested loops join always operates with two (outer and inner) streams, so you'll see
    nested JOIN clauses in the case of 3 or more outer streams joined. A sort merge operates
    with two input streams which are sorted beforehand, then they're merged in a single run.

  Example(s):
    SELECT RDB$RELATION_NAME
    FROM RDB$RELATIONS
    WHERE RDB$RELATION_NAME LIKE 'RDB$%'
    PLAN (RDB$RELATIONS NATURAL)
    ORDER BY RDB$RELATION_NAME

    SELECT R.RDB$RELATION_NAME, RF.RDB$FIELD_NAME
    FROM RDB$RELATIONS R
      JOIN RDB$RELATION_FIELDS RF ON R.RDB$RELATION_NAME = RF.RDB$RELATION_NAME
    PLAN MERGE (SORT (R NATURAL), SORT (RF NATURAL))

  Note(s):
    1. A PLAN clause may be used in all select expressions, including subqueries,
       derived tables and view definitions. It can be also used in UPDATE and DELETE
       statements, because they're implicitly based on select expressions.
    2. If a PLAN clause contains some invalid retrieval description, then either an
       error will be returned or this bad clause will be silently ignored, depending
       on severity of the issue.
    3. ORDER <navigational_index> INDEX ( <filter_indices> ) kind of plan is reported
       by the engine and can be used in the user-supplied plans starting with FB 2.0.
