%token	AUTO REGISTER STATIC EXTERN TYPEDEF VOID CHAR SHORT INT LONG FLOAT
	DOUBLE SIGNED UNSIGNED CONST VOLATILE STRUCT UNION ENUM CASE DEFAULT
	IF SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN ELSE
	MULEQ DIVEQ MODEQ ADDEQ SUBEQ LSHEQ RSHEQ ANDEQ XOREQ OREQ
	AND OR EQU NEQ LEQ GEQ LSH RSH INC DEC ARROW IDENTIFIER STRING
	INTCONST CHARCONST FLOATCONST ELIPSIS SIZEOF

%%

translation.unit
	: external.declaration
	| translation.unit external.declaration
	;

external.declaration
	: function.definition
	| declaration
	;

function.definition
	: declaration.specifiers declarator declaration.list compound.statement
	| declarator declaration.list compound.statement
	| declaration.specifiers declarator compound.statement
	| declarator compound.statement
	;

declaration
	: declaration.specifiers init.declarator.list ';'
	| declaration.specifiers ';'
	;

declaration.list
	: declaration
	| declaration.list declaration
	;

declaration.specifiers
	: storage.class.specifier declaration.specifiers
	| storage.class.specifier
	| type.specifier declaration.specifiers
	| type.specifier
	| type.qualifier declaration.specifiers
	| type.qualifier
	;

storage.class.specifier
	: AUTO
	| REGISTER
	| STATIC
	| EXTERN
	| TYPEDEF
	;

type.specifier
	: VOID
	| CHAR
	| SHORT
	| INT
	| LONG
	| FLOAT
	| DOUBLE
	| SIGNED
	| UNSIGNED
	| struct.or.union.specifier
	| enum.specifier
	| typedef.name
	;

type.qualifier
	: CONST
	| VOLATILE
	;

struct.or.union.specifier
	: struct.or.union IDENTIFIER '{' struct.declaration.list '}'
	| struct.or.union  '{' struct.declaration.list '}'
	| struct.or.union IDENTIFIER
	;

struct.or.union
	: STRUCT
	| UNION
	;

struct.declaration.list
	: struct.declaration
	| struct.declaration.list struct.declaration
	;

init.declarator.list
	: init.declarator
	| init.declarator.list ',' init.declarator
	;

init.declarator
	: declarator
	| declarator '=' initializer
	;

struct.declaration
	: specifier.qualifier.list struct.declarator.list ';'
	;

specifier.qualifier.list
	: type.specifier specifier.qualifier.list
	| type.specifier
	| type.qualifier specifier.qualifier.list
	| type.qualifier
	;

struct.declarator.list
	: struct.declarator
	| struct.declarator.list ',' struct.declarator
	;

struct.declarator
	: declarator
	| declarator ':' constant.expression
	| ':' constant.expression
	;

enum.specifier
	: ENUM IDENTIFIER '{' enumerator.list '}'
	| ENUM '{' enumerator.list '}'
	| ENUM IDENTIFIER
	;

enumerator.list
	: enumerator
	| enumerator.list ',' enumerator
	;

enumerator
	: IDENTIFIER
	| IDENTIFIER '=' constant.expression
	;

declarator
	: pointer direct.declarator
	| direct.declarator
	;

direct.declarator
	: IDENTIFIER
	| '(' declarator ')'
	| direct.declarator '[' constant.expression ']'
	| direct.declarator '[' ']'
	| direct.declarator '(' parameter.type.list ')'
	| direct.declarator '(' identifier.list ')'
	| direct.declarator '(' ')'
	;

pointer : '*' type.qualifier.list
	| '*'
	| '*' type.qualifier.list pointer
	| '*' pointer
	;

type.qualifier.list
	: type.qualifier
	| type.qualifier.list type.qualifier
	;

parameter.type.list
	: parameter.list
	| parameter.list ',' ELIPSIS
	;

parameter.list
	: parameter.declaration
	| parameter.list ',' parameter.declaration
	;

parameter.declaration
	: declaration.specifiers declarator
	| declaration.specifiers abstract.declarator
	| declaration.specifiers
	;

identifier.list
	: IDENTIFIER
	| identifier.list ',' IDENTIFIER
	;

initializer
	: assignment.expression
	| '{' initializer.list  '}'
	| '{' initializer.list ',' '}'
	;


initializer.list
	: initializer
	| initializer.list ',' initializer
	;

type.name
	: specifier.qualifier.list abstract.declarator
	| specifier.qualifier.list
	;

abstract.declarator
	: pointer
	| pointer direct.abstract.declarator
	| direct.abstract.declarator
	;

direct.abstract.declarator
	: '(' abstract.declarator ')'
	| direct.abstract.declarator '[' constant.expression ']'
	| '[' constant.expression ']'
	| direct.abstract.declarator '[' ']'
	| '[' ']'
	| direct.abstract.declarator '(' parameter.type.list ')'
	| '(' parameter.type.list ')'
	| direct.abstract.declarator '(' ')'
	| '(' ')'
	;

typedef.name
	: IDENTIFIER
	;

statement
	: labeled.statement
	| expression.statement
	| compound.statement
	| selection.statement
	| iteration.statement
	| jump.statement
	;

labeled.statement
	: IDENTIFIER ':' statement
	| CASE constant.expression ':' statement
	| DEFAULT ':' statement
	;

expression.statement
	: expression ';'
	| ';'
	;

compound.statement
	: '{' declaration.list statement.list '}'
	| '{' declaration.list '}'
	| '{' statement.list '}'
	| '{' '}'
	;

statement.list
	: statement
	| statement.list statement
	;

selection.statement
	: IF '(' expression ')' statement
	| IF '(' expression ')' statement ELSE statement
	| SWITCH '(' expression ')' statement
	;

iteration.statement
	: WHILE '(' expression ')' statement
	| DO statement WHILE '(' expression ')' ';'
	| FOR '(' expression ';' expression ';' expression ')' statement
	| FOR '(' expression ';' expression ';' ')' statement
	| FOR '(' expression ';' ';' expression ')' statement
	| FOR '(' expression ';' ';' ')' statement
	| FOR '(' ';' expression ';' expression ')' statement
	| FOR '(' ';' expression ';' ')' statement
	| FOR '(' ';' ';' expression ')' statement
	| FOR '(' ';' ';' ')' statement
	;

jump.statement
	: GOTO IDENTIFIER ';'
	| CONTINUE ';'
	| BREAK ';'
	| RETURN expression ';'
	| RETURN  ';'
	;

constant.expression
	: assignment.expression
	;

expression
	: assignment.expression
	| expression ',' assignment.expression
	;

assignment.expression
	: conditional.expression
	| unary.expression assignment.operator assignment.expression
	;

assignment.operator
	:  '='  | MULEQ | DIVEQ | MODEQ | ADDEQ | SUBEQ
	| LSHEQ | RSHEQ | ANDEQ | XOREQ | OREQ
	;

conditional.expression
	: logical.OR.expression
	| logical.OR.expression '?' expression ':' conditional.expression
	;

logical.OR.expression
	: logical.AND.expression
	| logical.OR.expression OR logical.AND.expression
	;

logical.AND.expression
	: inclusive.OR.expression
	| logical.AND.expression AND inclusive.OR.expression
	;

inclusive.OR.expression
	: exclusive.OR.expression
	| inclusive.OR.expression '|' exclusive.OR.expression
	;

exclusive.OR.expression
	: AND.expression
	| exclusive.OR.expression '^' AND.expression
	;

AND.expression
	: equality.expression
	| AND.expression '&' equality.expression
	;

equality.expression
	: relational.expression
	| equality.expression EQU relational.expression
	| equality.expression NEQ relational.expression
	;

relational.expression
	: shift.expression
	| relational.expression '<' shift.expression
	| relational.expression '>' shift.expression
	| relational.expression LEQ shift.expression
	| relational.expression GEQ shift.expression
	;

shift.expression
	: additive.expression
	| shift.expression LSH additive.expression
	| shift.expression RSH additive.expression
	;

additive.expression
	: multiplicative.expression
	| additive.expression '+' multiplicative.expression
	| additive.expression '-' multiplicative.expression
	;

multiplicative.expression
	: cast.expression
	| multiplicative.expression '*' cast.expression
	| multiplicative.expression '/' cast.expression
	| multiplicative.expression '%' cast.expression
	;

cast.expression
	: unary.expression
	| '(' type.name ')' cast.expression
	;

unary.expression
	: postfix.expression
	| INC unary.expression
	| DEC unary.expression
	| unary.operator cast.expression
	| SIZEOF unary.expression
	| SIZEOF '(' type.name ')'
	;

unary.operator
	: '&' | '*' | '+' | '-' | '~' | '!'
	;

postfix.expression
	: primary.expression
	| postfix.expression '[' expression ']'
	| postfix.expression '(' argument.expression.list ')'
	| postfix.expression '(' ')'
	| postfix.expression '.' IDENTIFIER
	| postfix.expression ARROW IDENTIFIER
	| postfix.expression INC
	| postfix.expression DEC
	;

primary.expression
	: IDENTIFIER
	| constant
	| STRING
	| '(' expression ')'
	;

argument.expression.list
	: assignment.expression
	| argument.expression.list ',' assignment.expression
	;

constant
	: INTCONST
	| CHARCONST
	| FLOATCONST
	;

