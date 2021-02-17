SQL Language Extension: BIT_LENGTH, CHAR_LENGTH/CHARACTER_LENGTH and OCTET_LENGTH

Function:
	Return number of bits, characters and bytes of a string.

Author:
	Adriano dos Santos Fernandes <adrianosf@uol.com.br>

Format:
	<length function> ::=
		{ BIT_LENGTH | CHAR_LENGTH | CHARACTER_LENGTH | OCTET_LENGTH } <left paren> <value expression> <right paren>

Example:
	select
		rdb$relation_name, char_length(rdb$relation_name), char_length(trim(rdb$relation_name))
		from rdb$relations;
