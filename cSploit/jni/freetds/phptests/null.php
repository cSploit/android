<?php

// $Id: null.php,v 1.3 2007-02-05 08:42:32 freddy77 Exp $

require_once("pwd.inc");

$conn = mssql_connect($server,$user,$pass) or die("opps");

mssql_query("CREATE TABLE #MyTable (
   myfield VARCHAR(10) NULL,
   n INT
)", $conn) or die("error querying");


mssql_query("INSERT INTO #MyTable VALUES('',1)
INSERT INTO #MyTable VALUES(NULL,2)
INSERT INTO #MyTable VALUES(' ',3)
INSERT INTO #MyTable VALUES('a',4)", $conn) or die("error querying");

$result = 0;

function test($sql, $expected)
{
	global $conn, $result;

	$res = mssql_query($sql, $conn) or die("query");
	$row = mssql_fetch_assoc($res);
	$s = $row['myfield'];

	if (is_null($s))
		$s = '(NULL)';
	else if ($s == '')
		$s = '(Empty String)';
	else
		$s = "'".str_replace("'", "''", $s)."'";
	echo "$sql -> $s\n";
	if ($s != $expected)
	{
		echo "error!\n";
		$result = 1;
	}
}

test("SELECT top 1 * FROM #MyTable WHERE n = 1 -- ''", "(Empty String)");

test("SELECT top 1 * FROM #MyTable WHERE n = 2 -- NULL", "(NULL)");

test("SELECT top 1 * FROM #MyTable WHERE n = 3 -- ' '", "' '");

test("SELECT top 1 * FROM #MyTable WHERE n = 4 -- 'a'", "'a'");

exit($result);
?>
