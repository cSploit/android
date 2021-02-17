--
-- Enum tests
--

CREATE TYPE rainbow AS ENUM ('red', 'orange', 'yellow', 'green', 'blue', 'purple');

--
-- Did it create the right number of rows?
--
SELECT COUNT(*) FROM pg_enum WHERE enumtypid = 'rainbow'::regtype;

--
-- I/O functions
--
SELECT 'red'::rainbow;
SELECT 'mauve'::rainbow;

--
-- adding new values
--

CREATE TYPE planets AS ENUM ( 'venus', 'earth', 'mars' );

SELECT enumlabel, enumsortorder
FROM pg_enum
WHERE enumtypid = 'planets'::regtype
ORDER BY 2;

ALTER TYPE planets ADD VALUE 'uranus';

SELECT enumlabel, enumsortorder
FROM pg_enum
WHERE enumtypid = 'planets'::regtype
ORDER BY 2;

ALTER TYPE planets ADD VALUE 'mercury' BEFORE 'venus';
ALTER TYPE planets ADD VALUE 'saturn' BEFORE 'uranus';
ALTER TYPE planets ADD VALUE 'jupiter' AFTER 'mars';
ALTER TYPE planets ADD VALUE 'neptune' AFTER 'uranus';

SELECT enumlabel, enumsortorder
FROM pg_enum
WHERE enumtypid = 'planets'::regtype
ORDER BY 2;

SELECT enumlabel, enumsortorder
FROM pg_enum
WHERE enumtypid = 'planets'::regtype
ORDER BY enumlabel::planets;

-- errors for adding labels
ALTER TYPE planets ADD VALUE
  'plutoplutoplutoplutoplutoplutoplutoplutoplutoplutoplutoplutoplutopluto';

ALTER TYPE planets ADD VALUE 'pluto' AFTER 'zeus';

--
-- Test inserting so many values that we have to renumber
--

create type insenum as enum ('L1', 'L2');

alter type insenum add value 'i1' before 'L2';
alter type insenum add value 'i2' before 'L2';
alter type insenum add value 'i3' before 'L2';
alter type insenum add value 'i4' before 'L2';
alter type insenum add value 'i5' before 'L2';
alter type insenum add value 'i6' before 'L2';
alter type insenum add value 'i7' before 'L2';
alter type insenum add value 'i8' before 'L2';
alter type insenum add value 'i9' before 'L2';
alter type insenum add value 'i10' before 'L2';
alter type insenum add value 'i11' before 'L2';
alter type insenum add value 'i12' before 'L2';
alter type insenum add value 'i13' before 'L2';
alter type insenum add value 'i14' before 'L2';
alter type insenum add value 'i15' before 'L2';
alter type insenum add value 'i16' before 'L2';
alter type insenum add value 'i17' before 'L2';
alter type insenum add value 'i18' before 'L2';
alter type insenum add value 'i19' before 'L2';
alter type insenum add value 'i20' before 'L2';
alter type insenum add value 'i21' before 'L2';
alter type insenum add value 'i22' before 'L2';
alter type insenum add value 'i23' before 'L2';
alter type insenum add value 'i24' before 'L2';
alter type insenum add value 'i25' before 'L2';
alter type insenum add value 'i26' before 'L2';
alter type insenum add value 'i27' before 'L2';
alter type insenum add value 'i28' before 'L2';
alter type insenum add value 'i29' before 'L2';
alter type insenum add value 'i30' before 'L2';

-- The exact values of enumsortorder will now depend on the local properties
-- of float4, but in any reasonable implementation we should get at least
-- 20 splits before having to renumber; so only hide values > 20.

SELECT enumlabel,
       case when enumsortorder > 20 then null else enumsortorder end as so
FROM pg_enum
WHERE enumtypid = 'insenum'::regtype
ORDER BY enumsortorder;

--
-- Basic table creation, row selection
--
CREATE TABLE enumtest (col rainbow);
INSERT INTO enumtest values ('red'), ('orange'), ('yellow'), ('green');
COPY enumtest FROM stdin;
blue
purple
\.
SELECT * FROM enumtest;

--
-- Operators, no index
--
SELECT * FROM enumtest WHERE col = 'orange';
SELECT * FROM enumtest WHERE col <> 'orange' ORDER BY col;
SELECT * FROM enumtest WHERE col > 'yellow' ORDER BY col;
SELECT * FROM enumtest WHERE col >= 'yellow' ORDER BY col;
SELECT * FROM enumtest WHERE col < 'green' ORDER BY col;
SELECT * FROM enumtest WHERE col <= 'green' ORDER BY col;

--
-- Cast to/from text
--
SELECT 'red'::rainbow::text || 'hithere';
SELECT 'red'::text::rainbow = 'red'::rainbow;

--
-- Aggregates
--
SELECT min(col) FROM enumtest;
SELECT max(col) FROM enumtest;
SELECT max(col) FROM enumtest WHERE col < 'green';

--
-- Index tests, force use of index
--
SET enable_seqscan = off;
SET enable_bitmapscan = off;

--
-- Btree index / opclass with the various operators
--
CREATE UNIQUE INDEX enumtest_btree ON enumtest USING btree (col);
SELECT * FROM enumtest WHERE col = 'orange';
SELECT * FROM enumtest WHERE col <> 'orange' ORDER BY col;
SELECT * FROM enumtest WHERE col > 'yellow' ORDER BY col;
SELECT * FROM enumtest WHERE col >= 'yellow' ORDER BY col;
SELECT * FROM enumtest WHERE col < 'green' ORDER BY col;
SELECT * FROM enumtest WHERE col <= 'green' ORDER BY col;
SELECT min(col) FROM enumtest;
SELECT max(col) FROM enumtest;
SELECT max(col) FROM enumtest WHERE col < 'green';
DROP INDEX enumtest_btree;

--
-- Hash index / opclass with the = operator
--
CREATE INDEX enumtest_hash ON enumtest USING hash (col);
SELECT * FROM enumtest WHERE col = 'orange';
DROP INDEX enumtest_hash;

--
-- End index tests
--
RESET enable_seqscan;
RESET enable_bitmapscan;

--
-- Domains over enums
--
CREATE DOMAIN rgb AS rainbow CHECK (VALUE IN ('red', 'green', 'blue'));
SELECT 'red'::rgb;
SELECT 'purple'::rgb;
SELECT 'purple'::rainbow::rgb;
DROP DOMAIN rgb;

--
-- Arrays
--
SELECT '{red,green,blue}'::rainbow[];
SELECT ('{red,green,blue}'::rainbow[])[2];
SELECT 'red' = ANY ('{red,green,blue}'::rainbow[]);
SELECT 'yellow' = ANY ('{red,green,blue}'::rainbow[]);
SELECT 'red' = ALL ('{red,green,blue}'::rainbow[]);
SELECT 'red' = ALL ('{red,red}'::rainbow[]);

--
-- Support functions
--
SELECT enum_first(NULL::rainbow);
SELECT enum_last('green'::rainbow);
SELECT enum_range(NULL::rainbow);
SELECT enum_range('orange'::rainbow, 'green'::rainbow);
SELECT enum_range(NULL, 'green'::rainbow);
SELECT enum_range('orange'::rainbow, NULL);
SELECT enum_range(NULL::rainbow, NULL);

--
-- User functions, can't test perl/python etc here since may not be compiled.
--
CREATE FUNCTION echo_me(anyenum) RETURNS text AS $$
BEGIN
RETURN $1::text || 'omg';
END
$$ LANGUAGE plpgsql;
SELECT echo_me('red'::rainbow);
--
-- Concrete function should override generic one
--
CREATE FUNCTION echo_me(rainbow) RETURNS text AS $$
BEGIN
RETURN $1::text || 'wtf';
END
$$ LANGUAGE plpgsql;
SELECT echo_me('red'::rainbow);
--
-- If we drop the original generic one, we don't have to qualify the type
-- anymore, since there's only one match
--
DROP FUNCTION echo_me(anyenum);
SELECT echo_me('red');
DROP FUNCTION echo_me(rainbow);

--
-- RI triggers on enum types
--
CREATE TABLE enumtest_parent (id rainbow PRIMARY KEY);
CREATE TABLE enumtest_child (parent rainbow REFERENCES enumtest_parent);
INSERT INTO enumtest_parent VALUES ('red');
INSERT INTO enumtest_child VALUES ('red');
INSERT INTO enumtest_child VALUES ('blue');  -- fail
DELETE FROM enumtest_parent;  -- fail
--
-- cross-type RI should fail
--
CREATE TYPE bogus AS ENUM('good', 'bad', 'ugly');
CREATE TABLE enumtest_bogus_child(parent bogus REFERENCES enumtest_parent);
DROP TYPE bogus;

--
-- Cleanup
--
DROP TABLE enumtest_child;
DROP TABLE enumtest_parent;
DROP TABLE enumtest;
DROP TYPE rainbow;

--
-- Verify properly cleaned up
--
SELECT COUNT(*) FROM pg_type WHERE typname = 'rainbow';
SELECT * FROM pg_enum WHERE NOT EXISTS
  (SELECT 1 FROM pg_type WHERE pg_type.oid = enumtypid);
