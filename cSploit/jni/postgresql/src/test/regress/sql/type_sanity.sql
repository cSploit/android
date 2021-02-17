--
-- TYPE_SANITY
-- Sanity checks for common errors in making type-related system tables:
-- pg_type, pg_class, pg_attribute.
--
-- None of the SELECTs here should ever find any matching entries,
-- so the expected output is easy to maintain ;-).
-- A test failure indicates someone messed up an entry in the system tables.
--
-- NB: we assume the oidjoins test will have caught any dangling links,
-- that is OID or REGPROC fields that are not zero and do not match some
-- row in the linked-to table.  However, if we want to enforce that a link
-- field can't be 0, we have to check it here.

-- **************** pg_type ****************

-- Look for illegal values in pg_type fields.

SELECT p1.oid, p1.typname
FROM pg_type as p1
WHERE p1.typnamespace = 0 OR
    (p1.typlen <= 0 AND p1.typlen != -1 AND p1.typlen != -2) OR
    (p1.typtype not in ('b', 'c', 'd', 'e', 'p')) OR
    NOT p1.typisdefined OR
    (p1.typalign not in ('c', 's', 'i', 'd')) OR
    (p1.typstorage not in ('p', 'x', 'e', 'm'));

-- Look for "pass by value" types that can't be passed by value.

SELECT p1.oid, p1.typname
FROM pg_type as p1
WHERE p1.typbyval AND
    (p1.typlen != 1 OR p1.typalign != 'c') AND
    (p1.typlen != 2 OR p1.typalign != 's') AND
    (p1.typlen != 4 OR p1.typalign != 'i') AND
    (p1.typlen != 8 OR p1.typalign != 'd');

-- Look for "toastable" types that aren't varlena.

SELECT p1.oid, p1.typname
FROM pg_type as p1
WHERE p1.typstorage != 'p' AND
    (p1.typbyval OR p1.typlen != -1);

-- Look for complex types that do not have a typrelid entry,
-- or basic types that do.

SELECT p1.oid, p1.typname
FROM pg_type as p1
WHERE (p1.typtype = 'c' AND p1.typrelid = 0) OR
    (p1.typtype != 'c' AND p1.typrelid != 0);

-- Look for basic or enum types that don't have an array type.
-- NOTE: as of 9.1, this check finds pg_node_tree, smgr, and unknown.

SELECT p1.oid, p1.typname
FROM pg_type as p1
WHERE p1.typtype in ('b','e') AND p1.typname NOT LIKE E'\\_%' AND NOT EXISTS
    (SELECT 1 FROM pg_type as p2
     WHERE p2.typname = ('_' || p1.typname)::name AND
           p2.typelem = p1.oid and p1.typarray = p2.oid);

-- Make sure typarray points to a varlena array type of our own base
SELECT p1.oid, p1.typname as basetype, p2.typname as arraytype,
       p2.typelem, p2.typlen
FROM   pg_type p1 LEFT JOIN pg_type p2 ON (p1.typarray = p2.oid)
WHERE  p1.typarray <> 0 AND
       (p2.oid IS NULL OR p2.typelem <> p1.oid OR p2.typlen <> -1);

-- Text conversion routines must be provided.

SELECT p1.oid, p1.typname
FROM pg_type as p1
WHERE (p1.typinput = 0 OR p1.typoutput = 0);

-- Check for bogus typinput routines

SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typinput = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    ((p2.pronargs = 1 AND p2.proargtypes[0] = 'cstring'::regtype) OR
     (p2.pronargs = 3 AND p2.proargtypes[0] = 'cstring'::regtype AND
      p2.proargtypes[1] = 'oid'::regtype AND
      p2.proargtypes[2] = 'int4'::regtype));

-- As of 8.0, this check finds refcursor, which is borrowing
-- other types' I/O routines
SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typinput = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    (p1.typelem != 0 AND p1.typlen < 0) AND NOT
    (p2.prorettype = p1.oid AND NOT p2.proretset)
ORDER BY 1;

-- Varlena array types will point to array_in
-- Exception as of 8.1: int2vector and oidvector have their own I/O routines
SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typinput = p2.oid AND p1.typtype in ('b', 'p') AND
    (p1.typelem != 0 AND p1.typlen < 0) AND NOT
    (p2.oid = 'array_in'::regproc)
ORDER BY 1;

-- Check for bogus typoutput routines

-- As of 8.0, this check finds refcursor, which is borrowing
-- other types' I/O routines
SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typoutput = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    (p2.pronargs = 1 AND
     (p2.proargtypes[0] = p1.oid OR
      (p2.oid = 'array_out'::regproc AND
       p1.typelem != 0 AND p1.typlen = -1)))
ORDER BY 1;

SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typoutput = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    (p2.prorettype = 'cstring'::regtype AND NOT p2.proretset);

-- Check for bogus typreceive routines

SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typreceive = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    ((p2.pronargs = 1 AND p2.proargtypes[0] = 'internal'::regtype) OR
     (p2.pronargs = 3 AND p2.proargtypes[0] = 'internal'::regtype AND
      p2.proargtypes[1] = 'oid'::regtype AND
      p2.proargtypes[2] = 'int4'::regtype));

-- As of 7.4, this check finds refcursor, which is borrowing
-- other types' I/O routines
SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typreceive = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    (p1.typelem != 0 AND p1.typlen < 0) AND NOT
    (p2.prorettype = p1.oid AND NOT p2.proretset)
ORDER BY 1;

-- Varlena array types will point to array_recv
-- Exception as of 8.1: int2vector and oidvector have their own I/O routines
SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typreceive = p2.oid AND p1.typtype in ('b', 'p') AND
    (p1.typelem != 0 AND p1.typlen < 0) AND NOT
    (p2.oid = 'array_recv'::regproc)
ORDER BY 1;

-- Suspicious if typreceive doesn't take same number of args as typinput
SELECT p1.oid, p1.typname, p2.oid, p2.proname, p3.oid, p3.proname
FROM pg_type AS p1, pg_proc AS p2, pg_proc AS p3
WHERE p1.typinput = p2.oid AND p1.typreceive = p3.oid AND
    p2.pronargs != p3.pronargs;

-- Check for bogus typsend routines

-- As of 7.4, this check finds refcursor, which is borrowing
-- other types' I/O routines
SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typsend = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    (p2.pronargs = 1 AND
     (p2.proargtypes[0] = p1.oid OR
      (p2.oid = 'array_send'::regproc AND
       p1.typelem != 0 AND p1.typlen = -1)))
ORDER BY 1;

SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typsend = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    (p2.prorettype = 'bytea'::regtype AND NOT p2.proretset);

-- Check for bogus typmodin routines

SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typmodin = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    (p2.pronargs = 1 AND
     p2.proargtypes[0] = 'cstring[]'::regtype AND
     p2.prorettype = 'int4'::regtype AND NOT p2.proretset);

-- Check for bogus typmodout routines

SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typmodout = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    (p2.pronargs = 1 AND
     p2.proargtypes[0] = 'int4'::regtype AND
     p2.prorettype = 'cstring'::regtype AND NOT p2.proretset);

-- Array types should have same typmodin/out as their element types

SELECT p1.oid, p1.typname, p2.oid, p2.typname
FROM pg_type AS p1, pg_type AS p2
WHERE p1.typelem = p2.oid AND NOT
    (p1.typmodin = p2.typmodin AND p1.typmodout = p2.typmodout);

-- Array types should have same typdelim as their element types

SELECT p1.oid, p1.typname, p2.oid, p2.typname
FROM pg_type AS p1, pg_type AS p2
WHERE p1.typarray = p2.oid AND NOT (p1.typdelim = p2.typdelim);

-- Check for bogus typanalyze routines

SELECT p1.oid, p1.typname, p2.oid, p2.proname
FROM pg_type AS p1, pg_proc AS p2
WHERE p1.typanalyze = p2.oid AND p1.typtype in ('b', 'p') AND NOT
    (p2.pronargs = 1 AND
     p2.proargtypes[0] = 'internal'::regtype AND
     p2.prorettype = 'bool'::regtype AND NOT p2.proretset);

-- **************** pg_class ****************

-- Look for illegal values in pg_class fields

SELECT p1.oid, p1.relname
FROM pg_class as p1
WHERE p1.relkind NOT IN ('r', 'i', 's', 'S', 'c', 't', 'v', 'f');

-- Indexes should have an access method, others not.

SELECT p1.oid, p1.relname
FROM pg_class as p1
WHERE (p1.relkind = 'i' AND p1.relam = 0) OR
    (p1.relkind != 'i' AND p1.relam != 0);

-- **************** pg_attribute ****************

-- Look for illegal values in pg_attribute fields

SELECT p1.attrelid, p1.attname
FROM pg_attribute as p1
WHERE p1.attrelid = 0 OR p1.atttypid = 0 OR p1.attnum = 0 OR
    p1.attcacheoff != -1 OR p1.attinhcount < 0 OR
    (p1.attinhcount = 0 AND NOT p1.attislocal);

-- Cross-check attnum against parent relation

SELECT p1.attrelid, p1.attname, p2.oid, p2.relname
FROM pg_attribute AS p1, pg_class AS p2
WHERE p1.attrelid = p2.oid AND p1.attnum > p2.relnatts;

-- Detect missing pg_attribute entries: should have as many non-system
-- attributes as parent relation expects

SELECT p1.oid, p1.relname
FROM pg_class AS p1
WHERE p1.relnatts != (SELECT count(*) FROM pg_attribute AS p2
                      WHERE p2.attrelid = p1.oid AND p2.attnum > 0);

-- Cross-check against pg_type entry
-- NOTE: we allow attstorage to be 'plain' even when typstorage is not;
-- this is mainly for toast tables.

SELECT p1.attrelid, p1.attname, p2.oid, p2.typname
FROM pg_attribute AS p1, pg_type AS p2
WHERE p1.atttypid = p2.oid AND
    (p1.attlen != p2.typlen OR
     p1.attalign != p2.typalign OR
     p1.attbyval != p2.typbyval OR
     (p1.attstorage != p2.typstorage AND p1.attstorage != 'p'));
