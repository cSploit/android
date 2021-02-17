============================
Regular expression SUBSTRING
============================

Function:

Regular expression SUBSTRING returns the portion of a value who matches a SIMILAR pattern. If the
match does not occur, it returns NULL.

Author:
    Adriano dos Santos Fernandes <adrianosf at gmail.com>


Syntax:

<regular expression substring> ::=
	SUBSTRING(<value> FROM <similar pattern> ESCAPE <escape character>)

<similar pattern> ::= <character value expression: substring similar expression>

<substring similar expression> ::=
    <similar pattern: R1><escape>"<similar pattern: R2><escape>"<similar pattern: R3>


Notes:
    1) If any one of R1, R2, or R3 is not a zero-length string and does not have the format of a
       <similar pattern>, then an exception is raised.
    2) The returned value is the matching portion of R2 where
       <value> SIMILAR TO R1 || R2 || R3 ESCAPE <escape>


Examples:

substring('abcabc' similar 'a#"bcab#"c' escape '#')  -- bcab
substring('abcabc' similar 'a#"%#"c' escape '#')     -- bcab
substring('abcabc' similar '_#"%#"_' escape '#')     -- bcab
substring('abcabc' similar '#"(abc)*#"' escape '#')  -- abcabc
substring('abcabc' similar '#"abc#"' escape '#')     -- <null>
