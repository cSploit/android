<?php
require_once("pwd.inc");

$conn = mssql_connect($server,$user,$pass) or die("opps");

$sql = <<<EOSQL
DECLARE Search CURSOR LOCAL SCROLL READ_ONLY FOR
   SELECT * FROM sysobjects
DECLARE @limit INT, @offset INT
SET @limit = 20
SET @offset = 4
OPEN Search
FETCH ABSOLUTE @offset FROM Search
WHILE @@FETCH_STATUS =0 AND @limit > 1
BEGIN
  FETCH NEXT FROM Search
  SET @limit = @limit -1
END
CLOSE Search
DEALLOCATE Search
EOSQL;

$num_rows = 0;
$res = mssql_query($sql) or die("query");
$row = mssql_fetch_assoc($res);
while ($row) {
  ++$num_rows;
//  print_r($row);
  echo "got a row, name is $row[name]\n";
  $row = mssql_fetch_assoc($res);
  if (!$row) {
    if (mssql_next_result($res)) {
       $row = mssql_fetch_assoc($res);
    }
  }
}

if ($num_rows < 2) {
  echo "Expected more than a row\n";
  exit(1);
}
exit(0);
?>
