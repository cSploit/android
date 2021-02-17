--
-- Test explicit subtransactions
--

-- Test table to see if transactions get properly rolled back

CREATE TABLE subtransaction_tbl (
    i integer
);

-- Explicit case for Python <2.6

CREATE FUNCTION subtransaction_test(what_error text = NULL) RETURNS text
AS $$
import sys
subxact = plpy.subtransaction()
subxact.__enter__()
exc = True
try:
    try:
        plpy.execute("INSERT INTO subtransaction_tbl VALUES (1)")
        plpy.execute("INSERT INTO subtransaction_tbl VALUES (2)")
        if what_error == "SPI":
            plpy.execute("INSERT INTO subtransaction_tbl VALUES ('oops')")
        elif what_error == "Python":
            plpy.attribute_error
    except:
        exc = False
        subxact.__exit__(*sys.exc_info())
        raise
finally:
    if exc:
        subxact.__exit__(None, None, None)
$$ LANGUAGE plpythonu;

SELECT subtransaction_test();
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;
SELECT subtransaction_test('SPI');
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;
SELECT subtransaction_test('Python');
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;

-- Context manager case for Python >=2.6

CREATE FUNCTION subtransaction_ctx_test(what_error text = NULL) RETURNS text
AS $$
with plpy.subtransaction():
    plpy.execute("INSERT INTO subtransaction_tbl VALUES (1)")
    plpy.execute("INSERT INTO subtransaction_tbl VALUES (2)")
    if what_error == "SPI":
        plpy.execute("INSERT INTO subtransaction_tbl VALUES ('oops')")
    elif what_error == "Python":
        plpy.attribute_error
$$ LANGUAGE plpythonu;

SELECT subtransaction_ctx_test();
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;
SELECT subtransaction_ctx_test('SPI');
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;
SELECT subtransaction_ctx_test('Python');
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;

-- Nested subtransactions

CREATE FUNCTION subtransaction_nested_test(swallow boolean = 'f') RETURNS text
AS $$
plpy.execute("INSERT INTO subtransaction_tbl VALUES (1)")
with plpy.subtransaction():
    plpy.execute("INSERT INTO subtransaction_tbl VALUES (2)")
    try:
        with plpy.subtransaction():
            plpy.execute("INSERT INTO subtransaction_tbl VALUES (3)")
            plpy.execute("error")
    except plpy.SPIError, e:
        if not swallow:
            raise
        plpy.notice("Swallowed %r" % e)
return "ok"
$$ LANGUAGE plpythonu;

SELECT subtransaction_nested_test();
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;

SELECT subtransaction_nested_test('t');
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;

-- Nested subtransactions that recursively call code dealing with
-- subtransactions

CREATE FUNCTION subtransaction_deeply_nested_test() RETURNS text
AS $$
plpy.execute("INSERT INTO subtransaction_tbl VALUES (1)")
with plpy.subtransaction():
    plpy.execute("INSERT INTO subtransaction_tbl VALUES (2)")
    plpy.execute("SELECT subtransaction_nested_test('t')")
return "ok"
$$ LANGUAGE plpythonu;

SELECT subtransaction_deeply_nested_test();
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;

-- Error conditions from not opening/closing subtransactions

CREATE FUNCTION subtransaction_exit_without_enter() RETURNS void
AS $$
plpy.subtransaction().__exit__(None, None, None)
$$ LANGUAGE plpythonu;

CREATE FUNCTION subtransaction_enter_without_exit() RETURNS void
AS $$
plpy.subtransaction().__enter__()
$$ LANGUAGE plpythonu;

CREATE FUNCTION subtransaction_exit_twice() RETURNS void
AS $$
plpy.subtransaction().__enter__()
plpy.subtransaction().__exit__(None, None, None)
plpy.subtransaction().__exit__(None, None, None)
$$ LANGUAGE plpythonu;

CREATE FUNCTION subtransaction_enter_twice() RETURNS void
AS $$
plpy.subtransaction().__enter__()
plpy.subtransaction().__enter__()
$$ LANGUAGE plpythonu;

CREATE FUNCTION subtransaction_exit_same_subtransaction_twice() RETURNS void
AS $$
s = plpy.subtransaction()
s.__enter__()
s.__exit__(None, None, None)
s.__exit__(None, None, None)
$$ LANGUAGE plpythonu;

CREATE FUNCTION subtransaction_enter_same_subtransaction_twice() RETURNS void
AS $$
s = plpy.subtransaction()
s.__enter__()
s.__enter__()
s.__exit__(None, None, None)
$$ LANGUAGE plpythonu;

-- No warnings here, as the subtransaction gets indeed closed
CREATE FUNCTION subtransaction_enter_subtransaction_in_with() RETURNS void
AS $$
with plpy.subtransaction() as s:
    s.__enter__()
$$ LANGUAGE plpythonu;

CREATE FUNCTION subtransaction_exit_subtransaction_in_with() RETURNS void
AS $$
with plpy.subtransaction() as s:
    s.__exit__(None, None, None)
$$ LANGUAGE plpythonu;

SELECT subtransaction_exit_without_enter();
SELECT subtransaction_enter_without_exit();
SELECT subtransaction_exit_twice();
SELECT subtransaction_enter_twice();
SELECT subtransaction_exit_same_subtransaction_twice();
SELECT subtransaction_enter_same_subtransaction_twice();
SELECT subtransaction_enter_subtransaction_in_with();
SELECT subtransaction_exit_subtransaction_in_with();

-- Make sure we don't get a "current transaction is aborted" error
SELECT 1 as test;

-- Mix explicit subtransactions and normal SPI calls

CREATE FUNCTION subtransaction_mix_explicit_and_implicit() RETURNS void
AS $$
p = plpy.prepare("INSERT INTO subtransaction_tbl VALUES ($1)", ["integer"])
try:
    with plpy.subtransaction():
        plpy.execute("INSERT INTO subtransaction_tbl VALUES (1)")
        plpy.execute(p, [2])
        plpy.execute(p, ["wrong"])
except plpy.SPIError:
    plpy.warning("Caught a SPI error from an explicit subtransaction")

try:
    plpy.execute("INSERT INTO subtransaction_tbl VALUES (1)")
    plpy.execute(p, [2])
    plpy.execute(p, ["wrong"])
except plpy.SPIError:
    plpy.warning("Caught a SPI error")
$$ LANGUAGE plpythonu;

SELECT subtransaction_mix_explicit_and_implicit();
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;

-- Alternative method names for Python <2.6

CREATE FUNCTION subtransaction_alternative_names() RETURNS void
AS $$
s = plpy.subtransaction()
s.enter()
s.exit(None, None, None)
$$ LANGUAGE plpythonu;

SELECT subtransaction_alternative_names();

-- try/catch inside a subtransaction block

CREATE FUNCTION try_catch_inside_subtransaction() RETURNS void
AS $$
with plpy.subtransaction():
     plpy.execute("INSERT INTO subtransaction_tbl VALUES (1)")
     try:
         plpy.execute("INSERT INTO subtransaction_tbl VALUES ('a')")
     except plpy.SPIError:
         plpy.notice("caught")
$$ LANGUAGE plpythonu;

SELECT try_catch_inside_subtransaction();
SELECT * FROM subtransaction_tbl;
TRUNCATE subtransaction_tbl;

ALTER TABLE subtransaction_tbl ADD PRIMARY KEY (i);

CREATE FUNCTION pk_violation_inside_subtransaction() RETURNS void
AS $$
with plpy.subtransaction():
     plpy.execute("INSERT INTO subtransaction_tbl VALUES (1)")
     try:
         plpy.execute("INSERT INTO subtransaction_tbl VALUES (1)")
     except plpy.SPIError:
         plpy.notice("caught")
$$ LANGUAGE plpythonu;

SELECT pk_violation_inside_subtransaction();
SELECT * FROM subtransaction_tbl;

DROP TABLE subtransaction_tbl;
