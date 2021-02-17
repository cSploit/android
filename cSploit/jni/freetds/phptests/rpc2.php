<?php

include('./pwd.inc');
$conn = mssql_connect($server,$user,$pass) or die("opps");

mssql_query("if exists (select * from sysobjects where name = 'sp_php_test1') drop proc sp_php_test1", $conn);
mssql_query("if exists (select * from sysobjects where name = 'sp_php_test2') drop proc sp_php_test2", $conn);
mssql_query("if exists (select * from sysobjects where name = 'sp_php_test3') drop proc sp_php_test3", $conn);
mssql_query("if exists (select * from sysobjects where name = 'sp_php_test4') drop proc sp_php_test4", $conn);

mssql_query("CREATE PROC sp_php_test1
@test1 as int 
AS
DECLARE @bla int
SELECT @bla = 10 * @test1
RETURN  @bla
", $conn) or die("error querying");

mssql_query("CREATE PROC sp_php_test2
@test1 as VARCHAR(20)
AS
DECLARE @retval int
SELECT @retval = 2
RETURN  @retval
", $conn) or die("error querying");

mssql_query("CREATE PROC sp_php_test3
@test1 as int  OUT
AS
SELECT @test1 = 10
RETURN 3
", $conn) or die("error querying");

mssql_query("CREATE PROC sp_php_test4
@test1 as varchar(20) OUT
AS
SELECT @test1 = 'xyz45678'
RETURN  4
", $conn) or die("error querying");

$proc = mssql_init("sp_php_test1", $conn);

$err = FALSE;



// Test 1, SQLINT4 input
//

$test1 = 35;
$retval = 0;

// Bind the parameters
mssql_bind($proc, "@test1", $test1, SQLINT4, FALSE, FALSE);

// Bind the return value
mssql_bind($proc, "RETVAL", &$retval, SQLINT4);

$result = mssql_execute($proc);
if (!$result){
	echo "Last message from SQL : " . mssql_get_last_message() ." \n";
	$err = TRUE;
	mssql_free_statement($proc);
} else {
	mssql_free_statement($proc);

	echo "test1 = $test1\n";
	echo "retval = $retval\n";
	if ($retval != 350) {
		echo "Expected retval 350\n";
		$err = TRUE;
	}
}



// Test 2, SQLVARCHAR input
//
$proc = mssql_init("sp_php_test2", $conn);

$test1 = "blaatschaap";
$retval = 0;

// Bind the parameters
mssql_bind($proc, "@test1", $test1, SQLVARCHAR, FALSE, FALSE, 20);

// Bind the return value
mssql_bind($proc, "RETVAL", &$retval, SQLINT4);

$result = mssql_execute($proc);
if (!$result){
	echo "Last message from SQL : " . mssql_get_last_message() ."\n";
	$err = TRUE;
	mssql_free_statement($proc);
} else {
	mssql_free_statement($proc);

	echo "test1 = $test1\n";
	echo "retval = $retval\n";
	if ($retval != 2) {
		echo "Expected retval 2\n";
		$err = TRUE;
	}
}



// Test 3, SQLINT4 output
//
$proc = mssql_init("sp_php_test3", $conn);

$test1 = 0;
$retval = 0;

// Bind the parameters
mssql_bind($proc, "@test1", &$test1, SQLINT4, TRUE, FALSE);

// Bind the return value
mssql_bind($proc, "RETVAL", &$retval, SQLINT4);

$result = mssql_execute($proc);
if (!$result){
	echo "Last message from SQL : " . mssql_get_last_message() ."\n";
	$err = TRUE;
	mssql_free_statement($proc);
} else {
	mssql_free_statement($proc);

	echo "test1 = $test1\n";
	if ($test1 != 10) {
		echo "Expected test1 10\n";
		$err = TRUE;
	}
	echo "retval = $retval\n";
	if ($retval != 3) {
		echo "Expected retval 3\n";
		$err = TRUE;
	}
}



// Test 4, SQLVARCHAR output
//
$proc = mssql_init("sp_php_test4", $conn);

$test1 = "";
$retval = 0;

// Bind the parameters
mssql_bind($proc, "@test1", &$test1, SQLVARCHAR, TRUE, FALSE, 13);

// Bind the return value
mssql_bind($proc, "RETVAL", &$retval, SQLINT4);

$result = mssql_execute($proc);
if (!$result){
	echo "Last message from SQL : " . mssql_get_last_message() ."\n";
	$err = TRUE;
	mssql_free_statement($proc);
} else {
	mssql_free_statement($proc);

	echo "test1 = >$test1<\n";
	if ($test1 != 'xyz45678') {
		echo "Expected test1 >xyz45678<\n";
		$err = TRUE;
	}
	echo "retval = $retval\n";
	if ($retval != 4) {
		echo "Expected retval 4\n";
		$err = TRUE;
	}
}

# cleanup
mssql_query("drop proc sp_php_test4");
mssql_query("drop proc sp_php_test3");
mssql_query("drop proc sp_php_test2");
mssql_query("drop proc sp_php_test1");

mssql_close($conn);

if ($err)
	exit(1);

exit(0);
?>
