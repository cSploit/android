# $Id$
#
# Take a comma-separated list of database names and spit out all the
# log records that affect those databases. 
# Note: this will not start printing records until a dbreg_register
#	record for that file is found.

NR == 1 {
	nfiles = 0
	while ((ndx = index(DBNAME, ",")) != 0) {
		filenames[nfiles] = substr(DBNAME, 1, ndx - 1) "\\0";
		DBNAME = substr(DBNAME, ndx + 1, length(DBNAME) - ndx);
		files[nfiles] = -1
		nfiles++
	}
	filenames[nfiles] = DBNAME "\\0";
	files[nfiles] = -1
	myfile = -1;
	nreg = 0;
}

/^\[.*dbreg_register/ {
	register = 1;
}
/opcode:/ {
	if (register == 1) {
		if ($2 == "CHKPNT" || $2 == "OPEN" || $2 == "PREOPEN" || $2 == "REOPEN")
			register = 3;
	}
}
/name:/ {
	if (register >= 2) {
		myfile = -2;
		for (i = 0; i <= nfiles; i++) {
			if ($5 == filenames[i]) {
				if (register == 2) {
					printme = 0;
					myfile = -2;
				} else {
					myfile = i;
				}
				break;
			}
		}
	}
	register = 0;
}
/fileid:/{
	if (myfile == -2)
		files[$2] = 0;
	else if (myfile != -1) {
		files[$2] = 1;
		if ($2 > nreg)
			nreg = $2;
		printme = 1;
		register = 0;
		myfile = -1;
	} else if ($2 <= nreg && files[$2] == 1) {
		printme = 1
	}
	myfile = -1;
}

/^\[/{
	if (printme == 1) {
		printf("%s\n", rec);
		printme = 0
	}
	rec = "";

	rec = $0
}

TXN == 1 && /txn_regop/ {printme = 1}
/^	/{
	if (length(rec) + length($0) < 2040)
		rec = sprintf("%s\n%s", rec, $0);
}

END {
	if (printme == 1)
		printf("%s\n", rec);
}
