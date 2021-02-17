<html>
<body bgcolor=white>
<?php
$dbproc = sybase_connect("JDBC","guest","sybase");
if (! $dbproc) {
	return;
}
$res = sybase_query("select * from test",$dbproc);
if (! $res) {
	return;
}
while ($arr = sybase_fetch_array($res)) {
	print $arr["i"] . " " . $arr["v"] . "<br>\n";
}
?>
</body>
</html>
