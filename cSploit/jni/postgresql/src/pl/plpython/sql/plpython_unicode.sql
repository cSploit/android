--
-- Unicode handling
--

SET client_encoding TO UTF8;

CREATE TABLE unicode_test (
	testvalue  text NOT NULL
);

CREATE FUNCTION unicode_return() RETURNS text AS E'
return u"\\x80"
' LANGUAGE plpythonu;

CREATE FUNCTION unicode_trigger() RETURNS trigger AS E'
TD["new"]["testvalue"] = u"\\x80"
return "MODIFY"
' LANGUAGE plpythonu;

CREATE TRIGGER unicode_test_bi BEFORE INSERT ON unicode_test
  FOR EACH ROW EXECUTE PROCEDURE unicode_trigger();

CREATE FUNCTION unicode_plan1() RETURNS text AS E'
plan = plpy.prepare("SELECT $1 AS testvalue", ["text"])
rv = plpy.execute(plan, [u"\\x80"], 1)
return rv[0]["testvalue"]
' LANGUAGE plpythonu;

CREATE FUNCTION unicode_plan2() RETURNS text AS E'
plan = plpy.prepare("SELECT $1 || $2 AS testvalue", ["text", u"text"])
rv = plpy.execute(plan, ["foo", "bar"], 1)
return rv[0]["testvalue"]
' LANGUAGE plpythonu;


SELECT unicode_return();
INSERT INTO unicode_test (testvalue) VALUES ('test');
SELECT * FROM unicode_test;
SELECT unicode_plan1();
SELECT unicode_plan2();
