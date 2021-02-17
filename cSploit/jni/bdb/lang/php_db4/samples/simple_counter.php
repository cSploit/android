<?php
// Create a new Db4 Instance
$db = new Db4();

// Open it outside a Db4Env environment with datafile db4 
// and database name "test."  This creates a non-transactional database
$db->open(null, "./db4", "test");

// Get the current value of "counter"
$counter = $db->get("counter");
print "Counter Value is $counter\n";

// Increment $counter and put() it.
$db->put("counter", $counter+1);
// Sync to be certain, since we're leaving the handle open
$db->sync();
?>
