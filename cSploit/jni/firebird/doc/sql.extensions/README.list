SQL Language Extension: LIST

Function:
    This function returns a string result with the concatenated non-NULL
    values from a group. It returns NULL if there are no non-NULL values.

Authors:
    Oleg Loa <loa@mail.ru>
    Dmitry Yemanov <dimitr@firebird.org>

Format:

  <list function> ::=
      LIST '(' [ {ALL | DISTINCT} ] <value expression> [',' <delimiter value> ] ')'

  <delimiter value> ::=
      { <string literal> | <parameter> | <variable> }

Syntax Rules:
    1) If neither ALL nor DISTINCT is specified, ALL is implied.
    2) If <delimiter value> is omitted, a comma is used to separate
       the concatenated values.

Notes:
    1) Numeric and datetime values are implicitly converted to strings
       during evaluation.
    2) The result value is of type BLOB with SUB_TYPE TEXT for all cases
       except list of BLOB with different subtype.
    3) Ordering of values within a group is implementation-defined.

Examples:

A)
  SELECT LIST(ID, ':')
  FROM MY_TABLE

B)
  SELECT TAG_TYPE, LIST(TAG_VALUE)
  FROM TAGS
  GROUP BY TAG_TYPE
