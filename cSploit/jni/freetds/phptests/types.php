<?php

// $Id: types.php,v 1.1 2010-07-25 10:02:56 freddy77 Exp $

require_once("pwd.inc");

$conn = odbc_connect($server,$user,$pass) or die("opps");

$sql = <<<EOSQL
CREATE TABLE php_types (
	ui SMALLINT,
	i INT,
	ti TINYINT,
	c CHAR(123),
	vc VARCHAR(125)
)
EOSQL;

odbc_exec($conn, "IF OBJECT_ID('php_types') IS NOT NULL DROP TABLE php_types") or die(odbc_errormsg());
odbc_exec($conn, $sql) or die(odbc_errormsg());
$sql = "select * from php_types";
echo "Query: $sql\n";
$result = odbc_exec($conn, $sql) or die(odbc_errormsg());

$all = array (
	'ui' => 'smallint-5',
	'i'  => 'int-10',
	'ti' => 'tinyint-3',
	'c'  => 'char-123',
	'vc' => 'varchar-125'
);

$err = '';
$ok = 0;
for($i=1;$i<=odbc_num_fields($result);$i++) {
	$name = odbc_field_name($result,$i);
	$type = odbc_field_type($result,$i);
	$len = odbc_field_len($result,$i);
	echo "column $name type $type len $len\n";
	$type = strtolower($type);
	if ($all[$name] != "$type-$len")
		$err .= "Invalid column $name\n";
	else
		++$ok;
}

if ($ok != 5)
	$err .= "Expected 5 columns\n";

if ($err) {
	echo "$err";
	exit(1);
}
echo "all columns seems ok\n";

odbc_exec($conn, "IF OBJECT_ID('php_types') IS NOT NULL DROP TABLE php_types") or die(odbc_errormsg());

exit(0);
?>
