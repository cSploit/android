<?php

include('./pwd.inc');
$conn = mssql_connect($server,$user,$pass) or die("opps");

mssql_query("if exists (select * from sysobjects where name = 'php_test') drop table php_test", $conn);
mssql_query("if exists (select * from sysobjects where name = 'sp_php_test') drop proc sp_php_test", $conn);
mssql_query("create table php_test(id int, c varchar(20))", $conn) or die("creating table");

mssql_query("insert into php_test values(1, 'aaa')", $conn) or die("inserting data");
mssql_query("insert into php_test values(10, 'meeewww')", $conn) or die("inserting data");

$sql = <<<EOSQL
CREATE PROCEDURE sp_php_test AS
	-- first Query: no match
	SELECT * FROM php_test WHERE id=10000
	-- second Query: 2 matches
	SELECT * FROM php_test
EOSQL;

mssql_query($sql, $conn) or die("error querying");

# one time to let sql compile and compute statistics on table
$rs = mssql_query("sp_php_test", $conn) or die("error querying");
mssql_next_result($rs);
mssql_next_result($rs);
mssql_free_result($rs);	

$err = "";
$num_res = 0;
$stmt = mssql_init("sp_php_test", $conn);
$rs = mssql_execute($stmt);
if (is_resource($rs)) {
	echo "fetching...\n";
	do {
		++$num_res;
		echo "Processing result $num_res\n";
		echo "Num rows = " . mssql_num_rows($rs) . "\n";
		if (!$err && $num_res == 1 && mssql_num_rows($rs) != 0)
			$err = "Expected 0 rows from recordset $num_res";
		if (!$err && $num_res == 2 && mssql_num_rows($rs) != 2)
			$err = "Expected 2 rows from recordset $num_res";
		// Process result
	} while (mssql_next_result($rs));
	mssql_free_result($rs);	
}

if (!$err && $num_res != 2)
	$err = "Expected 2 resultset got $num_res";

# cleanup
mssql_query("drop proc sp_php_test");
mssql_query("drop table php_test");

mssql_close($conn);

if ($err) {
	echo "$err\n";
	exit(1);
}

exit(0);
?>
