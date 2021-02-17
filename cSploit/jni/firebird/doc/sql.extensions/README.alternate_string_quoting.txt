------------------------
Alternate String Quoting
------------------------

Support for alternate format of strings literals.

Author:
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>

Syntax:

<alternate string literal> ::=
    { q | Q } <quote> <alternate start char> [ { <char> }... ] <alternate end char> <quote>

Syntax rules:
    When <alternate start char> is '(', '{', '[' or '<', <alternate end char> is respectively
    ')', '}', ']' and '>'. In other cases, <alternate end char> is equal to <alternate start char>.

Notes:
    Inside the string, i.e., <char> items, single (not escaped) quotes could be used.
    Each quote will be part of the result string.

Examples:
    select q'{abc{def}ghi}' from rdb$database;        -- result: abc{def}ghi
    select q'!That's a string!' from rdb$database;    -- result: That's a string
