/* contrib/tsearch2/tsearch2--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION tsearch2" to load this file. \quit

-- These domains are just to catch schema-qualified references to the
-- old data types.
CREATE DOMAIN tsvector AS pg_catalog.tsvector;
CREATE DOMAIN tsquery AS pg_catalog.tsquery;
CREATE DOMAIN gtsvector AS pg_catalog.gtsvector;
CREATE DOMAIN gtsq AS pg_catalog.text;

--dict interface
CREATE FUNCTION lexize(oid, text)
	RETURNS _text
	as 'ts_lexize'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION lexize(text, text)
        RETURNS _text
        as 'MODULE_PATHNAME', 'tsa_lexize_byname'
        LANGUAGE C
        RETURNS NULL ON NULL INPUT;

CREATE FUNCTION lexize(text)
        RETURNS _text
        as 'MODULE_PATHNAME', 'tsa_lexize_bycurrent'
        LANGUAGE C
        RETURNS NULL ON NULL INPUT;

CREATE FUNCTION set_curdict(int)
	RETURNS void
	as 'MODULE_PATHNAME', 'tsa_set_curdict'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION set_curdict(text)
	RETURNS void
	as 'MODULE_PATHNAME', 'tsa_set_curdict_byname'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

--built-in dictionaries
CREATE FUNCTION dex_init(internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_dex_init'
	LANGUAGE C;

CREATE FUNCTION dex_lexize(internal,internal,int4)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_dex_lexize'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION snb_en_init(internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_snb_en_init'
	LANGUAGE C;

CREATE FUNCTION snb_lexize(internal,internal,int4)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_snb_lexize'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION snb_ru_init_koi8(internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_snb_ru_init_koi8'
	LANGUAGE C;

CREATE FUNCTION snb_ru_init_utf8(internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_snb_ru_init_utf8'
	LANGUAGE C;

CREATE FUNCTION snb_ru_init(internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_snb_ru_init'
	LANGUAGE C;

CREATE FUNCTION spell_init(internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_spell_init'
	LANGUAGE C;

CREATE FUNCTION spell_lexize(internal,internal,int4)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_spell_lexize'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION syn_init(internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_syn_init'
	LANGUAGE C;

CREATE FUNCTION syn_lexize(internal,internal,int4)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_syn_lexize'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION thesaurus_init(internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_thesaurus_init'
	LANGUAGE C;

CREATE FUNCTION thesaurus_lexize(internal,internal,int4,internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_thesaurus_lexize'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

--sql-level interface
CREATE TYPE tokentype
	as (tokid int4, alias text, descr text);

CREATE FUNCTION token_type(int4)
	RETURNS setof tokentype
	as 'ts_token_type_byid'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT
        ROWS 16;

CREATE FUNCTION token_type(text)
	RETURNS setof tokentype
	as 'ts_token_type_byname'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT
        ROWS 16;

CREATE FUNCTION token_type()
	RETURNS setof tokentype
	as 'MODULE_PATHNAME', 'tsa_token_type_current'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT
        ROWS 16;

CREATE FUNCTION set_curprs(int)
	RETURNS void
	as 'MODULE_PATHNAME', 'tsa_set_curprs'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION set_curprs(text)
	RETURNS void
	as 'MODULE_PATHNAME', 'tsa_set_curprs_byname'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

CREATE TYPE tokenout
	as (tokid int4, token text);

CREATE FUNCTION parse(oid,text)
	RETURNS setof tokenout
	as 'ts_parse_byid'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION parse(text,text)
	RETURNS setof tokenout
	as 'ts_parse_byname'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION parse(text)
	RETURNS setof tokenout
	as 'MODULE_PATHNAME', 'tsa_parse_current'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

--default parser
CREATE FUNCTION prsd_start(internal,int4)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_prsd_start'
	LANGUAGE C;

CREATE FUNCTION prsd_getlexeme(internal,internal,internal)
	RETURNS int4
	as 'MODULE_PATHNAME', 'tsa_prsd_getlexeme'
	LANGUAGE C;

CREATE FUNCTION prsd_end(internal)
	RETURNS void
	as 'MODULE_PATHNAME', 'tsa_prsd_end'
	LANGUAGE C;

CREATE FUNCTION prsd_lextype(internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_prsd_lextype'
	LANGUAGE C;

CREATE FUNCTION prsd_headline(internal,internal,internal)
	RETURNS internal
	as 'MODULE_PATHNAME', 'tsa_prsd_headline'
	LANGUAGE C;

--tsearch config
CREATE FUNCTION set_curcfg(int)
	RETURNS void
	as 'MODULE_PATHNAME', 'tsa_set_curcfg'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION set_curcfg(text)
	RETURNS void
	as 'MODULE_PATHNAME', 'tsa_set_curcfg_byname'
	LANGUAGE C
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION show_curcfg()
	RETURNS oid
	AS 'get_current_ts_config'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT STABLE;

CREATE FUNCTION length(tsvector)
	RETURNS int4
	AS 'tsvector_length'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION to_tsvector(oid, text)
	RETURNS tsvector
	AS 'to_tsvector_byid'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION to_tsvector(text, text)
	RETURNS tsvector
	AS 'MODULE_PATHNAME', 'tsa_to_tsvector_name'
	LANGUAGE C RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION to_tsvector(text)
	RETURNS tsvector
	AS 'to_tsvector'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION strip(tsvector)
	RETURNS tsvector
	AS 'tsvector_strip'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION setweight(tsvector,"char")
	RETURNS tsvector
	AS 'tsvector_setweight'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION concat(tsvector,tsvector)
	RETURNS tsvector
	AS 'tsvector_concat'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION querytree(tsquery)
	RETURNS text
	AS 'tsquerytree'
	LANGUAGE INTERNAL RETURNS NULL ON NULL INPUT;

CREATE FUNCTION to_tsquery(oid, text)
	RETURNS tsquery
	AS 'to_tsquery_byid'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION to_tsquery(text, text)
	RETURNS tsquery
	AS 'MODULE_PATHNAME','tsa_to_tsquery_name'
	LANGUAGE C RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION to_tsquery(text)
	RETURNS tsquery
	AS 'to_tsquery'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION plainto_tsquery(oid, text)
	RETURNS tsquery
	AS 'plainto_tsquery_byid'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION plainto_tsquery(text, text)
	RETURNS tsquery
	AS 'MODULE_PATHNAME','tsa_plainto_tsquery_name'
	LANGUAGE C RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION plainto_tsquery(text)
	RETURNS tsquery
	AS 'plainto_tsquery'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT IMMUTABLE;

--Trigger
CREATE FUNCTION tsearch2()
	RETURNS trigger
	AS 'MODULE_PATHNAME', 'tsa_tsearch2'
	LANGUAGE C;

--Relevation
CREATE FUNCTION rank(float4[], tsvector, tsquery)
	RETURNS float4
	AS 'ts_rank_wtt'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION rank(float4[], tsvector, tsquery, int4)
	RETURNS float4
	AS 'ts_rank_wttf'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION rank(tsvector, tsquery)
	RETURNS float4
	AS 'ts_rank_tt'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION rank(tsvector, tsquery, int4)
	RETURNS float4
	AS 'ts_rank_ttf'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION rank_cd(float4[], tsvector, tsquery)
        RETURNS float4
	AS 'ts_rankcd_wtt'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION rank_cd(float4[], tsvector, tsquery, int4)
	RETURNS float4
	AS 'ts_rankcd_wttf'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION rank_cd(tsvector, tsquery)
	RETURNS float4
	AS 'ts_rankcd_tt'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION rank_cd(tsvector, tsquery, int4)
	RETURNS float4
	AS 'ts_rankcd_ttf'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION headline(oid, text, tsquery, text)
	RETURNS text
	AS 'ts_headline_byid_opt'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION headline(oid, text, tsquery)
	RETURNS text
	AS 'ts_headline_byid'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION headline(text, text, tsquery, text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'tsa_headline_byname'
	LANGUAGE C RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION headline(text, text, tsquery)
	RETURNS text
	AS 'MODULE_PATHNAME', 'tsa_headline_byname'
	LANGUAGE C RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION headline(text, tsquery, text)
	RETURNS text
	AS 'ts_headline_opt'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION headline(text, tsquery)
	RETURNS text
	AS 'ts_headline'
	LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

-- CREATE the OPERATOR class
CREATE OPERATOR CLASS gist_tsvector_ops
FOR TYPE tsvector USING gist
AS
        OPERATOR        1       @@ (tsvector, tsquery),
        FUNCTION        1       gtsvector_consistent (internal, gtsvector, int, oid, internal),
        FUNCTION        2       gtsvector_union (internal, internal),
        FUNCTION        3       gtsvector_compress (internal),
        FUNCTION        4       gtsvector_decompress (internal),
        FUNCTION        5       gtsvector_penalty (internal, internal, internal),
        FUNCTION        6       gtsvector_picksplit (internal, internal),
        FUNCTION        7       gtsvector_same (gtsvector, gtsvector, internal),
        STORAGE         gtsvector;

--stat info
CREATE TYPE statinfo
	as (word text, ndoc int4, nentry int4);

CREATE FUNCTION stat(text)
	RETURNS setof statinfo
	as 'ts_stat1'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT;

CREATE FUNCTION stat(text,text)
	RETURNS setof statinfo
	as 'ts_stat2'
	LANGUAGE INTERNAL
	RETURNS NULL ON NULL INPUT;

--reset - just for debuging
CREATE FUNCTION reset_tsearch()
        RETURNS void
        as 'MODULE_PATHNAME', 'tsa_reset_tsearch'
        LANGUAGE C
        RETURNS NULL ON NULL INPUT;

--get cover (debug for rank_cd)
CREATE FUNCTION get_covers(tsvector,tsquery)
        RETURNS text
        as 'MODULE_PATHNAME', 'tsa_get_covers'
        LANGUAGE C
        RETURNS NULL ON NULL INPUT;

--debug function
create type tsdebug as (
        ts_name text,
        tok_type text,
        description text,
        token   text,
        dict_name text[],
        "tsvector" tsvector
);

CREATE FUNCTION _get_parser_from_curcfg()
RETURNS text as
$$select prsname::text from pg_catalog.pg_ts_parser p join pg_ts_config c on cfgparser = p.oid where c.oid = show_curcfg();$$
LANGUAGE SQL RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION ts_debug(text)
RETURNS setof tsdebug as $$
select
        (select c.cfgname::text from pg_catalog.pg_ts_config as c
         where c.oid = show_curcfg()),
        t.alias as tok_type,
        t.descr as description,
        p.token,
        ARRAY ( SELECT m.mapdict::pg_catalog.regdictionary::pg_catalog.text
                FROM pg_catalog.pg_ts_config_map AS m
                WHERE m.mapcfg = show_curcfg() AND m.maptokentype = p.tokid
                ORDER BY m.mapseqno )
        AS dict_name,
        strip(to_tsvector(p.token)) as tsvector
from
        parse( _get_parser_from_curcfg(), $1 ) as p,
        token_type() as t
where
        t.tokid = p.tokid
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT;

CREATE FUNCTION numnode(tsquery)
        RETURNS int4
        as 'tsquery_numnode'
        LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION tsquery_and(tsquery,tsquery)
        RETURNS tsquery
        as 'tsquery_and'
        LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION tsquery_or(tsquery,tsquery)
        RETURNS tsquery
        as 'tsquery_or'
        LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION tsquery_not(tsquery)
        RETURNS tsquery
        as 'tsquery_not'
        LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

--------------rewrite subsystem

CREATE FUNCTION rewrite(tsquery, text)
        RETURNS tsquery
        as 'tsquery_rewrite_query'
        LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION rewrite(tsquery, tsquery, tsquery)
        RETURNS tsquery
        as 'tsquery_rewrite'
        LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION rewrite_accum(tsquery,tsquery[])
        RETURNS tsquery
        AS 'MODULE_PATHNAME', 'tsa_rewrite_accum'
        LANGUAGE C;

CREATE FUNCTION rewrite_finish(tsquery)
      RETURNS tsquery
      as 'MODULE_PATHNAME', 'tsa_rewrite_finish'
      LANGUAGE C;

CREATE AGGREGATE rewrite (
      BASETYPE = tsquery[],
      SFUNC = rewrite_accum,
      STYPE = tsquery,
      FINALFUNC = rewrite_finish
);

CREATE FUNCTION tsq_mcontains(tsquery, tsquery)
        RETURNS bool
        as 'tsq_mcontains'
        LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE FUNCTION tsq_mcontained(tsquery, tsquery)
        RETURNS bool
        as 'tsq_mcontained'
        LANGUAGE INTERNAL
        RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE OPERATOR CLASS gist_tp_tsquery_ops
FOR TYPE tsquery USING gist
AS
        OPERATOR        7       @> (tsquery, tsquery),
        OPERATOR        8       <@ (tsquery, tsquery),
        FUNCTION        1       gtsquery_consistent (internal, internal, int, oid, internal),
        FUNCTION        2       gtsquery_union (internal, internal),
        FUNCTION        3       gtsquery_compress (internal),
        FUNCTION        4       gtsquery_decompress (internal),
        FUNCTION        5       gtsquery_penalty (internal, internal, internal),
        FUNCTION        6       gtsquery_picksplit (internal, internal),
        FUNCTION        7       gtsquery_same (bigint, bigint, internal),
        STORAGE         bigint;

CREATE OPERATOR CLASS gin_tsvector_ops
FOR TYPE tsvector USING gin
AS
        OPERATOR        1       @@ (tsvector, tsquery),
        OPERATOR        2       @@@ (tsvector, tsquery),
        FUNCTION        1       bttextcmp(text, text),
        FUNCTION        2       gin_extract_tsvector(tsvector,internal,internal),
        FUNCTION        3       gin_extract_tsquery(tsquery,internal,smallint,internal,internal,internal,internal),
        FUNCTION        4       gin_tsquery_consistent(internal,smallint,tsquery,int,internal,internal,internal,internal),
        FUNCTION        5       gin_cmp_prefix(text,text,smallint,internal),
        STORAGE         text;

CREATE OPERATOR CLASS tsvector_ops
FOR TYPE tsvector USING btree AS
        OPERATOR        1       < ,
        OPERATOR        2       <= ,
        OPERATOR        3       = ,
        OPERATOR        4       >= ,
        OPERATOR        5       > ,
        FUNCTION        1       tsvector_cmp(tsvector, tsvector);

CREATE OPERATOR CLASS tsquery_ops
FOR TYPE tsquery USING btree AS
        OPERATOR        1       < ,
        OPERATOR        2       <= ,
        OPERATOR        3       = ,
        OPERATOR        4       >= ,
        OPERATOR        5       > ,
        FUNCTION        1       tsquery_cmp(tsquery, tsquery);
