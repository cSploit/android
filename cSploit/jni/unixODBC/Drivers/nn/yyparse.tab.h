typedef union {
	char*	qstring;
	char*	name;
	long	number;

	int	ipar;		/* parameter index */
	int	cmpop;		/* for comparsion operators */
	char	esc;
	int	flag;		/* for not_opt and case_opt */
	int	idx;

	void*	offset; 	/* actually, it is used as a 'int' offset */

	node_t node;		/* a node haven't add to tree */
} YYSTYPE;
#define kwd_select	258
#define kwd_all 259
#define kwd_news	260
#define kwd_xnews	261
#define kwd_distinct	262
#define kwd_count	263
#define kwd_from	264
#define kwd_where	265
#define kwd_in	266
#define kwd_between	267
#define kwd_like	268
#define kwd_escape	269
#define kwd_group	270
#define kwd_by	271
#define kwd_having	272
#define kwd_order	273
#define kwd_for 274
#define kwd_insert	275
#define kwd_into	276
#define kwd_values	277
#define kwd_delete	278
#define kwd_update	279
#define kwd_create	280
#define kwd_alter	281
#define kwd_drop	282
#define kwd_table	283
#define kwd_column	284
#define kwd_view	285
#define kwd_index	286
#define kwd_of	287
#define kwd_current	288
#define kwd_grant	289
#define kwd_revoke	290
#define kwd_is	291
#define kwd_null	292
#define kwd_call	293
#define kwd_uncase	294
#define kwd_case	295
#define kwd_fn	296
#define kwd_d	297
#define QSTRING 298
#define NUM	299
#define NAME	300
#define PARAM	301
#define kwd_or	302
#define kwd_and 303
#define kwd_not 304
#define CMPOP	305

