Issue:
======
 When database in forced writes mode grows rapidly, filesystem
 disk operations produce a serious overhead, making performance
 up to 3 times lower compared with turned off forced writes.

Scope:
======
 Mainly affects Linux, because Linux misses appropriate system
 call to make database file grow efficiently.

Document author:
=================
 Alex Peshkov (peshkoff@mail.ru)

Document date:  2007/11/21
==============


 To make firebird have better performance under such circumstances
 you may place your database not in a regular file on some
 filesystem, but on raw device. Any type of block device is
 supported.

 For example:

 gbak -c my.fbk /dev/sda7

 will restore your database on the third logical of extended
 partition of your SCSI(SATA) disk0.

 Known issue:
 To be able to do physical (using nbackup utility) copy of
 database you MUST specify explicit name of difference file:

# isql /dev/sda7
SQL> alter database add difference file '/tmp/dev_sda7';

 This is required because default location of difference file
 will be in /dev, which is surely not what you need. It's also
 better to know how many blocks on block device are actually
 occupied (or you will have to copy all data on raw device,
 which can make size of your copy abnormally large). To obtain
 real size of database, you should use '-S' switch of nbackup:

# nbackup -s -l /dev/sda7
77173

 Where 77173 is a number of pages, occupied by database. Take
 care - this is database's page size, not disk physical block
 size! If unsure, use

# gstat -h /dev/sda7
Database "/dev/sda7"
Database header page information:
        Flags                   0
        Checksum                12345
        Generation              43
        Page size               4096  <== that's what you need
        ODS version             11.1
 . . . . . . .

 You may use nbackup output directly in a script, performing
 database backup:

# DbFile=/dev/sda7
# DbSize=`nbackup -L $DbFile -S` || exit 1
# dd if=$DbFile ibs=4k count=$DbSize | # compress and record DVD
# nbackup -N $DbFile

 Or perform physical backup using nbackup:

# nbackup -B 0 /dev/sda7 /tmp/lvl.0

 In all other aspects raw devices do not have known specific
 in use.
 Tip: it's good idea to have raw devices in databases.conf - in
 case of HW reconfiguration of your server you will not need to
 change connection strings.
