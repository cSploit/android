#! /usr/bin/perl -w

use strict;
use diagnostics;
use Socket;

use Devel::Peek;

use ncpfs;

my ($test_objectname, $test_objecttype, $test_objectid, $server_name, $nds_volume, $test_base);

$test_objectname = 'ADMIN';
$test_objecttype = 1;
$test_objectid   = 0xBD000001;

$server_name = 'VMWARE-NW6';

$nds_volume = 'CN=VMWARE-NW6_SYS.O=VANASOFT.C=CZ';

$test_base = 'C=CZ';

sub errorinfo($) {
	my ($err) = @_;
	
	if ($err) {
		printf "failed, %s\n", ncpfs::strnwerror($err);
		return 0;
	}
	return 1;
}

sub errorall($) {
	my ($err) = @_;
	if (errorinfo($err)) {
		printf "ok\n";
	}
}

sub checkNWpath_int {
	my ($r1, $r2) = @_;
	my ($ret, $stat);

	print "ParsePath (array mode): $r1";
	if (defined($r2)) {
		print "($r2)";
		($stat, $ret) = ncpfs::ncp_path_to_NW_format($r1, $r2);
	} else {
		($stat, $ret) = ncpfs::ncp_path_to_NW_format($r1);
	}
	print " => ";
	if (errorinfo($stat)) {
		print "OK, ".unpack('H*', $ret)."\n";
	}

	print "ParsePath (scalar mode): $r1";
	if (defined($r2)) {
		print "($r2)";
		$stat = ncpfs::ncp_path_to_NW_format($r1, $r2);
	} else {
		$stat = ncpfs::ncp_path_to_NW_format($r1);
	}
	print " => ";
	if (not defined($stat)) {
		print "Err\n";
	} else {
		print "OK, ".unpack('H*', $stat)."\n";
	}
}

sub checkNWpath() {
	checkNWpath_int('SYS:SYSTEM/LOPATA/AHOJ.TST');
	checkNWpath_int('SYS:SYSTEM/LOPATA/AHOJ.TST', 3);
	checkNWpath_int('SYS:///////////');
	checkNWpath_int('/.', 55);
	checkNWpath_int('A' x 256);
}

sub gsinfo($) {
	my ($conn) = @_;
	my ($err);
my ($d,$e,$f,$g,$h);
	
print "OK, Connection=$conn\n";
#($err,$d,$e,$f,$g,$h) = ncpfs::ncp_get_file_server_description_strings($conn);
#print 'Status: '.ncpfs::strnwerror($err).", desc=$d/$e/$f/$g/$h\n";
#loginUser($conn, "IDOS", "POKUS");
for ($e = 0; $e < 1; $e++) {
	my $f;
for ($f = 0; $f < 1; $f++) {
	($err,$d) = ncpfs::ncp_get_file_server_information($conn);
#	bless $d,"ncpfs::ncp_file_server_info";
	if ($err) {
		printf "GetFSInfo failed with %s\n", ncpfs::strnwerror($err);
		last;
	}
	if (1) {
	print "D=$d\n";
	print 'Status: '.ncpfs::strnwerror($err).", info=$d\n";
	print 'Name1: '.$d->{'ServerName'}."\n";
	print 'Name2: '.$d->ServerName()."\n";
	print 'Name3: '.$d->ServerName."\n";
	print 'Name: '.$d->ServerName.", Version: ".$d->FileServiceVersion.".".$d->FileServiceSubVersion.".".$d->Revision."\n";
	print 'Conns: Max: '.$d->MaximumServiceConnections.", InUse: ".$d->ConnectionsInUse.", Top: ".$d->MaxConnectionsEverUsed."\n";
	print 'Max volumes: '.$d->NumberMountedVolumes.", SFT: ".$d->SFTLevel.", TTS: ".$d->TTSLevel."\n";
	print 'Version: Account: '.$d->AccountVersion.", VAP: ".$d->VAPVersion.", Queue: ".$d->QueueVersion.", Print: ".$d->PrintVersion;
	print ", VCVersion: ".$d->VirtualConsoleVersion."\n";
	print 'Restriction level: '.$d->RestrictionLevel.", Bridge: ".$d->InternetBridge."\n";
	print 'Reserved: '.$d->Reserved."\n";

	print "EE=".$d->{'ServerName'}."\n";

	$d->ServerName('Ahoj');
	print $d->ServerName."\n";

	print "EE=".$d->{'ServerName'}."\n";

	print $d->ServerName('Cau')."\n";
	print $d->ServerName."\n";

	print join(',', keys %$d);
	print "\n";

	foreach $g (keys %$d) {
		my $h = $d->{$g};
		if ($g eq 'Reserved') {
			$h = unpack('H*', $h);
		}
		printf "%30s => $h\n", $g;
	}
	print $d->Reserved()."\n";

#	if (0) {
#	while (($g,$h) = each(%$d)) {
#		printf "%30s => $h %s %s\n", $g, 'x', $d->$g;
#	}
#	}
	print "$d\n";
	}
}
}
}

sub dumpObjectInfo($) {
	my $z = shift;

	return sprintf "%08X | %04X | %02X | %02X | %02X | %-48s", $z->object_id(), $z->object_type(), $z->object_flags(),
		$z->object_security(), $z->object_has_prop(), $z->object_name();
}

sub dumpBindery($$$) {
	my ($conn,$type,$name) = @_;
	my $m = -1;
	my $err;

	print "Bindery listing\n";
	print "ObjectID | Type | Fl | Sc | Pr | Name\n";
	print "---------+------+----+----+----+--------------------\n";
	while (1) {
		my $z;
		my $d;
		
		($err,$z) = ncpfs::ncp_scan_bindery_object($conn, $m, $type, $name);
		last if ($err);
		print dumpObjectInfo($z)."\n";
		$m = $z->object_id();
	}
	print "---------+------+----+----+----+--------------------\n";
	print 'Error='.ncpfs::strnwerror($err)."\n\n";
}

sub getTime($) {
	my $conn = shift;
	
	print "Get File Server Time\n";
	my ($err,$tm) = ncpfs::ncp_get_file_server_time($conn);
	if ($err) {
		print 'Error='.ncpfs::strnwerror($err)."\n\n";
		return undef;
	} else {
		print 'Time='.$tm.' == '.scalar gmtime($tm)."\n\n";
		return $tm;
	}
}

sub setTime($$) {
	my $conn = shift;
	my $tm = shift;
	
	print "Set File Server Time: ";
	my $err = ncpfs::ncp_set_file_server_time($conn,$tm);
	errorall($err);
}

sub getConnList($$$) {
	my ($conn, $type, $name) = @_;

	printf "Connection list	for %s(%04X)\n", $name, $type;
	my ($err, $conns) = ncpfs::ncp_get_connlist($conn, $type, $name);
	if ($err) {
		print 'Error='.ncpfs::strnwerror($err)."\n\n";
		return undef;
	}
	print "Connections: ".scalar @$conns." at $conns; list: ";
	print join(',', @$conns);
	print "\n\n";
}

sub getStationInfo($$$) {
	my ($conn, $first, $last) = @_;
	
	print "Connection listing\n";
	print "Con | ObjectID | Type | Fl | Sc | Pr | Name                                             | Login Time\n";
	print "----+----------+------+----+----+----+--------------------------------------------------+--------------------------\n";
	while ($first <= $last) {
		my ($err, $info, $tm) = ncpfs::ncp_get_stations_logged_info($conn, $first);
		if ($err) {
			if ($err != $ncpfs::NWE_CONN_NUM_INVALID) {
				printf "%3u | Error: %s\n", $first, ncpfs::strnwerror($err);
			}
		} else {
			printf "%3u | %s | %s\n", $first, dumpObjectInfo($info), scalar gmtime($tm);
		}
		$first++;
	}
	print "----+----------+------+----+----+----+--------------------------------------------------+--------------------------\n";
}

sub getInternetAddress($$$) {
	my ($conn, $first, $last) = @_;
	
	print "Connection addresses\n";
	while ($first <= $last) {
		my ($err, $addr, $type) = ncpfs::ncp_get_internet_address($conn, $first);
		if ($err) {
			printf "%3u | Error=%s\n", $first, ncpfs::strnwerror($err) unless $err == $ncpfs::NWE_SERVER_FAILURE;
		} else {
			my ($port,$ad) = eval { my ($a,$b) = unpack_sockaddr_in($addr); $b = inet_ntoa($b); return ($a,$b); };
			if ($@) {
				printf "%3u | %3u | %s\n", $first, $type, $@;
			} else {
				printf "%3u | %3u | %s:%u\n", $first, $type, $ad, $port;
			}
		}
		$first++;
	}
}

sub sendMessage1($$$) {
	my ($conn, $arr, $msg) = @_;
	
	print "Sending message (old): ";
	my $err = ncpfs::ncp_send_broadcast($conn, $arr, $msg);
	errorall($err);
}

sub sendMessage2($$$) {
	my ($conn, $arr, $msg) = @_;
	
	print "Sending message: ";
	my $err = ncpfs::ncp_send_broadcast2($conn, $arr, $msg);
	errorall($err);

	my $out;
	($err, $out) = ncpfs::ncp_get_broadcast_message($conn);
	if ($err) {
		print "Cannot fetch broadcast message: ".ncpfs::strnwerror($err)."\n";
	} else {
		print "Broadcast message: >$out<\n";
	}
}

sub getEncryptionKey($) {
	my $conn = shift;
	
	print "Get encryption key\n";
	my ($err, $key) = ncpfs::ncp_get_encryption_key($conn);
	if ($err) {
		print "Error=".ncpfs::strnwerror($err)."\n";
		return undef;
	}
	print "OK ";
	foreach $conn (split(//,$key)) {
		printf " %02X", ord $conn;
	}
	print "\n";
	return $key;
}

sub getBinderyObjectId($$$) {
	my ($conn, $type, $name) = @_;

	print "ObjectID | Type | Fl | Sc | Pr | Name\n";
	print "---------+------+----+----+----+--------------------------------------------------\n";
	my ($err, $info) = ncpfs::ncp_get_bindery_object_id($conn, $type, $name);
	if ($err) {
		printf "Error: %s\n", ncpfs::strnwerror($err);
	} else {
		printf "%s\n", dumpObjectInfo($info);
	}
	print "---------+------+----+----+----+--------------------------------------------------\n";
}

sub getBinderyObjectName($$) {
	my ($conn, $id) = @_;

	print "ObjectID | Type | Fl | Sc | Pr | Name\n";
	print "---------+------+----+----+----+--------------------------------------------------\n";
	my ($err, $info) = ncpfs::ncp_get_bindery_object_name($conn, $id);
	if ($err) {
		printf "Error: %s\n", ncpfs::strnwerror($err);
	} else {
		printf "%s\n", dumpObjectInfo($info);
	}
	print "---------+------+----+----+----+--------------------------------------------------\n";
}

sub demoBindery($$$) {
	my ($conn, $type, $name) = @_;
	
	print "Creating $name... ";
	my $err = ncpfs::ncp_create_bindery_object($conn, $type, $name, 0, 0);
	if ($err) {
		printf "Error: %s\n", ncpfs::strnwerror($err);
	} else {
		print "OK\n";
		dumpBindery($conn, $type, $name);
	}
	print "Changing security of $name... ";
	$err = ncpfs::ncp_change_object_security($conn, $type, $name, 0x33);
	if ($err) {
		printf "Error: %s\n", ncpfs::strnwerror($err);
	} else {
		print "OK\n";
		dumpBindery($conn, $type, $name);
	}
	print "Create normal property... ";
	$err = ncpfs::ncp_create_property($conn, $type, $name, 'MY_NORMAL_PROP', 0, 0);
	if (errorinfo($err)) {
		print "OK\n";
		scanProperties($conn, $type, $name, 'MY_NORMAL_PROP');
	}
	print "Write property value... ";
	my $pp = ncpfs::nw_property->new();
	$pp->{'more_flag'} = 0;
	$pp->{'value'} = pack('a128', 'Hokus Pokus Property');
	$err = ncpfs::ncp_write_property_value($conn, $type, $name, 'MY_NORMAL_PROP', 1, $pp);
	if (errorinfo($err)) {
		print "OK\n";
		scanProperties($conn, $type, $name, 'MY_NORMAL_PROP');
	}
	print "Deleting property... ";
	$err = ncpfs::ncp_delete_property($conn, $type, $name, 'MY_NORMAL_PROP');
	if (errorinfo($err)) {
		print "OK\n";
		scanProperties($conn, $type, $name, 'MY_NORMAL_PROP');
	}
	print "Create set property... ";
	$err = ncpfs::ncp_create_property($conn, $type, $name, 'MY_SET_PROP', 2, 0);
	if (errorinfo($err)) {
		print "OK\n";
		scanProperties($conn, $type, $name, 'MY_SET_PROP');
	}
	print "Change property security... ";
	$err = ncpfs::ncp_change_property_security($conn, $type, $name, 'MY_SET_PROP', 0x22);
	if (errorinfo($err)) {
		print "OK\n";
		scanProperties($conn, $type, $name, 'MY_SET_PROP');
	}
	print "Add SUPERVISOR to set... ";
	$err = ncpfs::ncp_add_object_to_set($conn, $type, $name, 'MY_SET_PROP', 1, 'SUPERVISOR');
	if (errorinfo($err)) {
		print "OK\n";
		scanProperties($conn, $type, $name, 'MY_SET_PROP');
	}

	printf 'NWIsObjectInSet(conn, "%s", %u, "%s", "%s", %u) = ', $name, $type, 'MY_SET_PROP', $name, $type;
	$err = ncpfs::NWIsObjectInSet($conn, $name, $type, 'MY_SET_PROP', $name, $type);
	errorall($err);

	print "Add myself to set... ";
	$err = ncpfs::ncp_add_object_to_set($conn, $type, $name, 'MY_SET_PROP', $type, $name);
	if (errorinfo($err)) {
		print "OK\n";
		scanProperties($conn, $type, $name, 'MY_SET_PROP');
	}

	printf 'NWIsObjectInSet(conn, "%s", %u, "%s", "%s", %u) = ', $name, $type, 'MY_SET_PROP', $name, $type;
	$err = ncpfs::NWIsObjectInSet($conn, $name, $type, 'MY_SET_PROP', $name, $type);
	errorall($err);

	print "Delete supervisor from set... ";
	$err = ncpfs::ncp_delete_object_from_set($conn, $type, $name, 'MY_SET_PROP', 1, 'SUPERVISOR');
	if (errorinfo($err)) {
		print "OK\n";
		scanProperties($conn, $type, $name, 'MY_SET_PROP');
	}
	
	print "Deleting property... ";
	$err = ncpfs::ncp_delete_property($conn, $type, $name, 'MY_SET_PROP');
	if (errorinfo($err)) {
		print "OK\n";
		scanProperties($conn, $type, $name, 'MY_SET_PROP');
	}
	print "Deleting $name... ";
	$err = ncpfs::ncp_delete_bindery_object($conn, $type, $name);
	if (errorinfo($err)) {
		print "OK\n";
		dumpBindery($conn, $type, $name);
	}
}

sub readProperty($$$$) {
	my ($conn, $type, $name, $prop) = @_;
	my $seg;

	print "Reading property $prop of $name...\n";
	for ($seg = 1; $seg < 256; $seg++) {
		my ($err, $info) = ncpfs::ncp_read_property_value($conn, $type, $name, $seg, $prop);
		print "Block #$seg:\n";
		if ($err) {
			printf "Error: %s\n", ncpfs::strnwerror($err);
			last;
		} else {
			my $k;
		
			foreach $k (keys %$info) {
				if ($k eq 'value') {
					print "Value => ".unpack("H128", $info->{$k})."\n";
				} else {
					printf "%s => %s\n", $k, $info->{$k};
				}
			}
			if ($info->{'more_flag'} != 255) {
				last;
			}
		}
	}
	print "----\n";
}

sub scanProperties($$$$) {
	my ($conn, $type, $name, $srch) = @_;
	my $id = -1;
	
	while (1) {
		my ($err, $info) = ncpfs::ncp_scan_property($conn, $type, $name, $id, $srch);
		if ($err) {
			if ($err != $ncpfs::NWE_NCP_NOT_SUPPORTED) {
				printf "Error: %s\n", ncpfs::strnwerror($err);
			} elsif ($id == -1) {
				printf "Error: No such property\n";
			}
			last;
		}
		my $k;
		foreach $k (keys %$info) {
			if ($k eq 'search_instance') {
				printf "%s => %08X\n", $k, $info->{$k};
			} else {
				printf "%s => %s\n", $k, $info->{$k};
			}
		}
		readProperty($conn, $type, $name, $info->{'property_name'});
#		We must not exit when we found that more_properties_flag signals no more properties.
#		If we do so, iterator handle on the server stays allocated, and so you get
#		search_instance == 1 on second run, 2 on third, 3 on fourth... out of server memory
#		on dozen attempt...
#		last if ($info->{'more_properties_flag'} != 255);
		$id = $info->{'search_instance'};
	}
}

sub loginEncrypted($$$$) {
	my ($conn, $type, $name, $pwd) = @_;
	
	print "Encrypted login... ";
	my ($err, $key) = ncpfs::ncp_get_encryption_key($conn);
	if ($err) {
		printf "Cannot get encryption key: %s\n".ncpfs::strnwerror($err);
	} else {
		my $info;
		
		($err,$info) = ncpfs::ncp_get_bindery_object_id($conn, $type, $name);
		if ($err) {
			printf "Get Object ID failed: %s\n", ncpfs::strnwerror($err);
		} else {
			$err = ncpfs::ncp_login_encrypted($conn, $info, $key, $pwd);
			if ($err) {
				printf "Login failed: %s\n", ncpfs::strnwerror($err);
			} else {
				print "OK\n";
				getConnList($conn, $type, $name);
			}
		}
	}
}

sub loginUnencrypted($$$$) {
	my ($conn, $type, $name, $pwd) = @_;
	
	print "Unencrypted login... ";
	my $err = ncpfs::ncp_login_unencrypted($conn, $type, $name, $pwd);
	if (errorinfo($err)) {
		print "OK\n";
		getConnList($conn, $type, $name);
	}
}

sub loginUser($$$) {
	my ($conn, $name, $pwd) = @_;
	
	print "Login User... ";
	my $err = ncpfs::ncp_login_user($conn, $name, $pwd);
	if (errorinfo($err)) {
		print "OK\n";
		getConnList($conn, 1, $name);
	}
}

sub changePasswordEncrypted($$$$$) {
	my ($conn, $type, $name, $pwd, $newpwd) = @_;
	
	print "Encrypted Change Password... ";
	my ($err, $key) = ncpfs::ncp_get_encryption_key($conn);
	if ($err) {
		printf "Cannot get encryption key: %s\n".ncpfs::strnwerror($err);
	} else {
		my $info;
		
		($err,$info) = ncpfs::ncp_get_bindery_object_id($conn, $type, $name);
		if ($err) {
			printf "Get Object ID failed: %s\n", ncpfs::strnwerror($err);
		} else {
			$err = ncpfs::ncp_change_login_passwd($conn, $info, $key, $pwd, $newpwd);
			if ($err) {
				printf "Change failed: %s\n", ncpfs::strnwerror($err);
			} else {
				print "OK\n";
			}
		}
	}
}

sub getVolInfos($) {
	my ($conn) = @_;
	my $voln;
	
	for ($voln = 0; $voln < 256; $voln++) {
		my ($err, $info) = ncpfs::ncp_get_volume_info_with_number($conn, $voln);
		if ($err) {
			printf "Volume %u does not exist: %s\n", $voln, ncpfs::strnwerror($err);
		} else {
			if ($info->{'volume_name'} ne '') {
				printf "Volume %u:\n", $voln;
				foreach $err (keys %$info) {
					printf "%24s => %s\n", $err, $info->{$err};
				}
			}
		}
	}
}

sub getVolumeNumber($$) {
	my ($conn, $name) = @_;
	
	print "Volume $name... ";
	my ($err, $voln) = ncpfs::ncp_get_volume_number($conn, $name);
	if (errorinfo($err)) {
		print "ok, no=$voln\n";
	}
}

sub dodir($$$$) {
	my ($conn,$dirh,$path,$path2) = @_;
	
	print "Doing directory listing of $dirh / $path... ";
	my ($err, $dir) = ncpfs::ncp_file_search_init($conn, $dirh, $path);
	if ($err) {
		printf "error: %s\n", ncpfs::strnwerror($err);
	} else {
		print "OK\n";
		while (1) {
			my $z;
			my $q;
			
			foreach $z (keys %$dir) {
				printf "%20s => %s\n", $z, $dir->{$z};
			}
#			Dump($dir);
			($err, $q) = ncpfs::ncp_file_search_continue($conn, $dir, 0x4E, $path2);
#			Dump($dir);
			if ($err) {
				printf "error: %s\n", ncpfs::strnwerror($err);
				last;
			}
			foreach $z (keys %$q) {
				my $v = $q->{$z};
				if ($z eq 'file_id') {
					$v = unpack('H*', $v);
				}
				printf "%20s => %s\n", $z, $v;
			}
		}
	}
}

sub getfinfo($$$$) {
	my ($conn,$dirh,$path,$name) = @_;
	
	print "Retrievning file info on $dirh / $path/$name... ";
	my ($err, $q) = ncpfs::ncp_get_finfo($conn, $dirh, $path, $name);
	if ($err) {
		printf "error: %s\n", ncpfs::strnwerror($err);
	} else {
		print "OK\n";
		my $z;

		foreach $z (keys %$q) {
			my $v = $q->{$z};
			if ($z eq 'file_id') {
				$v = unpack('H*', $v);
			}
			printf "%20s => %s\n", $z, $v;
		}
	}
}

sub optrailer($$) {
	my ($conn, $q) = @_;
	my $err;

	print "OK\n";
	my $z;

	foreach $z (keys %$q) {
		my $v = $q->{$z};
		if ($z eq 'file_id') {
			$v = unpack('H*', $v);
		}
		printf "%20s => %s\n", $z, $v;
	}
	print "Closing... ";
#	$z = $q->{'file_id'};
#	Dump($z);
	$err = ncpfs::ncp_close_file($conn, $q->{'file_id'});
	errorall($err);
}

sub openclose($$$$$) {
	my ($conn, $dirh, $path, $attr, $accm) = @_;
	
	print "Opening file $dirh / $path... ";
	my ($err, $q) = ncpfs::ncp_open_file($conn, $dirh, $path, $attr, $accm);
	if ($err) {
		printf "error: %s\n", ncpfs::strnwerror($err);
	} else {
		my $ret;

		($err, $ret) = ncpfs::ncp_read($conn, $q->{'file_id'}, 1, 1000);
		if ($err < 0) {
			print "Read failed: ".ncpfs::strnwerror($err)."... ";
		} else {
			print "Read ok: $err bytes, \"$ret\"... ";
		}
		($err, $ret) = ncpfs::ncp_copy_file($conn, $q->{'file_id'}, $q->{'file_id'}, 5, 891, 12);
		if ($err) {
			print "Copy failed: ".ncpfs::strnwerror($err)."... ";
		} else {
			print "Copy ok: $ret bytes copied... ";
		}
		($err, $ret) = ncpfs::ncp_read($conn, $q->{'file_id'}, 891, 5);
		if ($err < 0) {
			print "Read failed: ".ncpfs::strnwerror($err)."... ";
		} else {
			print "Read ok: $err bytes, \"$ret\"... ";
		}
		optrailer($conn, $q);
	}
	
}

sub createnewfile($$$$) {
	my ($conn, $dirh, $path, $attr) = @_;

	print "Creating new file $dirh / $path... ";
	my ($err, $q) = ncpfs::ncp_create_newfile($conn, $dirh, $path, $attr);
	if ($err) {
		printf "error: %s\n", ncpfs::strnwerror($err);
	} else {
		$err = ncpfs::ncp_write($conn, $q->{'file_id'}, 0, 'Test1' x 100);
		if ($err < 0) {
			print "write failed: ".ncpfs::strnwerror($err)."... ";
		} else {
			print "written $err bytes... ";
		}
		optrailer($conn, $q);
	}
}

sub createfile($$$$) {
	my ($conn, $dirh, $path, $attr) = @_;

	print "Creating file $dirh / $path... ";
	my ($err, $q) = ncpfs::ncp_create_file($conn, $dirh, $path, $attr);
	if (errorinfo($err)) {
		optrailer($conn, $q);
	}
}

sub erasefile($$$$) {
	my ($conn, $dirh, $path, $attr) = @_;

	print "Erasing file $dirh / $path... ";
	my $err = ncpfs::ncp_erase_file($conn, $dirh, $path, $attr);
	errorall($err);
}

sub renamefile($$$$$$) {
	my ($conn, $dirh, $path, $attr, $dirhn, $pathn) = @_;

	print "Renaming file $dirh / $path => $dirhn / $pathn... ";
	my $err = ncpfs::ncp_rename_file($conn, $dirh, $path, $attr, $dirhn, $pathn);
	errorall($err);
}

sub dumptstee($$$) {
	my ($conn, $id, $vol) = @_;
	my $seq = -1;

	print "SeqNo | GrRights | Path\n";
	print "------+----------+----------------------------\n";
	while (1) {
		my ($err,$path,$rights) = ncpfs::ncp_get_trustee($conn, $id, $vol, \$seq);
		
		if ($err) {
			printf "Error: %s\n", ncpfs::strnwerror($err);
			last;
		}
		if ($seq == 0) {
			last;
		}
		printf "%5u | %08b | %s\n", $seq, $rights, $path;
	}
	print "------+----------+----------------------------\n";
}

sub demoFile($$$) {
	my ($conn, $dirh, $path) = @_;
	
	print "Creating directory $dirh / $path... ";
	my $err = ncpfs::ncp_create_directory($conn, $dirh, $path, 0xFF);
	errorall($err);
	print "Adding trustee... ";
	$err = ncpfs::ncp_add_trustee($conn, $dirh, $path, 0x00000001, 0xFF);
	errorall($err);
	my $pathf = $path.'/TESTFILE.TST';
	createnewfile($conn, $dirh, $pathf, 0);
	openclose($conn, $dirh, $pathf, 0x4E, 3);
	createfile($conn, $dirh, $pathf, 0);
	print "Renaming directory... ";
	$err = ncpfs::ncp_rename_directory($conn, $dirh, $path, 'RENAMED.TST');
	if (errorinfo($err)) {
		print "OK\n";
		$path = 'SYS:RENAMED.TST';
		$pathf = $path.'/TESTFILE.TST';
	}
	renamefile($conn, $dirh, $pathf, 0, $dirh, 'SYS:NCPFSPRL.NEW');
	erasefile($conn, $dirh, 'SYS:NCPFSPRL.NEW', 0);
	dumptstee($conn, 0x00000001, 0);
	print "Removing trustee... ";
	$err = ncpfs::ncp_delete_trustee($conn, $dirh, $path, 0x00000001);
	errorall($err);
	print "Removing directory $dirh / $path... ";
	$err = ncpfs::ncp_delete_directory($conn, $dirh, $path);
	errorall($err);
}

sub dump_nw_info_struct($) {
	my ($info) = @_;
	my $key;
	
	foreach $key (keys %$info) {
		printf "%30s => %s\n", $key, $info->{$key};
	}
}

sub dump_nw_file_info($) {
	my ($info) = @_;
	my $key;
	
	foreach $key (keys %$info) {
		my $val = $info->{$key};
		
		if ($key eq 'file_handle') {
			$val = unpack('H*', $val);
		}
		printf "%30s => %s\n", $key, $val;
	}
	dump_nw_info_struct($info->{'i'});
}

sub file_subdir_info($) {
	my ($conn) = @_;
	
	my ($err, $info) = ncpfs::ncp_obtain_file_or_subdir_info($conn, 0, 0, $ncpfs::SA_FILES_ALL, 0x00000FFF, 0, 0, undef);
	print "Obtain info: ".ncpfs::strnwerror($err)."\n";
	if (!$err) {
		my $z;
	
		dump_nw_info_struct($info);	
		($err, $z) = ncpfs::ncp_get_eff_directory_rights($conn, 0, 0, $ncpfs::SA_FILES_ALL, $info->{'volNumber'},
			$info->{'DosDirNum'}, 'LOGIN');
		if ($err) {
			printf "Cannot retrieve effective rights: %s\n", ncpfs::strnwerror($err);
		} else {
			printf "Effective rights: 0x%04X\n", $z;
		}
		($err, $info) = ncpfs::ncp_do_lookup($conn, $info, 'LOGIN');
		if ($err) {
			printf "Cannot find SYS:LOGIN: %s\n", ncpfs::strnwerror($err);
		} else {
		        my $i2;
			
			print "SYS:LOGIN:\n";
			dump_nw_info_struct($info);
			my $seq;
			
			print "Searching everything in SYS:LOGIN\n";
			($err, $seq) = ncpfs::ncp_initialize_search($conn, $info, 0);
			if ($err) {
				print "Initialize search failed: ".ncpfs::strnwerror($err)."\n";
			} else {
				while (1) {
					print "Search seq:\n";
					foreach $z (keys %$seq) {
						my $v = $seq->{$z};
						if ($z eq 's') {
							$v = sprintf "%02X:%08X:%08X", $v->{'volNumber'}, $v->{'dirBase'}, $v->{'sequence'};
						}
						printf "%20s => %s\n", $z, $v;
					}
					($err, $i2) = ncpfs::ncp_search_for_file_or_subdir($conn, $seq);
					if ($err) {
						print "Search failed: ".ncpfs::strnwerror($err)."\n";
						last;
					}
					print "Result:\n";
					dump_nw_info_struct($i2);
				}
			}
			print "Searching files in SYS:LOGIN/NLS/ENGLISH, requesting filename only:\n";
			($err, $seq) = ncpfs::ncp_initialize_search2($conn, $info, 0, scalar ncpfs::ncp_path_to_NW_format('NLS/ENGLISH'));
			if ($err) {
				print "Initialize search failed: ".ncpfs::strnwerror($err)."\n";
			} else {
				while (1) {
					print "Search seq:\n";
					foreach $z (keys %$seq) {
						my $v = $seq->{$z};
						if ($z eq 's') {
							$v = sprintf "%02X:%08X:%08X", $v->{'volNumber'}, $v->{'dirBase'}, $v->{'sequence'};
						}
						printf "%20s => %s\n", $z, $v;
					}
					($err, $i2) = ncpfs::ncp_search_for_file_or_subdir2($conn, 6, 1, $seq);
					if ($err) {
						print "Search failed: ".ncpfs::strnwerror($err)."\n";
						last;
					}
					print "Result:\n";
					dump_nw_info_struct($i2);
				}
			}
			($err, $i2) = ncpfs::ncp_do_lookup2($conn, 0, $info, 'LOGIN.EXE', 4);
			if ($err) {
				printf "Cannot find SYS:LOGIN/LOGIN.EXE: %s\n", ncpfs::strnwerror($err);
			} else {
				print "SYS:LOGIN/LOGIN.EXE:\n";
				dump_nw_info_struct($i2);
			}
			
			($err, $i2) = ncpfs::ncp_open_create_file_or_subdir($conn, $info, 'NCPFSTMP.TMP',
				$ncpfs::OC_MODE_OPEN | $ncpfs::OC_MODE_CREATE, 0x06, 
				$ncpfs::AR_READ_ONLY | $ncpfs::AR_WRITE_ONLY);
			if ($err) {
				printf "Cannot create SYS:LOGIN/NCPFSTMP.TMP: %s\n", ncpfs::strnwerror($err);
			} else {
				my $q = new ncpfs::nw_modify_dos_info;
			
				print "$i2\n";
				dump_nw_file_info($i2);

				$q->{'lastAccessDate'} = 12345;
				$err = ncpfs::ncp_modify_file_or_subdir_dos_info($conn, $i2->{'i'}, $ncpfs::DM_LAST_ACCESS_DATE, $q);
				if ($err) {
					printf "Modify failed: %s\n", ncpfs::strnwerror($err);
				} else {
					my $i3;
					
					print "Modify OK\n";
					($err, $i3) = ncpfs::ncp_do_lookup2($conn, 0, $i2->{'i'}, undef, 0);
					if ($err) {
						printf "Cannot find updated info: %s\n", ncpfs::strnwerror($err);
					} else {
						print "Updated info:\n";
						dump_nw_info_struct($i3);
					}
				}
				$err = ncpfs::ncp_close_file($conn, $i2->{'file_handle'});
				if ($err) {
					print "Close failed: ".ncpfs::strnwerror($err)."\n";
				} else {
					print "Close OK\n";
				}
				my $tlist;
				my $tl2;
		
				$tlist = new ncpfs::ncp_trustee_struct;
				$tlist->object_id(0x00000001);
				$tlist->rights(0x01FB);
				$tl2 = new ncpfs::ncp_trustee_struct;
				$tl2->object_id(0x010000FF);
				$tl2->rights(0x001);
				print "Adding trustees ... ";
				$err = ncpfs::ncp_add_trustee_set($conn, $i2->{'i'}->{'volNumber'}, $i2->{'i'}->{'DosDirNum'}, -1, [ $tlist, $tl2] );
				if ($err) {
					print "failed: ".ncpfs::strnwerror($err)."\n";
				} else {
					print "OK\n";
				}
				$err = ncpfs::ncp_ren_or_mov_file_or_subdir($conn, $info, 'NCPFSTMP.TMP', $info, 'NEWNAMEN.TMP');
				if ($err) {
					print "Rename failed: ".ncpfs::strnwerror($err)."\n";
					$err = ncpfs::ncp_del_file_or_subdir($conn, $i2->{'i'}, undef);
				} else {
					print "Rename OK\n";
					$err = ncpfs::ncp_del_file_or_subdir($conn, $info, 'NEWNAMEN.TMP');
				}
				if ($err) {
					print "Delete failed: ".ncpfs::strnwerror($err)."\n";
				} else {
					print "Delete OK\n";
				}
				my $salvseq = new ncpfs::ncp_deleted_file;
				my $dopurge = 1;
				while (1) {
					my $pti;

					foreach $pti (keys %$salvseq) {
						my $v = $salvseq->{$pti};
						printf "%20s => %s\n", $pti, $v;
					}
					($err,$pti) = ncpfs::ncp_ns_scan_salvageable_file($conn, 0, 0xFF, 0, 0, scalar ncpfs::ncp_path_to_NW_format('SYS'), $salvseq);
					if ($err) {
						print "Cannot scan for salvageable file: ".ncpfs::strnwerror($err)."\n";
						last;
					}
					print "File: $pti\n";
					if ($dopurge) {
						print "Purging... ";
						$err = ncpfs::ncp_ns_purge_file($conn, $salvseq);
						if ($err) {
							print "failed: ".ncpfs::strnwerror($err)."\n";
						} else {
							print "OK\n";
							$dopurge = 0;
						}
					}
				}
			}
		}
	}
	($err, $info) = ncpfs::ncp_obtain_file_or_subdir_info2($conn, 0, 4, $ncpfs::SA_FILES_ALL, 0x00000FFF, 0xFF, 0, 0, scalar ncpfs::ncp_path_to_NW_format('SYS/LOGIN'));
	print "Obtain info of SYS:LOGIN: ".ncpfs::strnwerror($err)."\n";
	if (!$err) {
		my $z;
	
		dump_nw_info_struct($info);
		print "Mapping it back to path... ";
		($err,$info) = ncpfs::ncp_ns_get_full_name($conn, 0, 0, 1, $info->{'volNumber'}, $info->{'DosDirNum'}, undef);
		if ($err) {
			print "failed: ".ncpfs::strnwerror($err)."\n";
		} else {
			print "OK, $info\n";
		}
	}
}

sub dumpJobEntry($) {
	my ($j) = @_;
	my $z;

	foreach $z (keys %$j) {
		my $v = $j->{$z};

		if ($z eq 'ClientRecordArea' or $z eq 'JobEntryTime' or $z eq 'TargetExecTime') {
			$v = unpack('H*', $v);
		}
		printf "%30s => %s\n", $z, $v;
	}
}

sub dumpJob($) {
	my ($job) = @_;
	my $z;

	print "Job info:\n";
	foreach $z (keys %$job) {
		my $v = $job->{$z};
		if ($z eq 'j') {
			next;
		}
		if ($z eq 'file_handle') {
			$v = unpack('H*', $v);
		}
		printf "%30s => %s\n", $z, $v;
	}
	dumpJobEntry($job->{'j'});
}

sub newQJob($$) {
	my ($conn, $qid) = @_;

	my $job = new ncpfs::queue_job;

	my $j = $job->{'j'};
	my $z;

#	Dump($j,100);
	$j->{'TargetServerID'} = 0xFFFFFFFF;
	$j->{'JobType'} = 0;
	$j->{'JobTextDescription'} = 'Hokus Pokus';
	$j->{'JobFileName'} = 'Hejhula';

	dumpJob($job);		
	
	my $err = ncpfs::ncp_create_queue_job_and_file($conn, $qid, $job);
	if ($err) {
		print "Create job failed: ".ncpfs::strnwerror($err)."\n";
	} else {
		dumpJob($job);
		
		print "Starting job... ";
		$err = ncpfs::ncp_close_file_and_start_job($conn, $qid, $job);
		if (errorinfo($err)) {
			print "OK\n";
			return $job->{'j'}->{'JobNumber'};
		}
	}
	return undef;
}

sub serviceQJob($$) {
	my ($conn, $qid) = @_;

	print "Servicing job... ";
	my ($err,$job) = ncpfs::ncp_service_queue_job($conn, $qid, -1);
	if ($err) {
		print "failed: ".ncpfs::strnwerror($err)."\n";
		return undef;
	} else {
		my $j2;

		print "OK\n";
			
		dumpJob($job);			
		($err, $j2) = ncpfs::ncp_get_queue_job_info($conn, $qid, $job->{'j'}->{'JobNumber'});
		if ($err) {
			print "Get info failed.\n";
		} else {
			dumpJobEntry($j2);
		}
		return $job->{'j'}->{'JobNumber'};
	}
}

sub qLength($$) {
	my ($conn, $qid) = @_;

	print "Queue length... ";
	my ($err, $cnt) = ncpfs::ncp_get_queue_length($conn, $qid);
	if (errorinfo($err)) {
		print "OK, $cnt jobs.\n";
	}
	my $jl;
	print "Queue jobs... ";
	($err, $cnt, $jl) = ncpfs::ncp_get_queue_job_ids($conn, $qid, 0);
	if (errorinfo($err)) {
		my $z;
		printf "OK, %u jobs, %u returned.\n", $cnt, scalar @$jl;
		foreach $z (@$jl) {
			print "... $z\n";
		}
	}
}
	
	
sub queueTest($) {
	my ($conn) = @_;
	my $err;
	my $info;
	my $qid;
	
	$err = ncpfs::ncp_create_bindery_object($conn, 3, 'NCPFSTESTQUEUE', 0, 0);
	if ($err) {
		print "Create queue failed: ".ncpfs::strnwerror($err)."\n";
	}
	($err, $info) = ncpfs::ncp_get_bindery_object_id($conn, 3, 'NCPFSTESTQUEUE');
	if ($err) {
		print "Test queue not found: ".ncpfs::strnwerror($err)."\n";
		return undef;
	}
	$qid = $info->{'object_id'};
	printf "Test queue has id 0x%08X\n", $qid;
	$err = ncpfs::ncp_add_object_to_set($conn, 3, 'NCPFSTESTQUEUE', 'Q_SERVERS', $test_objecttype, $test_objectname);
	if ($err) {
		printf "Adding $test_objectname to Q_SERVERS: Error: %s\n", ncpfs::strnwerror($err);
	}
	$err = ncpfs::ncp_add_object_to_set($conn, 3, 'NCPFSTESTQUEUE', 'Q_OPERATORS', $test_objecttype, $test_objectname);
	if ($err) {
		printf "Adding $test_objectname to Q_OPERATORS: Error: %s\n", ncpfs::strnwerror($err);
	}
	my $pp = ncpfs::nw_property->new();
	$pp->{'more_flag'} = 0;
	$pp->{'value'} = pack('a128', 'SYS:LOGIN/NLS/ENGLISH');
	$err = ncpfs::ncp_write_property_value($conn, 3, 'NCPFSTESTQUEUE', 'Q_DIRECTORY', 1, $pp);
	if ($err) {
		printf "Setting Q_DIRECTORY: Error: %s\n", ncpfs::strnwerror($err);
	}
	$err = ncpfs::ncp_create_property($conn, 3, 'NCPFSTESTQUEUE', 'Q_USERS', 2, 0);
	if ($err) {
		printf "Creating Q_USERS: Error: %s\n", ncpfs::strnwerror($err);
	}
	$err = ncpfs::ncp_add_object_to_set($conn, 3, 'NCPFSTESTQUEUE', 'Q_USERS', $test_objecttype, $test_objectname);
	if ($err) {
		printf "Adding $test_objectname to Q_USERS: Error: %s\n", ncpfs::strnwerror($err);
	}

	my $job;
	my $j;
	my $z;

	$j = newQJob($conn, $qid);
	if (defined($j)) {
		print "Removing job... ";
		$err = ncpfs::NWRemoveJobFromQueue2($conn, $qid, $j);
		errorall($err);
	}
	
	newQJob($conn, $qid);
	
	print "Attaching to queue... ";
	$err = ncpfs::ncp_attach_to_queue($conn, $qid);
	if (errorinfo($err)) {
		print "OK\n";

		qLength($conn, $qid);

		my $n = serviceQJob($conn, $qid);			
		if (defined($n)) {
			print "Aborting servicing job... ";
			$err = ncpfs::ncp_abort_servicing_job($conn, $qid, $n);
			errorall($err);
		}
		
		qLength($conn, $qid);

		newQJob($conn, $qid);
		
		$n = serviceQJob($conn, $qid);
		if (defined($n)) {
			print "Finish servicing job... ";
			$err = ncpfs::ncp_finish_servicing_job($conn, $qid, $n, 5);
			errorall($err);
		}
		print "Detaching from queue... ";
		$err = ncpfs::ncp_detach_from_queue($conn, $qid);
		errorall($err);
	}
	
#	$err = ncpfs::ncp_delete_bindery_object($conn, 3, 'NCPFSTESTQUEUE');
	$err = 0;
	if ($err) {
		print "Queue removing failed: ".ncpfs::strnwerror($err)."\n";
	} else {
		print "Queue removed\n";
	}
}

sub file2_demo($) {
	my ($conn) = @_;
	
	my $dir = new ncpfs::nw_info_struct;
	$dir->volNumber(0);
	$dir->DosDirNum(0);
	
	print "Allocating short dir handle for SYS: ... ";
	my ($err, $hnd) = ncpfs::ncp_alloc_short_dir_handle($conn, $dir, 1);
	if ($err) {
		print "failed: ".ncpfs::strnwerror($err)."\n";
	} else {
		print "OK, $hnd\n";
		
		print "Releasing short dir handle for SYS: ... ";
		$err = ncpfs::ncp_dealloc_dir_handle($conn, $hnd);
		errorall($err);
	}

	print "Allocating short dir handle for SYS: ... ";
	($err, $hnd) = ncpfs::ncp_alloc_short_dir_handle2($conn, 0, $dir, 1);
	if (errorinfo($err)) {
		print "OK, $hnd\n";

		print "Releasing short dir handle for SYS: ... ";
		$err = ncpfs::ncp_dealloc_dir_handle($conn, $hnd);
		errorall($err);
	}
}

sub connInfo($) {
	my ($conn) = @_;
	
	print "Conn type:   ".ncpfs::ncp_get_conn_type($conn)."\n";
	print "Conn number: ".ncpfs::ncp_get_conn_number($conn)."\n";
	my ($err, $ttl) = ncpfs::ncp_get_dentry_ttl($conn);
	print "Directory Cache TTL: ";
	if ($err) {
		print "??? (".ncpfs::strnwerror($err).")\n";
	} else {
		printf "%u ms\n", $ttl * 10;
	}
}

sub vollist($$) {
	my ($conn, $nspace) = @_;
	
	print "Looking for volumes with ".ncpfs::ncp_namespace_to_str($nspace)." namespace... ";
	my ($err, $h) = ncpfs::ncp_volume_list_init($conn, $nspace, 1);
	if (errorinfo($err)) {
		print "ok, with handle $h\n";
		while (1) {
			my ($id, $nm);

			($err, $id, $nm) = ncpfs::ncp_volume_list_next($h);
			if ($err) {
				print "No next entry: ".ncpfs::strnwerror($err)."\n";
				last;
			}
			printf "[%u] %s\n", $id, $nm;
		}
		$err = ncpfs::ncp_volume_list_end($h);
		if ($err) {
			print "Could not close iteration handle: ".ncpfs::strnwerror($err)."\n";
		}
	}
}

sub cnvtValue($) {
	my ($val) = @_;

##	$val =~ s/([\000-\037\200-\237\\])/sprintf "\\%02X", ord($1)/eg;
	$val =~ s/([\000-\037\177-\377\\])/sprintf "\\%02X", ord($1)/eg;
	return $val;
}

sub dumpInside($$) {
	my ($spc, $val) = @_;
	if (UNIVERSAL::isa($val, 'HASH')) {
		&dumpInsideHash($spc, $val);
	} elsif (UNIVERSAL::isa($val, 'ARRAY')) {
		&dumpInsideArray($spc, $val);
	}
}

sub dumpInsideArray($$) {
	my ($spc, $a) = @_;
	my ($val);
	my ($l) = (0);

#	Dump($a);
	foreach $val (@$a) {
		printf "%s[%u] => %s\n", $spc, $l, cnvtValue($val);
		dumpInside($spc.'  ', $val);
		$l++;
	}
#	pop(@$a);	READONLY
#	$a->[4] = 123;
#	push(@$a, 21, 22, 23);
#	shift @$a;	READONLY
#	unshift @$a, 11, 22;
#	undef @$a;	READONLY
#	$#$a = 111;
#	@$a = ();	READONLY
#	splice @$a, 5, 2, 11, 12;
#	delete $a->[4];	READONLY
#	exists $a->[4];
#	untie @$a, 333;
}

sub dumpInsideHash($$) {
	my ($spc, $h) = @_;
	my ($k);
	
	foreach $k (keys %$h) {
		my $val = $h->{$k};
		printf "%s%s => %s\n", $spc, $k, cnvtValue($val);
		dumpInside($spc.'  ', $val);
	}
}

sub newnsdemo($) {
	my ($conn) = @_;

	print "Obtaining info about SYS:LOGIN/NLS/ENGLISH... ";
	my ($err, $h) = ncpfs::ncp_ns_obtain_entry_info($conn, $ncpfs::NW_NS_DOS, $ncpfs::SA_ALL, 
		$ncpfs::NCP_DIRSTYLE_NOHANDLE, 0, 0, scalar ncpfs::ncp_path_to_NW_format('SYS:LOGIN/NLS/ENGLISH'),
		$ncpfs::NW_NS_OS2, 0x81FFFFFF, $ncpfs::NW_INFO_STRUCT2);
	if (errorinfo($err)) {
		print "ok, returned structure $h\n";
		dumpInsideHash('', $h);
#		$h->{'ReferenceID'} = 1233;
#		$h->{'LastAccess'}->{'Time'} = 1233;
#		$h->{'reserved2'} = 'ABCD1234' x 64;
#		dumpInsideHash('', $h);
		my ($fh, $oca, $occ);

		print "Creating TMPFILE.TMP... ";
		($err, $h, $oca, $occ, $fh) = ncpfs::ncp_ns_open_create_entry($conn, $ncpfs::NW_NS_OS2, $ncpfs::SA_ALL,
			$ncpfs::NCP_DIRSTYLE_DIRBASE, $h->{'Directory'}->{'volNumber'}, $h->{'Directory'}->{'dirEntNum'},
			scalar ncpfs::ncp_path_to_NW_format('TMPFILE.TMP'), -1, $ncpfs::OC_MODE_OPEN | $ncpfs::OC_MODE_CREATE, 
			0x06, $ncpfs::AR_READ_ONLY | $ncpfs::AR_WRITE_ONLY, 0x81FFFFFF,
			$ncpfs::NW_INFO_STRUCT2);
		if (errorinfo($err)) {
			print "ok, returned structure $h, action $oca, callback $occ\n";
			dumpInsideHash('', $h);
			print 'File handle: '.unpack('H*', $fh)."\n";

			print 'Writting... ';
			$err = ncpfs::ncp_write($conn, $fh, 0, 'Test1' x 100);
			if ($err < 0) {
				print "failed: ".ncpfs::strnwerror($err)."... ";
			} else {
				print "OK, $err bytes written... ";
			}
			
			print 'Closing file '.unpack('H*', $fh).'... ';
			$err = ncpfs::ncp_close_file($conn, $fh);
			errorall($err);
			print 'Refetching file info... ';
			my $nh;
			($err, $nh) = ncpfs::ncp_ns_obtain_entry_info($conn, $ncpfs::NW_NS_OS2, $ncpfs::SA_ALL,
				$ncpfs::NCP_DIRSTYLE_DIRBASE, $h->{'Directory'}->{'volNumber'}, $h->{'Directory'}->{'dirEntNum'},
				undef, $ncpfs::NW_NS_OS2, 0x81FFFFFF, $ncpfs::NW_INFO_STRUCT2);
			if (errorinfo($err)) {
				print "ok, returned structure $nh\n";
				dumpInsideHash('', $nh);
			}
			print 'Setting DOS info... ';
			my $ns = new ncpfs::ncp_dos_info;
			$ns->{'Attributes'} = 0x26;
			$ns->{'Modify'}->{'Date'} = 0x1111;
			$ns->{'Modify'}->{'Time'} = 0xE0E0;
			$ns->{'Modify'}->{'ID'} = 1;
			$ns->{'Rights'}->{'Grant'} = 0x0000;
			$ns->{'Rights'}->{'Revoke'} = 0xFFFF;
			dumpInsideHash('', $ns);
#			my $qq = $ns->{'Modify'};
#			print $qq;
#			Dump($qq);
			$err = ncpfs::ncp_ns_modify_entry_dos_info($conn, $ncpfs::NW_NS_OS2, $ncpfs::SA_ALL,
				$ncpfs::NCP_DIRSTYLE_DIRBASE, $h->{'Directory'}->{'volNumber'}, $h->{'Directory'}->{'dirEntNum'},
				undef, $ncpfs::DM_ATTRIBUTES | $ncpfs::DM_MODIFY_DATE | $ncpfs::DM_MODIFY_TIME | $ncpfs::DM_MODIFIER_ID |
				$ncpfs::DM_INHERITED_RIGHTS_MASK, $ns);
			if (errorinfo($err)) {
				print 'ok, refetching file info... ';
				($err, $nh) = ncpfs::ncp_ns_obtain_entry_info($conn, $ncpfs::NW_NS_OS2, $ncpfs::SA_ALL,
					$ncpfs::NCP_DIRSTYLE_DIRBASE, $h->{'Directory'}->{'volNumber'}, $h->{'Directory'}->{'dirEntNum'},
					undef, $ncpfs::NW_NS_OS2, 0x81FFFFFF, $ncpfs::NW_INFO_STRUCT2);
				if (errorinfo($err)) {
					print "ok, returned structure $nh\n";
					dumpInsideHash('', $nh);
				}
			}
		}
	}
}

sub dumpnsinfo($$$$) {
	my ($conn, $ns, $vol, $dirent) = @_;
	my ($err,$nspc, $volnm,$lay);
	
	$nspc = ncpfs::ncp_namespace_to_str($ns);
	($err, $volnm) = ncpfs::ncp_ns_get_full_name($conn, 0, 0, 1, $vol, $dirent, undef);
	if ($err) {
		$volnm = '<unknown>';
	}
	printf 'Obtaining %s namespace layout from %s... ', $nspc, $volnm;
	($err, $lay) = ncpfs::ncp_ns_obtain_namespace_info_format($conn, $vol, $ns, $ncpfs::NCP_NAMESPACE_FORMAT);
	if ($err) {
		print 'failed '.ncpfs::strnwerror($err)."\n";
		return undef;
	}
	print "ok\n";
	dumpInsideHash('',$lay);
	my ($i,$inf);
	for ($i = 0; $i < 32; $i++) {
		printf 'Obtaining %s namespace info %u from %s ... ', $nspc, $i, $volnm;
		($err, $inf) = ncpfs::ncp_ns_obtain_entry_namespace_info($conn, $ns, $vol, $dirent, $ns, 1 << $i, 1024);
		if (errorinfo($err)) {
#			Dump($inf);
			print unpack('H*', $inf)."\n";
		}
	}
	printf 'Obtaining %s namespace info from %s ... ', $nspc, $volnm;
	($err,$inf) = ncpfs::ncp_ns_obtain_entry_namespace_info($conn, $ns, $vol, $dirent, $ns, 0xFFFFFFFF, 1024);
	if (errorinfo($err)) {
		print "ok\n";
		for ($i = 0; $i < 32; $i++) {
			my $itm;
			
			($err,$itm) = ncpfs::ncp_ns_get_namespace_info_element($lay, 0xFFFFFFFF, $inf, $i);
			if (not $err) {
				printf 'Item %u: %s'."\n", $i, unpack('H*', $itm);
			}
		}
	}
}	
	
sub namespacedemo($) {
	my ($conn) = @_;
	
	dumpnsinfo($conn, 0, 0, 0);
	dumpnsinfo($conn, 1, 0, 0);
	dumpnsinfo($conn, 2, 0, 0);
	dumpnsinfo($conn, 3, 0, 0);
	dumpnsinfo($conn, 4, 0, 0);
	dumpnsinfo($conn, 5, 0, 0);
	print 'Changing attributes through DOS namespace info on SYS: ... ';
	my ($err) = ncpfs::ncp_ns_modify_entry_namespace_info($conn, $ncpfs::NW_NS_DOS, 0, 0, $ncpfs::NW_NS_DOS, 0x0002, pack('H*', '16000000'));
	errorall($err);
}

sub ns3demo($) {
	my ($conn) = @_;

	print 'Allocating short directory handle for SYS: ... ';
	my ($err, $handle, $vol) = ncpfs::ncp_ns_alloc_short_dir_handle($conn, $ncpfs::NW_NS_DOS, $ncpfs::NCP_DIRSTYLE_DIRBASE,
		0, 0, undef, $ncpfs::NCP_ALLOC_TEMPORARY);
	if (errorinfo($err)) {
		print "ok, handle=$handle, volume=$vol\n";
		my ($info);
		
		print 'Obtaining info about LOGIN directory... ';
		($err,$info) = ncpfs::ncp_ns_obtain_entry_info($conn, $ncpfs::NW_NS_DOS, $ncpfs::SA_ALL,
			$ncpfs::NCP_DIRSTYLE_HANDLE, 0, $handle, scalar ncpfs::ncp_path_to_NW_format('LOGIN'),
			$ncpfs::NW_NS_DOS, 0x81FFFFFF, $ncpfs::NW_INFO_STRUCT3);
		if (errorinfo($err)) {
			print "ok, $info\n";
			dumpInsideHash('', $info);
			my $i;
			for ($i = 0; $i < 32; $i++) {
				my ($v);
				printf 'Field %u has size ',$i;
				($err,$v) = ncpfs::ncp_ns_extract_info_field_size($info, $i);
				if ($err) {
					print '<unknown, error '.ncpfs::strnwerror($err).'>'."\n";
				} else {
					print "$v bytes\n";
				}
			}
		}
		print 'Deallocating ... ';
		$err = ncpfs::ncp_dealloc_dir_handle($conn, $handle);
		errorall($err);
	}
	
}

sub convdemo() {
	print "Rights 0xF0 => ".ncpfs::ncp_perms_to_str(0xF0)."\n";
	print "Namespace 4 => ".ncpfs::ncp_namespace_to_str(4)."\n";
	my ($err,$z) = ncpfs::ncp_str_to_perms('[RWC]');
	if ($err) {
		print "Cannot convert [RWC] to rights: ".ncpfs::strnwerror($err)."\n";
	} else {
		printf "[RWC] => %04X\n", $z;
	}
}

sub fidinfo($) {
	my ($conn) = @_;
	
	my ($fid) = ncpfs::ncp_get_fid($conn);
	print "FID=$fid\n";
	my ($err, $uid) = ncpfs::ncp_get_mount_uid($fid);
	if ($err) {
		print "Cannot retrieve mount uid: ".ncpfs::strnwerror($err)."\n";
	} else {
		print "Mount uid is $uid\n";
	}
}

sub nwparsepath($) {
	my ($path) = @_;
	print "NWParsePath(\"$path\")... ";
	my ($err,$server,$nconn,$volume,$volpath) = ncpfs::NWParsePath($path);
	if ($err) {
		printf "error: %s\n", ncpfs::strnwerror($err);
	} elsif (defined($nconn)) {
		print "remote, server=$server, conn=$nconn, volume=$volume, path=$volpath\n";
		ncpfs::NWCCCloseConn($nconn);
	} else {
		print "local, path=$volpath\n";
	}
}

sub nwcallsdemo($) {
	my ($conn) = @_;
	
	print 'NWCallsInit()... ';
	my ($err) = ncpfs::NWCallsInit(undef, undef);
	errorall($err);

	my ($name, $type);
	print 'NWGetObjectName(conn, 1)... ';
	($err, $name, $type) = ncpfs::NWGetObjectName($conn, 1);
	if (errorinfo($err)) {
		printf "ok, %s(0x%04X)\n", $name, $type;
	}

	print 'NWGetObjectID(conn, SUPERVISOR, 1)... ';
	($err, $type) = ncpfs::NWGetObjectID($conn, 'SUPERVISOR', 1);
	if (errorinfo($err)) {
		printf "ok, 0x%08X\n", $type;
	}

	my ($level, $id);
	print 'NWGetBinderyAccessLevel(conn)... ';
	($err, $level, $id) = ncpfs::NWGetBinderyAccessLevel($conn);
	if (errorinfo($err)) {
		printf "ok, level=0x%02X, id=0x%08X\n", $level, $id;
	}

	print 'NWVerifyObjectPassword... ';
	$err = ncpfs::NWVerifyObjectPassword($conn, 'SUPERVISOR', 1, 'AAAAA');
	errorall($err);

	my ($info);
	print 'NWGetNSEntryInfo... ';
	($err, $info) = ncpfs::NWGetNSEntryInfo($conn, 0, 'SYS:LOGIN',
		$ncpfs::NW_NS_DOS, $ncpfs::NW_NS_LONG, 0x8006, 0xFFF);
	if (errorinfo($err)) {
		print "ok, $info\n";
		dumpInsideHash('  ', $info);
	}

	nwparsepath("/nw6/sys/login");
	nwparsepath("/tmp");
	print 'ncp_get_volume_name(conn,0) = ';
	($err,$info) = ncpfs::ncp_get_volume_name($conn, 0);
	if (errorinfo($err)) {
		print "ok, $info\n";
	}

	print 'NWGetVolumeNumber(conn, "SYS") = ';
	($err,$info) = ncpfs::NWGetVolumeNumber($conn, 'SYS');
	if (errorinfo($err)) {
		print "ok, $info\n";
	}

	print 'NWGetVolumeName(conn, 0) = ';
	($err,$info) = ncpfs::NWGetVolumeName($conn, 0);
	if (errorinfo($err)) {
		print "ok, $info\n";
	}

	print 'NWGetNSLoadedList(conn, 0) = ';
	($err,$info) = ncpfs::NWGetNSLoadedList($conn, 0);
	if (errorinfo($err)) {
		printf "ok, $info, %u items\n", scalar @$info;
		dumpInsideArray('  ', $info);
	}
	my ($cnum);
	my ($wl_vol, $wl_dir);	
	for ($cnum = 1; $cnum < 10; $cnum++) {
		my ($ctrl, $of, $n);
		
		$ctrl = new ncpfs::OPEN_FILE_CONN_CTRL;
		$n = 0;
		while (1) {
			($err,$of) = ncpfs::NWScanOpenFilesByConn2($conn, $cnum, \$n, $ctrl);
			if ($err) {
				last;
			}
			print "Ctrl:\n";
			dumpInsideHash('  ', $ctrl);
			print "[$n] $of\n";
			dumpInsideHash('  ', $of);
			$wl_vol = $of->{'volNumber'};
			$wl_dir = $of->{'dirEntry'};
		}
		printf "Scan open files by conn terminated: %s\n", ncpfs::strnwerror($err);
	}
	if (defined($wl_vol)) {
		my ($ctrl, $of, $n);
		
		$ctrl = new ncpfs::CONNS_USING_FILE;
		$n = 0;
		while (1) {
			($err,$of) = ncpfs::ncp_ns_scan_connections_using_file($conn,
				$wl_vol, $wl_dir, 0, \$n, $ctrl);
			if ($err) {
				last;
			}
			print "Ctrl:\n";
			dumpInsideHash('  ', $ctrl);
			print "[$n] $of\n";
			dumpInsideHash('  ', $of);
		}
		printf "Scan connections using file terminated: %s\n", ncpfs::strnwerror($err);
	}
	if (defined($wl_vol)) {
		my ($ctrl, $of, $n);
		
		$ctrl = new ncpfs::PHYSICAL_LOCKS;
		$n = 0;
		while (1) {
			($err,$of) = ncpfs::ncp_ns_scan_physical_locks_by_file($conn,
				$wl_vol, $wl_dir, 0, \$n, $ctrl);
			if ($err) {
				last;
			}
			print "Ctrl:\n";
			dumpInsideHash('  ', $ctrl);
			print "[$n] $of\n";
			dumpInsideHash('  ', $of);
		}
		printf "Scan physical locks by file terminated: %s\n", ncpfs::strnwerror($err);
	}
}

sub nwbindry($) {
	my ($conn) = @_;
	my ($err);
	
	print 'NWCloseBindery(conn) = ';
	$err = ncpfs::NWCloseBindery($conn);
	errorall($err);
	
	print 'NWOpenBindery(conn) = ';
	$err = ncpfs::NWOpenBindery($conn);
	errorall($err);

	if (0) {
		print 'NWDownFileServer(conn,0) = ';
		$err = ncpfs::NWDownFileServer($conn,0);
		errorall($err);
	}
	print 'NWDisableFileServerLogin(conn) = ';
	$err = ncpfs::NWDisableFileServerLogin($conn);
	errorall($err);

	print 'NWEnableFileServerLogin(conn) = ';
	$err = ncpfs::NWEnableFileServerLogin($conn);
	errorall($err);

	print 'NWDisableTTS(conn) = ';
	$err = ncpfs::NWDisableTTS($conn);
	errorall($err);

	print 'NWEnableTTS(conn) = ';
	$err = ncpfs::NWEnableTTS($conn);
	errorall($err);
}

sub nwrpc($) {
	my ($conn) = @_;
	my ($err, $vol);
	
	print 'NWSMLoadNLM(conn, "MONITOR.NLM") = ';
	$err = ncpfs::NWSMLoadNLM($conn, 'MONITOR.NLM');
	errorall($err);

	print 'NWSMUnloadNLM(conn, "MONITOR") = ';
	$err = ncpfs::NWSMUnloadNLM($conn, 'MONITOR');
	errorall($err);

	print 'NWSMDismountVolumeByName(conn, "SYS") = ';
	$err = ncpfs::NWSMDismountVolumeByName($conn, 'SYS');
	errorall($err);

	print 'NWSMMountVolume(conn, "SYS") = ';
	($err, $vol) = ncpfs::NWSMMountVolume($conn, 'SYS');
	if (errorinfo($err)) {
		printf "ok, volume number %u\n", $vol;
		printf 'NWSMDismountVolumeByNumber(conn, %u) = ', $vol;
		$err = ncpfs::NWSMDismountVolumeByNumber($conn, $vol);
		if (errorinfo($err)) {
			print "ok, remounting back: ";
			($err,$vol) = ncpfs::NWSMMountVolume($conn, 'SYS');
			if (errorinfo($err)) {
				printf "ok, volume number %u\n", $vol;
			}
		}
	}
	print 'NWSMExecuteNCFFile(conn,"BSTART.NCF") = ';
	$err = ncpfs::NWSMExecuteNCFFile($conn, 'BSTART.NCF');
	errorall($err);
}

sub nwsmset($) {
	my ($conn) = @_;
	my ($err);
	
	print 'NWSMSetDynamicCmdStrValue(conn, "DSTRACE", "*.") = ';
	$err = ncpfs::NWSMSetDynamicCmdStrValue($conn, 'DSTRACE', '*.');
	errorall($err);

	print 'NWSMSetDynamicCmdIntValue(conn, "Maximum transactions", 9999) = ';
	$err = ncpfs::NWSMSetDynamicCmdIntValue($conn, 'Maximum transactions', 9999);
	errorall($err);
}

sub nwconnsop($) {
	my ($conn) = @_;
	my ($err, $cnum, $srvname, $srvver, $connlist, $status, $bcstmode);

	print 'NWGetConnectionNumber(conn) = ';
	($err, $cnum) = ncpfs::NWGetConnectionNumber($conn);
	if (errorinfo($err)) {
		printf "ok, connection %u\n", $cnum;
	}
	print 'NWGetFileServerName(conn) = ';
	($err, $srvname) = ncpfs::NWGetFileServerName($conn);
	if (errorinfo($err)) {
		printf "ok, name %s\n", $srvname;
	}
	print 'NWGetFileServerVersion(conn) = ';
	($err, $srvver) = ncpfs::NWGetFileServerVersion($conn);
	if (errorinfo($err)) {
		printf "ok, version %u.%02u\n", $srvver / 256, $srvver % 256;
	}
	printf 'NWGetObjectConnectionNumbers(conn, "%s", %u) = ', $test_objectname, $test_objecttype;
	($err, $connlist) = ncpfs::NWGetObjectConnectionNumbers($conn, $test_objectname, $test_objecttype);
	if (errorinfo($err)) {
		print "ok, $connlist\n";
		dumpInsideArray('  ', $connlist);
	}
	printf 'NWGetConnListFromObject(conn, 0x%08X, 0) = ', $test_objectid;
	($err, $connlist) = ncpfs::NWGetConnListFromObject($conn, $test_objectid, 0);
	if (errorinfo($err)) {
		print "ok, $connlist\n";
		dumpInsideArray('  ', $connlist);
	}
	print 'NWDisableBroadcasts(conn) = ';
	$err = ncpfs::NWDisableBroadcasts($conn);
	errorall($err);

	print 'NWSendBroadcastMessage(conn, "Test", $connlist) = ';
	($err, $status) = ncpfs::NWSendBroadcastMessage($conn, "Test", $connlist);
	if (errorinfo($err)) {
		print "ok, $status\n";
		dumpInsideArray('  ', $status);
	}
	print 'NWEnableBroadcasts(conn) = ';
	$err = ncpfs::NWEnableBroadcasts($conn);
	errorall($err);
	print 'NWGetBroadcastMode(conn) = ';
	($err, $bcstmode) = ncpfs::NWGetBroadcastMode($conn);
	if (errorinfo($err)) {
		printf "ok, mode %u\n", $bcstmode;
		printf 'NWSetBroadcastMode(conn, %u) = ', $bcstmode;
		$err = ncpfs::NWSetBroadcastMode($conn, $bcstmode);
		errorall($err);
	}
}

sub nwncpext($) {
	my ($conn) = @_;
	my ($err, $cnt, $iter);
	
	print 'NWGetNumberNCPExtensions(conn) = ';
	($err, $cnt) = ncpfs::NWGetNumberNCPExtensions($conn);
	if (errorinfo($err)) {
		printf "ok, %u extensions loaded\n", $cnt;
	}
	$iter = -1;
	while (1) {
		my ($name,$maj,$min,$rev,$qdata);
		
		printf 'NWScanNCPExtensions(conn, \%u) = ', $iter;
		($err, $name, $maj, $min, $rev, $qdata) = 
			ncpfs::NWScanNCPExtensions($conn, \$iter);
		if ($err) {
			last;
		}
		printf "ok, id=0x%08X, ver=%u.%u.%u,\nname=%s,\nqdata=%s\n",
			$iter, $maj, $min, $rev, $name, unpack('H*', $qdata);
	}
	printf "failed, %s\n", ncpfs::strnwerror($err);
}

sub openbytran($) {
	my ($addr) = @_;
	my ($err, $nconn);
	
	print 'NWCCOpenConnByAddr(addr, $ncpfs::NWCC_OPEN_PRIVATE, $ncpfs::NWCC_RESERVED) = ';
	($err, $nconn) = ncpfs::NWCCOpenConnByAddr($addr,
		$ncpfs::NWCC_OPEN_PRIVATE, $ncpfs::NWCC_RESERVED);
	if (errorinfo($err)) {
		printf "ok, conn2=$nconn\n";
		
		print 'NWCCCloseConn(conn2) = ';
		$err = ncpfs::NWCCCloseConn($nconn);
		errorall($err);
	}
}

sub buildipx($$$) {
	my ($net, $node, $sock) = @_;
	
	return pack('snNH12CC', 4, $sock, $net, $node, 0x11, 0);
}

sub nwconnopen($) {
	my ($conn) = @_;
	my ($err, $nconn);
	
	printf 'NWCCOpenConnByName(undef, "%s", $ncpfs::NWCC_NAME_FORMAT_BIND, $ncpfs::NWCC_OPEN_PRIVATE, $ncpfs::NWCC_RESERVED) = ', $server_name;
	($err, $nconn) = ncpfs::NWCCOpenConnByName(undef, $server_name,
		$ncpfs::NWCC_NAME_FORMAT_BIND, $ncpfs::NWCC_OPEN_PRIVATE,
		$ncpfs::NWCC_RESERVED);
	if (errorinfo($err)) {
		my ($cnum);
		
		printf "ok, nconn=$nconn\n";
		
		print 'NWGetConnectionNumber(nconn) = ';
		($err, $cnum) = ncpfs::NWGetConnectionNumber($nconn);
		if (errorinfo($err)) {
			printf "ok, connection %u\n", $cnum;
		}
		if (defined($cnum)) {
			printf 'NWClearConnectionNumber(nconn, %u) = ', $cnum;
			$err = ncpfs::NWClearConnectionNumber($nconn, $cnum);
			errorall($err);
		}
		{
			my ($addrlen,$addr);
			my ($optn);

			for ($optn = 0; $optn < 0x4010; $optn++) {
				my ($val);
				if ($optn == 16) {
					$optn = 0x4000;
				}
				printf 'NWCCGetConnInfo(conn, %u) = ', $optn;
				($err,$val) = ncpfs::NWCCGetConnInfo($nconn, $optn);
				if (errorinfo($err)) {
					print "ok, $val\n";
					dumpInside('  ', $val);
				} 
			}
			print 'NWCCGetConnAddressLength(nconn) = ';
			($err, $addrlen) = ncpfs::NWCCGetConnAddressLength($nconn);
			if (errorinfo($err)) {
				printf "ok, %u\n", $addrlen;
				
				print 'NWCCGetConnAddress(nconn) = ';
				($err, $addr) = ncpfs::NWCCGetConnAddress($nconn);
				if (errorinfo($err)) {
					print "ok, $addr\n";
					dumpInsideHash('  ', $addr);
					printf "Address: %s\n", unpack('H*', $addr->{'buffer'});
				}
				openbytran($addr);
			}
			my ($conn2);
			
			print 'NWCCOpenConnBySockAddr(conn, ADDR, IPX, ...) = ';
			($err, $conn2) = ncpfs::NWCCOpenConnBySockAddr(
				buildipx(0xDD354240, '000000000001', 0x451),
				$ncpfs::NT_IPX, $ncpfs::NWCC_OPEN_PRIVATE,
				$ncpfs::NWCC_RESERVED);
			if (errorinfo($err)) {
				print "ok, conn2=$conn2\n";
				print 'NWCCCloseConn(conn2) = ';
				$err = ncpfs::NWCCCloseConn($conn2);
				errorall($err);
			}				
		}

		print 'NWCCCloseConn(nconn) = ';
		$err = ncpfs::NWCCCloseConn($nconn);
		errorall($err);
	}
}

sub setdirlimit($$$) {
	my ($conn, $path, $limit) = @_;
	my ($err);

	my $ns = new ncpfs::ncp_dos_info;
	$ns->{'MaximumSpace'} = $limit;
	printf 'Limiting directory %s with limit %u: ', $path, $limit;
	$err = ncpfs::ncp_ns_modify_entry_dos_info($conn, $ncpfs::NW_NS_DOS,
		$ncpfs::SA_ALL, $ncpfs::NCP_DIRSTYLE_NOHANDLE,  0, 0,
		scalar ncpfs::ncp_path_to_NW_format($path), 
		$ncpfs::DM_MAXIMUM_SPACE, $ns);
	errorall($err);
}

sub nwdiskrest($) {
	my ($conn) = @_;
	my ($err, $rest, $inuse, $rest2, $inuse2, $ih);

	printf 'NWGetObjDiskRestrictions(conn, 0, 0x%08X) = ', $test_objectid;
	($err, $rest, $inuse) = ncpfs::NWGetObjDiskRestrictions($conn, 0, $test_objectid);
	if (errorinfo($err)) {
		printf "ok, restriction 0x%08X, in use 0x%08X\n", $rest, $inuse;
	}
	
	printf 'NWSetObjectVolSpaceLimit(conn, 0, 0x%08X, 123) = ', $test_objectid;
	$err = ncpfs::NWSetObjectVolSpaceLimit($conn, 0, $test_objectid, 123);
	errorall($err);

	printf 'NWGetObjDiskRestrictions(conn, 0, 0x%08X) = ', $test_objectid;
	($err, $rest2, $inuse2) = ncpfs::NWGetObjDiskRestrictions($conn, 0, $test_objectid);
	if (errorinfo($err)) {
		printf "ok, restriction 0x%08X, in use 0x%08X\n", $rest2, $inuse2;
	}
	
	$ih = 0;
	while (1) {
		my ($volinfo);

		printf 'NWScanVolDiskRestrictions(conn, 0, \%u) = ', $ih;		
		($err, $volinfo) = ncpfs::NWScanVolDiskRestrictions($conn, 0, \$ih);
		if ($err) {
			printf "failed, %s\n", ncpfs::strnwerror($err);	
			last;
		}
		print "ok, $volinfo\n";
		dumpInsideHash('  ',$volinfo);
		if ($volinfo->{'numberOfEntries'} == 0) {
			last;
		}
		$ih += $volinfo->{'numberOfEntries'};
	}

	$ih = 0;
	while (1) {
		my ($volinfo);

		printf 'NWScanVolDiskRestrictions2(conn, 0, \%u) = ', $ih;		
		($err, $volinfo) = ncpfs::NWScanVolDiskRestrictions2($conn, 0, \$ih);
		if ($err) {
			printf "failed, %s\n", ncpfs::strnwerror($err);	
			last;
		}
		print "ok, $volinfo\n";
		dumpInsideHash('  ',$volinfo);
		if ($volinfo->{'numberOfEntries'} == 0) {
			last;
		}
		$ih += $volinfo->{'numberOfEntries'};
	}
	
	printf 'NWRemoveObjectDiskRestrictions(conn, 0, 0x%08X) = ', $test_objectid;
	$err = ncpfs::NWRemoveObjectDiskRestrictions($conn, 0, $test_objectid);
	errorall($err);

	printf 'NWGetObjDiskRestrictions(conn, 0, 0x%08X) = ', $test_objectid;
	($err, $rest2, $inuse2) = ncpfs::NWGetObjDiskRestrictions($conn, 0, $test_objectid);
	if (errorinfo($err)) {
		printf "ok, restriction 0x%08X, in use 0x%08X\n", $rest2, $inuse2;
	}
	
	setdirlimit($conn, 'SYS:',               1048576000);
	setdirlimit($conn, 'SYS:LOGIN',           123456789);
	setdirlimit($conn, 'SYS:LOGIN/NLS',        12345678);
	setdirlimit($conn, 'SYS:LOGIN/NLS/ENGLISH', 1234567);
	printf 'Allocating directory handle for SYS:LOGIN/NLS/ENGLISH: ';
	($err, $ih, $rest2) = ncpfs::ncp_ns_alloc_short_dir_handle($conn,
		$ncpfs::NW_NS_DOS, $ncpfs::NCP_DIRSTYLE_NOHANDLE, 0, 0,
		scalar ncpfs::ncp_path_to_NW_format('SYS:LOGIN/NLS/ENGLISH'),
		$ncpfs::NCP_ALLOC_TEMPORARY);
	if (errorinfo($err)) {
		my ($buff);
		
		printf "ok, handle=%u, volume=%u\n", $ih, $rest2;
		
		printf 'NWGetDirSpaceLimitList(conn, %u) = ', $ih;
		($err, $buff) = ncpfs::NWGetDirSpaceLimitList($conn, $ih);
		if (errorinfo($err)) {
			printf "ok, %s\n", unpack('H*', $buff);
		}
		printf 'NWGetDirSpaceLimitList2(conn, %u) = ', $ih;
		($err, $buff) = ncpfs::NWGetDirSpaceLimitList2($conn, $ih);
		if (errorinfo($err)) {
			print "ok, $buff\n";
			dumpInsideHash('  ', $buff);
		}
		printf 'ncp_get_directory_info(conn, %u) = ', $ih;
		($err, $buff) = ncpfs::ncp_get_directory_info($conn, $ih);
		if (errorinfo($err)) {
			print "ok, $buff\n";
			dumpInsideHash('  ', $buff);
		}
		setdirlimit($conn, 'SYS:',                  0);
		setdirlimit($conn, 'SYS:LOGIN',             0);
		setdirlimit($conn, 'SYS:LOGIN/NLS',         0);
		setdirlimit($conn, 'SYS:LOGIN/NLS/ENGLISH', 0);
		printf 'NWGetDirSpaceLimitList2(conn, %u) = ', $ih;
		($err, $buff) = ncpfs::NWGetDirSpaceLimitList2($conn, $ih);
		if (errorinfo($err)) {
			print "ok, $buff\n";
			dumpInsideHash('  ', $buff);
		}
	}
}

sub nwsema($) {
	my ($conn) = @_;
	my ($err, $semh, $opncnt);
	
	print 'NWOpenSemaphore(conn, "NCPFSSEMA", 0) = ';
	($err, $semh, $opncnt) = ncpfs::NWOpenSemaphore($conn, 'NCPFSSEMA', 0);
	if (errorinfo($err)) {
		my ($semv);
		printf "ok, handle=0x%08X, open count=%u\n", $semh, $opncnt;
		
		printf 'NWExamineSemaphore(conn, 0x%08X) = ', $semh;
		($err, $semv, $opncnt) = ncpfs::NWExamineSemaphore($conn, $semh);
		if (errorinfo($err)) {
			printf "ok, value=%u, open count=%u\n", $semv, $opncnt;
		}
		
		printf 'NWSignalSemaphore(conn, 0x%08X) = ', $semh;
		$err = ncpfs::NWSignalSemaphore($conn, $semh);
		errorall($err);
		
		printf 'NWExamineSemaphore(conn, 0x%08X) = ', $semh;
		($err, $semv, $opncnt) = ncpfs::NWExamineSemaphore($conn, $semh);
		if (errorinfo($err)) {
			printf "ok, value=%u, open count=%u\n", $semv, $opncnt;
		}
		
		my ($cnum, $ctrl, $of, $n);
		
		($err, $cnum) = ncpfs::NWGetConnectionNumber($conn);
		
		$ctrl = new ncpfs::CONN_SEMAPHORES;
		$n = 0;
		while (1) {
			($err,$of) = ncpfs::NWScanSemaphoresByConn($conn,
				$cnum, \$n, $ctrl);
			if ($err) {
				last;
			}
			print "Ctrl:\n";
			dumpInsideHash('  ', $ctrl);
			print "[$n] $of\n";
			dumpInsideHash('  ', $of);
		}
		printf "Scan semaphores by conn terminated: %s\n", ncpfs::strnwerror($err);

		printf 'NWWaitOnSemaphore(conn, 0x%08X, 1) = ', $semh;
		$err = ncpfs::NWWaitOnSemaphore($conn, $semh, 1);
		errorall($err);
		
		printf 'NWCloseSemaphore(conn, 0x%08X) = ', $semh;
		$err = ncpfs::NWCloseSemaphore($conn, $semh);
		errorall($err);
	}
}

sub readea1($$$$) {
	my ($conn, $vol, $dirent, $aname) = @_;
	my ($offset, $err, $hnd, $data);
	
	$offset = 0;
	printf 'Reading extended attribute "%s": ', $aname;
	($err, $hnd, $data) = ncpfs::ncp_ea_read($conn,
		$ncpfs::NWEA_FL_CLOSE_ERR|$ncpfs::NWEA_FL_SRC_DIRENT,
		$vol, $dirent, -1, $aname, $offset);
	while (not $err) {
		print "Read hnd=$hnd\n";
		dumpInside('  ',$hnd);
		print "Data=$data\n";
		## last unless length($data);
		$offset += length($data);

		printf 'Reading at offset %u: ', $offset;
		($err, $hnd, $data) = ncpfs::ncp_ea_read($conn,
			$ncpfs::NWEA_FL_CLOSE_ERR|$ncpfs::NWEA_FL_SRC_EAHANDLE,
			$hnd->{'newEAhandle'}, 0, -1, undef, $offset);
	}
	errorinfo($err);
}

sub dumpeaX(&$$$$;$) {
	my ($proc, $flags, $conn, $vol, $dirent, $aname) = @_;
	my ($hnd, $err, $data);
	
	$hnd = new ncpfs::ncp_ea_enumerate_info;
	printf 'Starting EA enumeration type %u: ', $flags / 16;
	($err, $data) = ncpfs::ncp_ea_enumerate($conn, 
		$flags | 
		$ncpfs::NWEA_FL_CLOSE_ERR | $ncpfs::NWEA_FL_SRC_DIRENT,
		$vol, $dirent, -1, $aname, $hnd);
	while ($err == 0) {
		my ($keycnt);
		print "ok, Handle=$hnd\n";
		dumpInside('  ', $hnd);
		printf "Data=%s\n", unpack('H*', $data);
		for ($keycnt = 0; $keycnt < $hnd->{'returnedItems'}; $keycnt++) {
			my ($inf);
			
			printf 'Attribute %u: ', $keycnt;
			($err,$data,$inf) = $proc->($data);
			if (errorinfo($err)) {
				print "ok, $inf\n";
				dumpInside('  ', $inf);
				printf "NewData=%s\n", unpack('H*', $data);
			}
		}
		last unless ($hnd->{'enumSequence'});
		print 'Next EA loop: ';
		($err, $data) = ncpfs::ncp_ea_enumerate($conn,
			$flags | $ncpfs::NWEA_FL_CLOSE_ERR | 
			$ncpfs::NWEA_FL_SRC_EAHANDLE,
			$hnd->{'newEAhandle'}, 0, -1, undef, $hnd);
	}
	errorinfo($err);
	if ($hnd->{'newEAhandle'}) {
		printf 'ncp_ea_close(conn, %u) = ', $hnd->{'newEAhandle'};
		$err = ncpfs::ncp_ea_close($conn, $hnd->{'newEAhandle'});
		errorall($err);
	}
}

sub dumpea0($$$) {
	my ($conn, $vol, $dirent) = @_;
	
	dumpeaX(sub { return -1 }, $ncpfs::NWEA_FL_INFO0, $conn, $vol, $dirent); 
}

sub dumpea1($$$) {
	my ($conn, $vol, $dirent) = @_;

	dumpeaX(\&ncpfs::ncp_ea_extract_info_level1, $ncpfs::NWEA_FL_INFO1,
		$conn, $vol, $dirent);
}

sub dumpea6($$$$) {
	my ($conn, $vol, $dirent, $aname) = @_;
	
	dumpeaX(\&ncpfs::ncp_ea_extract_info_level6, $ncpfs::NWEA_FL_INFO6,
		$conn, $vol, $dirent, $aname);
}

sub dumpea7($$$) {
	my ($conn, $vol, $dirent) = @_;

	dumpeaX(\&ncpfs::ncp_ea_extract_info_level7, $ncpfs::NWEA_FL_INFO7,
		$conn, $vol, $dirent);
}

sub name2entry($$) {
	my ($conn, $path) = @_;
	my ($err, $dirinfo);
	
	printf 'Mapping %s to directory entry: ', $path;
	($err, $dirinfo) = ncpfs::ncp_ns_obtain_entry_info($conn, 
			$ncpfs::NW_NS_DOS, $ncpfs::SA_ALL,
			$ncpfs::NCP_DIRSTYLE_NOHANDLE, 0, 0,
			scalar ncpfs::ncp_path_to_NW_format($path), 
			$ncpfs::NW_NS_DOS, $ncpfs::IM_DIRECTORY,
			$ncpfs::NW_INFO_STRUCT2);
	if (errorinfo($err)) {
		my ($vol, $dirent);
		$vol = $dirinfo->{'Directory'}->{'volNumber'};
		$dirent = $dirinfo->{'Directory'}->{'dirEntNum'};
		printf "ok, got %u:0x%08X\n", $vol, $dirent;
		return ($vol, $dirent);
	}
	return ();
}

sub nwea($$$) {
	my ($conn, $path, $path2) = @_;
	my ($err, $dirinfo);
	my ($vol, $dirent) = name2entry($conn, $path);
	if (not defined($vol)) {
		return;
	}
	my ($vol2, $dirent2) = name2entry($conn, $path2);
	if (not defined($vol2)) {
		return;
	}
	dumpea1($conn, $vol, $dirent);

	my ($winfo);
	printf 'Creating some EA: ';
	($err, $winfo) = ncpfs::ncp_ea_write($conn,
		$ncpfs::NWEA_FL_CLOSE_ERR |
		$ncpfs::NWEA_FL_SRC_DIRENT, $vol, $dirent, 1024,
		'MyExtendedAttributeForTesting', 0, 0,
		'Test_Data ' x 51 . '01');
	if (errorinfo($err)) {
		printf "ok, info=$winfo\n";
		dumpInside('  ', $winfo);
		($err,$winfo) = ncpfs::ncp_ea_write($conn,
			$ncpfs::NWEA_FL_CLOSE_ERR |
			$ncpfs::NWEA_FL_CLOSE_IMM |
			$ncpfs::NWEA_FL_SRC_EAHANDLE, 
			$winfo->{'newEAhandle'}, 0, 1024,
			undef, 512, 0,
			'SecondHalf' x 51 . 'ab');
		if (errorinfo($err)) {
			printf "ok, info=$winfo\n";
			dumpInside('  ', $winfo);
		}				
		dumpea1($conn, $vol, $dirent);
	}
	readea1($conn, $vol, $dirent, 'MyExtendedAttributeForTesting');
	{
		my ($copycount, $datasize, $keysize);
		print 'Duplicating EAs: ';
		($err, $copycount, $datasize, $keysize) = ncpfs::ncp_ea_duplicate($conn,
			$ncpfs::NWEA_FL_SRC_DIRENT, $vol, $dirent,
			$ncpfs::NWEA_FL_SRC_DIRENT, $vol2, $dirent2);
		if (errorinfo($err)) {
			printf "ok, count=%u, datasize=%u, keysize=%u\n",
				$copycount, $datasize, $keysize;
		}
		dumpea0($conn, $vol2, $dirent2);
		dumpea1($conn, $vol2, $dirent2);
		dumpea6($conn, $vol2, $dirent2, 'MyExtendedAttributeForTesting');
		dumpea7($conn, $vol2, $dirent2);
	}
	printf 'Removing EA: ';
	($err, $winfo) = ncpfs::ncp_ea_write($conn,
		$ncpfs::NWEA_FL_CLOSE_ERR | $ncpfs::NWEA_FL_CLOSE_IMM |
		$ncpfs::NWEA_FL_SRC_DIRENT, $vol, $dirent, 0,
		'MyExtendedAttributeForTesting', 0, 0,
		undef);
	if (errorinfo($err)) {
		printf "ok, info=$winfo\n";
		dumpInside('  ', $winfo);
	}
	printf 'Removing EA from copy: ';
	($err, $winfo) = ncpfs::ncp_ea_write($conn,
		$ncpfs::NWEA_FL_CLOSE_ERR | $ncpfs::NWEA_FL_CLOSE_IMM |
		$ncpfs::NWEA_FL_SRC_DIRENT, $vol2, $dirent2, 0,
		'MyExtendedAttributeForTesting', 0, 0,
		undef);
	if (errorinfo($err)) {
		printf "ok, info=$winfo\n";
		dumpInside('  ', $winfo);
	}
	dumpea1($conn, $vol, $dirent);
	dumpea1($conn, $vol2, $dirent2);
}

sub nwdsbufcheck($$) {
	my ($conn,$ctx) = @_;
	my ($err,$buf);

	print 'new ncpfs::Buf_T = ';
	$buf = new ncpfs::Buf_T;
	print "$buf\n";
	
	print 'NWDSAllocBuf(4096) = ';
	($err, $buf) = ncpfs::NWDSAllocBuf(4096);
	if (errorinfo($err)) {
		my ($buf2);

		print "ok, buf=$buf\n";
		dumpInside('  ',$buf);

		print 'NWDSInitBuf(ctx, $ncpfs::DSV_READ_ATTR_DEF, buf) = ';
		$err = ncpfs::NWDSInitBuf($ctx, $ncpfs::DSV_READ_ATTR_DEF, $buf);
		errorall($err);

		my ($ih) = ($ncpfs::NO_MORE_ITERATIONS);
			
		print 'NWDSPutClassName(ctx, buf, "User") = ';
		$err = ncpfs::NWDSPutClassName($ctx, $buf, 'User');
		errorall($err);
			
		print 'NWDSReadAttrDef(ctx, 1, 0, buf, \ih) = ';
		($err, $buf2) = ncpfs::NWDSReadAttrDef($ctx, 1, 0, $buf, \$ih);
		if (errorinfo($err)) {
			print "ok, buf2=$buf2\n";

			my ($cnt);
			print 'NWDSGetAttrCount(ctx, buf2) = ';
			($err, $cnt) = ncpfs::NWDSGetAttrCount($ctx, $buf2);
			if (errorinfo($err)) {
				print "ok, cnt=$cnt\n";

				while ($cnt) {
					my ($name, $ainfo);

					print 'NWDSGetAttrDef(ctx, buf2) = ';
					($err,$name,$ainfo) = ncpfs::NWDSGetAttrDef($ctx, $buf2);
					if (errorinfo($err)) {
						print "ok, name=$name, ainfo=$ainfo\n";
						
						dumpInside('  ', $ainfo);
						::Dump($name);
					}
					$cnt--;
				}
			}		
			print 'NWDSFreeBuf(buf2) = ';
			$err = ncpfs::NWDSFreeBuf($buf2);
			errorall($err);
		}
		print 'NWDSFreeBuf(buf) = ';
		$err = ncpfs::NWDSFreeBuf($buf);
		errorall($err);
	}
}

sub nwdsctxcheck($) {
	my ($conn) = @_;
	my ($err,$ctx);
	
	print 'NWDSInitRequester() = ';
	$err = ncpfs::NWDSInitRequester();
	errorall($err);
	print 'NWDSCreateContext() = ';
	$ctx = ncpfs::NWDSCreateContext();
	if (not defined($ctx)) {
		print "failed\n";
	} else {
		my ($nctx);
		print "ok, ctx=$ctx\n";
		print 'NWDSDuplicateContext(ctx) = ';
		$nctx = ncpfs::NWDSDuplicateContext($ctx);
		if (not defined($nctx)) {
			print "failed\n";
		} else {
			print "ok, nctx=$nctx\n";
			print 'NWDSFreeContext(nctx) = ';
			$err = ncpfs::NWDSFreeContext($nctx);
			errorall($err);
		}
		print 'NWDSFreeContext(ctx) = ';
		$err = ncpfs::NWDSFreeContext($ctx);
		errorall($err);
	}
	print 'NWDSCreateContextHandle() = ';
	($err,$ctx) = ncpfs::NWDSCreateContextHandle();
	if (errorinfo($err)) {
		my ($nctx);
		print "ok, ctx=$ctx\n";
		print 'NWDSDuplicateContextHandle(ctx) = ';
		($err,$nctx) = ncpfs::NWDSDuplicateContextHandle($ctx);
		if (errorinfo($err)) {
			print "ok, nctx=$nctx\n";
			print 'NWDSFreeContext(nctx) = ';
			$err = ncpfs::NWDSFreeContext($nctx);
			errorall($err);
		}
		print 'NWDSAddConnection(ctx,conn) = ';
		$err = ncpfs::NWDSAddConnection($ctx, $conn);
		errorall($err);

		nwdsbufcheck($conn,$ctx);
		print 'NWDSFreeContext(ctx) = ';
		$err = ncpfs::NWDSFreeContext($ctx);
		errorall($err);
	}
}

sub nwdsdumpattr($$$) {
	my ($ctx,$buf,$type) = @_;
	my ($err,$cnt);
					
	print 'NWDSGetAttrCount(ctx,buf) = ';
	($err,$cnt) = ncpfs::NWDSGetAttrCount($ctx,$buf);
	if (errorinfo($err)) {
		print "ok, cnt=$cnt\n";
		while ($cnt > 0) {
			my ($nm,$vcnt,$synt);
			
			print 'NWDSGetAttrName(ctx,buf) = ';
			($err,$nm,$vcnt,$synt) = ncpfs::NWDSGetAttrName($ctx,$buf);
			if (errorinfo($err)) {
				print "ok, name=$nm, valcount=$vcnt, syntaxid=$synt\n";
				while ($vcnt > 0) {
					my ($sz);
					
					if ($type == 3 || $type == 4) {
						print "NWDSGetAttrValFlags(ctx,buf) = ";
						($err,$sz) = ncpfs::NWDSGetAttrValFlags($ctx, $buf);
						if (errorinfo($err)) {
							printf "ok, 0x%08X\n", $sz;
						}
						print "NWDSGetAttrValModTime(ctx,buf) = ";
						($err,$sz) = ncpfs::NWDSGetAttrValModTime($ctx, $buf);
						if (errorinfo($err)) {
							print "ok, $sz\n";
							dumpInside('  ', $sz);
						}
					}
					if ($type == 4) {
						print "Attribute size in NDS = ";
						($err,$sz) = ncpfs::NWDSGetAttrCount($ctx,$buf);
						if (errorinfo($err)) {
							print "ok, $sz bytes\n";
						}
					} else {
						print "NWDSComputeAttrValSize(ctx,buf,$synt) = ";
						($err,$sz) = ncpfs::NWDSComputeAttrValSize($ctx,$buf,$synt);
						if (errorinfo($err)) {
							my ($val);
							print "$sz bytes, retrieving value... ";
							($err, $val) = ncpfs::NWDSGetAttrVal($ctx,$buf,$synt);
							if (errorinfo($err)) {
								printf "ok, value=%s\n", cnvtValue($val);
								dumpInside('  ', $val);
							}
						}
					}
					$vcnt--;
				}
			}
			$cnt--;
		}
	}
}

sub nwdsdumpone($$) {
	my ($ctx,$buf) = @_;
	my ($err,$nm,$attrs,$oit);
					
	print 'NWDSGetObjectName(ctx,buf) = ';
	($err,$nm,$attrs,$oit) = ncpfs::NWDSGetObjectName($ctx,$buf);
	if (errorinfo($err)) {
		print "ok, name=$nm, attrs=$attrs\n";
		print "  oit=$oit\n";
		dumpInside('    ', $oit);
		if ($attrs != 0) {
			print "  attributes:\n";
			print "<FIXME>\n";
		}
	}
}

sub nwdsdumpbuf($$) {
	my ($ctx,$buf) = @_;
	my ($err,$cnt);
	
	print 'NWDSGetObjectCount(ctx,buf) = ';
	($err,$cnt) = ncpfs::NWDSGetObjectCount($ctx,$buf);
	if (errorinfo($err)) {
		print "ok, $cnt objects\n";
		while ($cnt > 0) {
			nwdsdumpone($ctx,$buf);
			$cnt--;
		}
	}
}

sub nwdslist($$) {
	my ($ctx,$base) = @_;
	my ($err,$iter,$buf);
	
	$iter = $ncpfs::NO_MORE_ITERATIONS;
	while (1) {
		printf 'NWDSList(ctx,"%s",\\%d) = ', $base, $iter;
		($err,$buf) = ncpfs::NWDSList($ctx, $base, \$iter);
		if (errorinfo($err)) {
			my ($cnt);
			
			print "ok, buf=$buf, iter=$iter\n";
			nwdsdumpbuf($ctx,$buf);
			if ($iter == $ncpfs::NO_MORE_ITERATIONS) {
				last;
			}
			next;
		}
		last;
	}
}

sub nwdsread($$$) {
	my ($ctx,$base,$type) = @_;
	my ($err,$iter,$buf);
	
	$iter = $ncpfs::NO_MORE_ITERATIONS;
	while (1) {
		printf 'NWDSRead(ctx,"%s",%u,1,undef,\\%d) = ', $base, $type, $iter;
		($err,$buf) = ncpfs::NWDSRead($ctx, $base, $type, 1, undef, \$iter);
		if (errorinfo($err)) {
			my ($cnt);
			
			print "ok, buf=$buf, iter=$iter\n";
			nwdsdumpattr($ctx,$buf,$type);
			if ($iter == $ncpfs::NO_MORE_ITERATIONS) {
				last;
			}
			next;
		}
		last;
	}
}

sub nwdsnames($) {
	my ($conn) = @_;
	my ($ctx,$err,$out,$a,$b,$c,$d,$e,$f,$buf);
	
	$ctx = new ncpfs::NWDSContextHandle;
	if (not defined($ctx)) {
		print "Cannot get context handle\n";
		return;
	}
	$err = ncpfs::NWDSAddConnection($ctx, $conn);
	if ($err) {
		printf "Cannot add connection to context: %s\n", ncpfs::strnwerror($err);
		return;
	}
	print 'NWDSGetBinderyContext(ctx, conn) = ';
	($err,$out) = ncpfs::NWDSGetBinderyContext($ctx, $conn);
	if (errorinfo($err)) {
		printf "ok, %s\n", $out;
	}
	print 'NWDSRemoveAllTypes(ctx, "CN=Test.OU=VANA\.C\=CZ.C=CZ") = ';
	($err,$out) = ncpfs::NWDSRemoveAllTypes($ctx, 'CN=Test.OU=VANA\.C\=CZ.C=CZ');
	if (errorinfo($err)) {
		printf "ok, %s\n", $out;
	}
	print 'NWDSCanonicalizeName(ctx, "Test.VANA\.C\=CZ.CZ") = ';
	($err,$out) = ncpfs::NWDSCanonicalizeName($ctx, 'Test.VANA\.C\=CZ.CZ');
	if (errorinfo($err)) {
		printf "ok, %s\n", $out;
	}
	print 'NWDSAbbreviateName(ctx, "CN=Test.OU=VANA\.C\=CZ.C=CZ") = ';
	($err,$out) = ncpfs::NWDSAbbreviateName($ctx, 'CN=Test.OU=VANA\.C\=CZ.C=CZ');
	if (errorinfo($err)) {
		printf "ok, %s\n", $out;
	}
	print 'NWIsDSServer(conn) = ';
	($err,$out) = ncpfs::NWIsDSServer($conn);
	if ($err) {
		print "yes, $out\n";
	} else {
		print "no\n";
	}
	print 'NWGetFileServerUTCTime(conn) = ';
	($err,$out) = ncpfs::NWGetFileServerUTCTime($conn);
	if (errorinfo($err)) {
		print "ok, $out\n";
	}
	print '__NWGetFileServerUTCTime(conn) = ';
	($err,$out,$a,$b,$c,$d,$e,$f) = ncpfs::__NWGetFileServerUTCTime($conn);
	if (errorinfo($err)) {
		printf "ok, %08X.%08X, %08X, %08X %08X %08X %08X\n", $out, $a, $b, $c, $d, $e, $f;
	}
	print 'NWDSGetServerDN(ctx,conn) = ';
	($err,$out) = ncpfs::NWDSGetServerDN($ctx, $conn);
	if (errorinfo($err)) {
		print "ok, $out\n";
		printf 'NWDSMapNameToID(ctx,conn,"%s") = ', $out;
		($err,$a) = ncpfs::NWDSMapNameToID($ctx, $conn, $out);
		if (errorinfo($err)) {
			printf "ok, 0x%08X\n", $a;
			printf 'NWDSMapIDToName(ctx,conn,0x%08X) = ', $a;
			($err,$b) = ncpfs::NWDSMapIDToName($ctx, $conn, $a);
			if (errorinfo($err)) {
				printf "ok, $b\n";
			}
		}
		printf 'NWDSResolveName(ctx,"%s") = ', $out;
		($err,$a,$b) = ncpfs::NWDSResolveName($ctx, $out);
		if (errorinfo($err)) {
			printf "ok, conn=$a, ID=0x%08X\n", $b;
			ncpfs::NWCCCloseConn($a);
		}
		printf 'NWDSReadObjectInfo(ctx,"%s") = ', $out;
		($err,$a,$b) = ncpfs::NWDSReadObjectInfo($ctx, $out);
		if (errorinfo($err)) {
			print "ok, name=$a, info=$b\n";
			dumpInside('  ', $b);
		}
		printf 'NWDSOpenConnToNDSServer(ctx,"%s") = ', $out;
		($err,$a) = ncpfs::NWDSOpenConnToNDSServer($ctx, $out);
		if (errorinfo($err)) {
			print "ok, conn=$a\n";
			ncpfs::NWCCCloseConn($a);
		}
		printf 'NWDSGetNDSStatistics(ctx,"%s") = ', $out;
		($err,$a) = ncpfs::NWDSGetNDSStatistics($ctx, $out);
		if (errorinfo($err)) {
			print "ok, stats=$a\n";
			dumpInside('  ', $a);
		}
	}
	print 'NWDSGetServerAddress(ctx,conn) = ';
	($err,$out,$buf) = ncpfs::NWDSGetServerAddress($ctx, $conn);
	if (errorinfo($err)) {
		print "ok, $out addresses, buffer $buf\n";
	}
	print 'NWDSGetDSVerInfo(conn) = ';
	($err,$a,$b,$c,$d,$e) = ncpfs::NWDSGetDSVerInfo($conn);
	if (errorinfo($err)) {
		print "ok, DSVersion=$a, Depth=$b, Name=$c, Flags=$d, WName=$e\n";
	}
	printf 'NWDSGetObjectHostServerAddress(ctx,"%s") = ', $nds_volume;
	($err,$a,$b) = ncpfs::NWDSGetObjectHostServerAddress($ctx, $nds_volume);
	if (errorinfo($err)) {
		print "ok, Server=$a, Addresses=$b\n";
	}
	nwdslist($ctx, '[Root]');
	nwdsread($ctx, '[Root]', 0);
	nwdsread($ctx, '[Root]', 1);
	nwdsread($ctx, '[Root]', 2);
	nwdsread($ctx, '[Root]', 3);
	nwdsread($ctx, '[Root]', 4);
}

sub nwdsreload($) {
	my ($conn) = @_;
	my ($ctx,$err,$out);
	
	$ctx = new ncpfs::NWDSContextHandle;
	if (not defined($ctx)) {
		print "Cannot get context handle\n";
		return;
	}
	$err = ncpfs::NWDSAddConnection($ctx, $conn);
	if ($err) {
		printf "Cannot add connection to context: %s\n", ncpfs::strnwerror($err);
		return;
	}
	($err,$out) = ncpfs::NWDSGetServerDN($ctx, $conn);
	if (errorinfo($err)) {
		printf 'NWDSReloadDS(ctx, "%s") = ', $out;
		$err = ncpfs::NWDSReloadDS($ctx, $out);
		errorall($err);
	}
}

sub retry($) {
	my ($err) = @_;

	if ($err == -637 || $err == -654) {
		print 'Sleeping...';
		sleep(5);
		print 'Retrying...';
		return 1;
	}
	return 0;
}

sub nwdscreate($) {
	my ($conn) = @_;
	my ($ctx,$err,$srvname,$out,$a,$b,$c,$d,$e,$f,$buf);
	my ($test_x,$test_ou2,$test_rdn,$test_user,$newtest_rdn);
	
	$test_x = "O=NCPFS_TEST.$test_base";
	$test_ou2 = "OU=Inside.$test_x";
	$test_rdn = "CN=Test";
	$test_user = "$test_rdn.$test_x";
	$newtest_rdn = "CN=NewTest";
	
	$ctx = new ncpfs::NWDSContextHandle;
	if (not defined($ctx)) {
		print "Cannot get context handle\n";
		return;
	}
	$err = ncpfs::NWDSAddConnection($ctx, $conn);
	if ($err) {
		printf "Cannot add connection to context: %s\n", ncpfs::strnwerror($err);
		return;
	}
	($err,$srvname) = ncpfs::NWDSGetServerDN($ctx, $conn);
	if ($err) {
		printf "Cannot retrieve server name: %s\n", ncpfs::strnwerror($err);
		return;
	}
	print "Allocating and initializing new buffer... ";
	($err,$buf) = ncpfs::NWDSInitBuf($ctx, $ncpfs::DSV_ADD_ENTRY);
	errorall($err);
	if ($err) {
		return;
	}
	print 'NWDSPutAttrNameAndVal(ctx, buf, "Object Class", $ncpfs::SYN_CLASS_NAME, "Organization") = ';
	$err = ncpfs::NWDSPutAttrNameAndVal($ctx, $buf, "Object Class", $ncpfs::SYN_CLASS_NAME, "Organization");
	errorall($err);
	printf 'NWDSAddObject(ctx, "%s", undef, 0, buf) = ', $test_x;
	$err = ncpfs::NWDSAddObject($ctx, $test_x, undef, 0, $buf);
	errorall($err);
	print "Allocating and initializing new buffer... ";
	($err,$buf) = ncpfs::NWDSInitBuf($ctx, $ncpfs::DSV_ADD_ENTRY);
	errorall($err);
	if ($err) {
		return;
	}
	print 'NWDSPutAttrNameAndVal(ctx, buf, "Object Class", $ncpfs::SYN_CLASS_NAME, "Organizational Unit") = ';
	$err = ncpfs::NWDSPutAttrNameAndVal($ctx, $buf, "Object Class", $ncpfs::SYN_CLASS_NAME, "Organizational Unit");
	errorall($err);
	printf 'NWDSAddObject(ctx, "%s", undef, 0, buf) = ', $test_ou2;
	$err = ncpfs::NWDSAddObject($ctx, $test_ou2, undef, 0, $buf);
	errorall($err);
	printf 'NWDSRemoveObject(ctx, "%s") = ', $newtest_rdn.'.'.$test_x;
	$err = ncpfs::NWDSRemoveObject($ctx, $newtest_rdn.'.'.$test_x);
	errorall($err);
	print "Allocating and initializing new buffer... ";
	($err,$buf) = ncpfs::NWDSInitBuf($ctx, $ncpfs::DSV_ADD_ENTRY);
	errorall($err);
	if ($err) {
		return;
	}
	print 'NWDSPutAttrName(ctx, buf, "Object Class") = ';
	$err = ncpfs::NWDSPutAttrName($ctx, $buf, "Object Class");
	errorall($err);
	print 'NWDSPutAttrVal(ctx, buf, $ncpfs::SYN_CLASS_NAME, "User") = ';
	$err = ncpfs::NWDSPutAttrVal($ctx, $buf, $ncpfs::SYN_CLASS_NAME, "User");
	errorall($err);
	print 'NWDSPutAttrNameAndVal(ctx, buf, "Surname", $ncpfs::SYN_CI_STRING, "NCPFS Test User") = ';
	$err = ncpfs::NWDSPutAttrNameAndVal($ctx, $buf, "Surname", $ncpfs::SYN_CI_STRING, "NCPFS Test User");
	errorall($err);
	printf 'NWDSAddObject(ctx, "%s", undef, 0, buf) = ', $test_user;
	$err = ncpfs::NWDSAddObject($ctx, $test_user, undef, 0, $buf);
	errorall($err);

	printf 'NWDSRemoveObject(ctx, "%s") = ', $newtest_rdn.'.'.$test_x;
	$err = ncpfs::NWDSRemoveObject($ctx, $newtest_rdn.'.'.$test_x);
	errorall($err);
	
	if (0) {
		# Do this only on special request... it is slow...
		printf 'NWDSModifyRDN(ctx, "%s", "%s", 1) = ', $test_user, $newtest_rdn;
		$err = ncpfs::NWDSModifyRDN($ctx, $test_user, $newtest_rdn, 1);
		errorall($err);
		printf 'NWDSMoveObject(ctx, "%s", "%s", "%s") = ', $newtest_rdn.'.'.$test_x, $test_ou2, $test_rdn;
		do {
			$err = ncpfs::NWDSMoveObject($ctx, $newtest_rdn.'.'.$test_x, $test_ou2, $test_rdn);
			errorall($err);
		} while (retry($err));
	}
	print 'Allocating and initializing new buffer... ';
	($err,$buf) = ncpfs::NWDSInitBuf($ctx, $ncpfs::DSV_MODIFY_ENTRY);
	errorall($err);
	if ($err) {
		return;
	}
	print 'NWDSPutChangeAndVal(ctx, buf, $ncpfs::DS_OVERWRITE_VALUE, "Surname", $ncpfs::SYN_CI_STRING, "Real Test User") = ';
	$err = ncpfs::NWDSPutChangeAndVal($ctx, $buf, $ncpfs::DS_OVERWRITE_VALUE, "Surname", $ncpfs::SYN_CI_STRING, "Real Test User");
	errorall($err);
	print 'NWDSPutChange(ctx, buf, $ncpfs::DS_ADD_ATTRIBUTE, "Given Name") = ';
	$err = ncpfs::NWDSPutChange($ctx, $buf, $ncpfs::DS_ADD_ATTRIBUTE, "Given Name");
	errorall($err);
	print 'NWDSPutAttrVal(ctx, buf, $ncpfs::SYN_CI_STRING, "NCPFS") = ';
	$err = ncpfs::NWDSPutAttrVal($ctx, $buf, $ncpfs::SYN_CI_STRING, "NCPFS");
	errorall($err);
	if (0) {
		printf 'NWDSModifyObject(ctx, "%s", undef, 0, buf) = ', $test_rdn.'.'.$test_ou2;
		$err = ncpfs::NWDSModifyObject($ctx, $test_rdn.'.'.$test_ou2, undef, 0, $buf);
		errorall($err);
		printf 'NWDSModifyDN(ctx, "%s", "%s", 1) = ', $test_rdn.'.'.$test_ou2, $test_user;
		do {
			$err = ncpfs::NWDSModifyDN($ctx, $test_rdn.'.'.$test_ou2, $test_user, 1);
			errorall($err);
		} while (retry($err));
	} else {
		printf 'NWDSModifyObject(ctx, "%s", undef, 0, buf) = ', $test_user;
		$err = ncpfs::NWDSModifyObject($ctx, $test_user, undef, 0, $buf);
		errorall($err);
	}
	printf 'NWDSGetPartitionRoot(ctx, "%s") = ', $test_user;
	($err, $out) = ncpfs::NWDSGetPartitionRoot($ctx, $test_user);
	if (errorinfo($err)) {
		print "ok, partition root=$out\n";
	}
	printf 'NWDSSplitPartition(ctx, "%s", 0) = ', $test_x;
	do {
		$err = ncpfs::NWDSSplitPartition($ctx, $test_x, 0);
		errorall($err);
	} while (retry($err));
	{
		my ($i);
		
		print "Waiting for partition split...\n";
		for ($i = 0; $i < 20; $i++) {
			printf 'NWDSGetPartitionRoot(ctx, "%s") = ', $test_user;
			($err, $out) = ncpfs::NWDSGetPartitionRoot($ctx, $test_user);
			if (errorinfo($err)) {
				print "ok, partition root=$out\n";
				if ($out eq $test_x) {
					last;
				}
				sleep 5;
			}
		}
	}
	my ($i) = (0);
	
	printf 'NWDSJoinPartitions(ctx, "%s", 0) = ', $test_x;
	do {
		$err = ncpfs::NWDSJoinPartitions($ctx, $test_x, 0);
		errorall($err);
		if ($err == -672) {
			print 'Whoa... strange...';
			$i++;
			if ($i < 20) {
				$err = -637;
			}
		}
	} while (retry($err));
	{
		my ($i);
		
		print "Waiting for partition merge...\n";
		for ($i = 0; $i < 20; $i++) {
			printf 'NWDSGetPartitionRoot(ctx, "%s") = ', $test_user;
			($err, $out) = ncpfs::NWDSGetPartitionRoot($ctx, $test_user);
			if (errorinfo($err)) {
				print "ok, partition root=$out\n";
				if ($out ne $test_x) {
					last;
				}
				sleep 5;
			}
		}
	}
	printf 'NWDSRemoveObject(ctx, "%s") = ', $test_ou2;
	do {
		$err = ncpfs::NWDSRemoveObject($ctx, $test_ou2);
		errorall($err);
	} while (retry($err));
	printf 'NWDSRemoveObject(ctx, "%s") = ', $test_user;
	$err = ncpfs::NWDSRemoveObject($ctx, $test_user);
	errorall($err);
	printf 'NWDSRemoveObject(ctx, "%s") = ', $test_x;
	$err = ncpfs::NWDSRemoveObject($ctx, $test_x);
	errorall($err);
}

$|=1;

my $x = ncpfs::ncp_conn_spec->new();

print "W=$x, Undef=".defined($x)."\n";

$x->{'server'} = $server_name;

print $x->{'server'}."\n";
#print $x->{'u'};

my ($m) = ncpfs::ncp_find_permanent($x);
print "Z=".(defined($m)?$m:'<undef>')."\n";

if (0) {
  my $z;

  ($x,$z) = ncpfs::ncp_find_conn_spec('NONE', 'AA', 'BB', 1, ~0);
  print "X=$x, Z=$z\n";
  dumpInside('  ', $x);
}

my ($err, $conn) = ncpfs::ncp_open_mount($m);

if ($err) {
	print 'Cannot open connection: '.ncpfs::strnwerror($err)."\n";
	exit;
}

#gsinfo($conn);
#dumpBindery($conn,-1,'*');
if (0) {
	my $z;

	$z = getTime($conn);
	setTime($conn, $z + 10);
	getTime($conn);
	setTime($conn, $z);
}
#checkNWpath();
#getTime($conn);
#getConnList($conn, $test_objecttype, $test_objectname);
#getStationInfo($conn, 1, 100);
#getInternetAddress($conn, 1, 100);
#sendMessage1($conn, [7,9,10], 'Test Perl::ncpfs #1');
#sendMessage2($conn, 7, 'Test Perl::ncpfs #2');
#getEncryptionKey($conn);
#getBinderyObjectId($conn, $test_objecttype, $test_objectname);
#getBinderyObjectName($conn, 0x00000001);
#demoBindery($conn, 1, 'HOKUSPOKUS_DELETE_ME_I\'M_OF_NO_VALUE');
#readProperty($conn, $test_objecttype, $test_objectname, 'ACL');
#scanProperties($conn, $test_objecttype, $test_objectname, '*');
#loginEncrypted($conn, $test_objecttype, $test_objectname, 'TEST');
#loginUnencrypted($conn, $test_objecttype, $test_objectname, 'TEST');
#changePasswordEncrypted($conn, $test_objecttype, $test_objectname, 'QTEST', 'TEST');
#loginUser($conn, $test_objectname, 'TEST');
#getVolInfos($conn);
#getVolumeNumber($conn, 'SYS');
#getVolumeNumber($conn, 'UNKNOWN');
#dodir($conn, 0, 'SYS:', '*');
#getfinfo($conn, 0, 'SYS:', 'VOL$LOG.ERR');
#demoFile($conn, 0, 'SYS:NCPFSPRL.TMP');
#file_subdir_info($conn);
#queueTest($conn);
#file2_demo($conn);
#connInfo($conn);
#vollist($conn, 0);
#vollist($conn, 2);
#convdemo();
#newnsdemo($conn);
#namespacedemo($conn);
#ns3demo($conn);
#fidinfo($conn);
#nwcallsdemo($conn);
#nwbindry($conn);
#nwrpc($conn);
#nwsmset($conn);
#nwconnsop($conn);
#nwncpext($conn);
#nwconnopen($conn);
#nwdiskrest($conn);
#nwsema($conn);
#nwea($conn, 'SYS:LOGIN/NLS', 'SYS:LOGIN/NLS/ENGLISH');
#nwdsctxcheck($conn);
#nwdsnames($conn);
#nwdsreload($conn);
nwdscreate($conn);
##$err = ncpfs::NWLogoutFromFileServer($conn);
##printf "Logout error code: %s\n", ncpfs::strnwerror($err);
$err = ncpfs::ncp_close($conn);
print 'Close: '.ncpfs::strnwerror($err)."\n";
