SQL Language Extension: TRIM

Function:
	Remove leading, trailing or both substring from a string.

Author:
	Adriano dos Santos Fernandes <adrianosf@uol.com.br>

Format:
	<trim function> ::=
		TRIM <left paren> [ [ <trim specification> ] [ <trim character> ] FROM ] <value expression> <right paren>

	<trim specification> ::=
		  LEADING
		| TRAILING
		| BOTH

	<trim character> ::=
		<value expression>

Syntax Rules:
	1) If <trim specification> is not specified, BOTH is assumed.
	2) If <trim character> is not specified, ' ' is assumed.
	3) If <trim specification> and/or <trim character> is specified, FROM should be specified.
	4) If <trim specification> and <trim character> is not specified, FROM should not be specified.

Examples:
A)
	select
		rdb$relation_name, trim(leading 'RDB$' from rdb$relation_name)
		from rdb$relations
		where rdb$relation_name starting with 'RDB$';

B)
	select
		trim(rdb$relation_name) || ' is a system table'
		from rdb$relations
		where rdb$system_flag = 1;
