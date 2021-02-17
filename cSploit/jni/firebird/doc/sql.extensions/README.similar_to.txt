====================
SIMILAR TO predicate
====================

Function:

SIMILAR TO predicate verifies if a given regular expression (per the SQL standard) matches a
string. It may be used in all the places that accept boolean expressions, like in WHERE and check
constraints.

Author:
    Adriano dos Santos Fernandes <adrianosf@uol.com.br>


Syntax:

<similar predicate> ::=
	<value> [ NOT ] SIMILAR TO <similar pattern> [ ESCAPE <escape character> ]

<similar pattern> ::= <character value expression: regular expression>

<regular expression> ::=
      <regular term>
    | <regular expression> <vertical bar> <regular term>

<regular term> ::=
      <regular factor>
    | <regular term> <regular factor>

<regular factor> ::=
      <regular primary>
    | <regular primary> <asterisk>
    | <regular primary> <plus sign>
    | <regular primary> <question mark>
    | <regular primary> <repeat factor>

<repeat factor> ::=
    <left brace> <low value> [ <upper limit> ] <right brace>

<upper limit> ::= <comma> [ <high value> ]

<low value> ::= <unsigned integer>

<high value> ::= <unsigned integer>

<regular primary> ::=
      <character specifier>
    | <percent>
    | <regular character set>
    | <left paren> <regular expression> <right paren>

<character specifier> ::=
      <non-escaped character>
    | <escaped character>

<regular character set> ::=
      <underscore>
    | <left bracket> <character enumeration>... <right bracket>
    | <left bracket> <circumflex> <character enumeration>... <right bracket>
    | <left bracket> <character enumeration include>... <circumflex> <character enumeration exclude>... <right bracket>

<character enumeration include> ::= <character enumeration>

<character enumeration exclude> ::= <character enumeration>

<character enumeration> ::=
      <character specifier>
    | <character specifier> <minus sign> <character specifier>
    | <left bracket> <colon> <character class identifier> <colon> <right bracket>

<character specifier> ::=
      <non-escaped character>
    | <escaped character>

<character class identifier> ::=
      ALPHA
    | UPPER
    | LOWER
    | DIGIT
    | SPACE
    | WHITESPACE
    | ALNUM

Note:
1) <non-escaped character> is any character except <left bracket>, <right bracket>, <left paren>,
<right paren>, <vertical bar>, <circumflex>, <minus sign>, <plus sign>, <asterisk>, <underscore>,
<percent>, <question mark>, <left brace> and <escape character>.

2) <escaped character> is the <escape character> succeeded by one of <left bracket>, <right bracket>,
<left paren>, <right paren>, <vertical bar>, <circumflex>, <minus sign>, <plus sign>, <asterisk>,
<underscore>, <percent>, <question mark>, <left brace> or <escape character>.


Syntax description and examples:

Returns true for strings that matches <regular expression> or <regular term>:
<regular expression> <vertical bar> <regular term>
    'ab' SIMILAR TO 'ab|cd|efg'   -- true
    'efg' SIMILAR TO 'ab|cd|efg'  -- true
    'a' SIMILAR TO 'ab|cd|efg'    -- false

Matches zero or more occurrences of <regular primary>:
<regular primary> <asterisk>
    '' SIMILAR TO 'a*'     -- true
    'a' SIMILAR TO 'a*'    -- true
    'aaa' SIMILAR TO 'a*'  -- true

Matches one or more occurrences of <regular primary>:
<regular primary> <plus sign>
    '' SIMILAR TO 'a+'     -- false
    'a' SIMILAR TO 'a+'    -- true
    'aaa' SIMILAR TO 'a+'  -- true

Matches zero or one occurrence of <regular primary>:
<regular primary> <question mark>
    '' SIMILAR TO 'a?'     -- true
    'a' SIMILAR TO 'a?'    -- true
    'aaa' SIMILAR TO 'a?'  -- false

Matches exact <low value> occurrences of <regular primary>:
<regular primary> <left brace> <low value> <right brace>
    '' SIMILAR TO 'a{2}'     -- false
    'a' SIMILAR TO 'a{2}'    -- false
    'aa' SIMILAR TO 'a{2}'   -- true
    'aaa' SIMILAR TO 'a{2}'  -- false

Matches <low value> or more occurrences of <regular primary>:
<regular primary> <left brace> <low value> <comma> <right brace>
    '' SIMILAR TO 'a{2,}'     -- false
    'a' SIMILAR TO 'a{2,}'    -- false
    'aa' SIMILAR TO 'a{2,}'   -- true
    'aaa' SIMILAR TO 'a{2,}'  -- true

Matches <low value> to <high value> occurrences of <regular primary>:
<regular primary> <left brace> <low value> <comma> <high value> <right brace>
    '' SIMILAR TO 'a{2,4}'       -- false
    'a' SIMILAR TO 'a{2,4}'      -- false
    'aa' SIMILAR TO 'a{2,4}'     -- true
    'aaa' SIMILAR TO 'a{2,4}'    -- true
    'aaaa' SIMILAR TO 'a{2,4}'   -- true
    'aaaaa' SIMILAR TO 'a{2,4}'  -- false

Matches any (non-empty) character:
<underscore>
    '' SIMILAR TO '_'    -- false
    'a' SIMILAR TO '_'   -- true
    '1' SIMILAR TO '_'   -- true
    'a1' SIMILAR TO '_'  -- false

Matches a string of any length (including empty strings):
<percent>
    '' SIMILAR TO '%'         -- true
    'az' SIMILAR TO 'a%z'     -- true
    'a123z' SIMILAR TO 'a%z'  -- true
    'azx' SIMILAR TO 'a%z'    -- false

Groups a complete <regular expression> to use as one single <regular primary> as a sub-expression:
<left paren> <regular expression> <right paren>
    'ab' SIMILAR TO '(ab){2}'    -- false
    'aabb' SIMILAR TO '(ab){2}'  -- false
    'abab' SIMILAR TO '(ab){2}'  -- true

Matches a character identical to one of <character enumeration>:
<left bracket> <character enumeration>... <right bracket>
    'b' SIMILAR TO '[abc]'    -- true
    'd' SIMILAR TO '[abc]'    -- false
    '9' SIMILAR TO '[0-9]'    -- true
    '9' SIMILAR TO '[0-8]'    -- false

Matches a character not identical to one of <character enumeration>:
<left bracket> <circumflex> <character enumeration>... <right bracket>
    'b' SIMILAR TO '[^abc]'    -- false
    'd' SIMILAR TO '[^abc]'    -- true

Matches a character identical to one of <character enumeration include> but not identical to one
of <character enumeration exclude>:
<left bracket> <character enumeration include>... <circumflex> <character enumeration exclude>... 
    '3' SIMILAR TO '[[:DIGIT:]^3]'    -- false
    '4' SIMILAR TO '[[:DIGIT:]^3]'    -- true

Matches a character identical to one character included in <character class identifier>.
See the table below. May be used with <circumflex> to invert the logic as above:
<left bracket> <colon> <character class identifier> <colon> <right bracket>
    '4' SIMILAR TO '[[:DIGIT:]]'  -- true
    'a' SIMILAR TO '[[:DIGIT:]]'  -- false
    '4' SIMILAR TO '[^[:DIGIT:]]' -- false
    'a' SIMILAR TO '[^[:DIGIT:]]' -- true


Character class identifiers:

Identifier 	Description
ALPHA       All characters that are simple latin letters (a-z, A-Z).
            Note: includes latin letters with accents when using accent-insensitive collation.
UPPER       All characters that are simple latin uppercase letters (A-Z).
            Important: Includes lowercase latters when using case-insensitive collation.
LOWER       All characters that are simple latin lowercase letters (a-z).
            Important: Includes uppercase latters when using case-insensitive collation.
DIGIT       All characters that are numeric digits (0-9).
SPACE       All characters that are the space character (ASCII 32).
WHITESPACE  All characters that are whitespaces (vertical tab (9), newline (10),
            horizontal tab (11), carriage return (13), formfeed (12), space (32)).
ALNUM       All characters that are simple latin letters (ALPHA) or numeric digits (DIGIT).


Functional example:

create table department (
    number numeric(3) not null,
    name varchar(25) not null,
    phone varchar(14) check (phone similar to '\([0-9]{3}\) [0-9]{3}\-[0-9]{4}' escape '\')
);

insert into department values ('000', 'Corporate Headquarters', '(408) 555-1234');
insert into department values ('100', 'Sales and Marketing', '(415) 555-1234');
insert into department values ('140', 'Field Office: Canada', '(416) 677-1000');

insert into department values ('600', 'Engineering', '(408) 555-123');	-- check constraint violation

select * from department
    where phone not similar to '\([0-9]{3}\) 555\-%' escape '\';

