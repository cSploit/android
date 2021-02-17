==============================================
Hexadecimal Numeric and Binary String Literals
==============================================

Support for hexadecimal numeric and binary string literals.

Authors:
    Bill Oliver <Bill.Oliver@sas.com>
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>

Syntax:

<numeric hex literal> ::=
    { 0x | 0X } <hexit> [ <hexit>... ]

<binary string literal> ::=
    { x | X } <quote> [ { <hexit> <hexit> }... ] <quote>

<digit> ::=
    0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

<hexit> ::=
    <digit> | A | B | C | D | E | F | a | b | c | d | e | f

Notes (numeric hex literal):
- The max number of <hexit> should be 16.
- If the number of <hexit> is greater than 8, the constant data type is a signed BIGINT. If it's
  less or equal than 8, the data type is a signed INTEGER. That means 0xF0000000 is -268435456 and
  0x0F0000000 is 4026531840.

Notes (binary string literal):
- The resulting string is defined as a CHAR(N / 2) CHARACTER SET OCTETS, where N is the number of <hexit>.

Example:
    select 0x10, cast('0x0F0000000' as bigint) from rdb$database;
    select x'deadbeef' from rdb$database;
