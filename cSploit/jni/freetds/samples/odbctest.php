<? 
// connect to a DSN "mydb" with a user and password "marin" 
$connect = odbc_connect("JDBC", "guest", "sybase");

// query the users table for name and surname
$query = "SELECT * from sysusers";


// perform the query
$result = odbc_exec($connect, $query);


// fetch the data from the database
while(odbc_fetch_row($result)){
	$suid = odbc_result($result, 1);
	$uid = odbc_result($result, 2);
	$gid = odbc_result($result, 3);
	$name = odbc_result($result, 4);
	print("$name|$suid|$uid|$gid\n");
}

// close the connection
odbc_close($connect);
?>
