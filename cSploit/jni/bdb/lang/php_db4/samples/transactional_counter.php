<?php

// Open a new Db4Env.  By default it is transactional.  The directory
// path in the open() call must exist.
$dbenv = new Db4Env();
$dbenv->set_data_dir(".");
$dbenv->open(".");

// Open a database in $dbenv.
$db = new Db4($dbenv);
$txn = $dbenv->txn_begin();
$db->open($txn, 'a', 'foo');
$txn->commit();

$counter = $db->get("counter");
// Create a new transaction
$txn = $dbenv->txn_begin();
if($txn == false) {
  print "txn_begin failed";
  exit;
}
print "Current value of counter is $counter\n";

// Increment and reset counter, protect it with $txn
$db->put("counter", $counter+1, $txn);

// Commit the transaction, otherwise the above put() will rollback.
$txn->commit();
// Sync for good measure
$db->sync();
// This isn't a real close, use _close() for that.
$db->close();
?>
