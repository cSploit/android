<html>
<head>
	<title>MS SQL Query</title>
    <META NAME="author" CONTENT="Sven Goldt">
    <META NAME="description" CONTENT="Send simple queries to MS SQL Database with freetdsl and php">
</head>

<body>
<?php

$dbserver="MyServer70"; // Entry in freetds.conf
$dbuser="Username"; // Replace with DB username
$dbpass="Password"; // Replace with DB password
$dbname="master"; // could work, but should be a DB of $dbuser

$query="SELECT * FROM sysobjects WHERE type='U'";
if (strlen($_REQUEST["sqlquery"]) > 0) {
 $query=stripslashes($_REQUEST["sqlquery"]);
}


$mslink=mssql_connect("$dbserver" , "$dbuser" , "$dbpass" ) or
die("Could not connect to $dbserver: " . mssql_get_last_message());
 
$msdb=mssql_select_db($dbname,$mslink) or 
die("Could not select $dbname: " . mssql_get_last_message());

$result=mssql_query($query) or 
die("Could not $query: " . mssql_get_last_message());

$numRows = mssql_num_rows($result); 
$numfields = mssql_num_fields($result);
$today=date("D M j G:i:s T Y");

echo <<<EOF
<P>
$today
<P>
<form action="$PHP_SELF" method="POST">
<input type="text" name="sqlquery" size="50" value="$query">
<input type="submit" value="query">
</form>
<P>
<table border="1">
<tr>
EOF;
for ($i=0; $i<$numfields; $i++) {
 echo "<th>" . mssql_field_name($result,$i) . "</th>";
}
echo "</tr>\n";
while($row = mssql_fetch_row($result))
{
 echo "<tr>";
 foreach ($row as $fieldvalue) 
 {
  echo "<td>$fieldvalue</td>";
 }
 echo "</tr>\n";
} 
mssql_close($mslink);
?>
</table>

</body>
</html>
