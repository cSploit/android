# Run this TCL script using "testfixture" in order get a report that shows
# how much disk space is used by a particular data to actually store data
# versus how much space is unused.
#

if {[catch {
# Get the name of the database to analyze
#
proc usage {} {
  set argv0 [file rootname [file tail [info nameofexecutable]]]
  puts stderr "Usage: $argv0 database-name"
  exit 1
}
set file_to_analyze {}
set flags(-pageinfo) 0
set flags(-stats) 0
append argv {}
foreach arg $argv {
  if {[regexp {^-+pageinfo$} $arg]} {
    set flags(-pageinfo) 1
  } elseif {[regexp {^-+stats$} $arg]} {
    set flags(-stats) 1
  } elseif {[regexp {^-} $arg]} {
    puts stderr "Unknown option: $arg"
    usage
  } elseif {$file_to_analyze!=""} {
    usage
  } else {
    set file_to_analyze $arg
  }
}
if {$file_to_analyze==""} usage
set root_filename $file_to_analyze
regexp {^file:(//)?([^?]*)} $file_to_analyze all x1 root_filename
if {![file exists $root_filename]} {
  puts stderr "No such file: $root_filename"
  exit 1
}
if {![file readable $root_filename]} {
  puts stderr "File is not readable: $root_filename"
  exit 1
}
set true_file_size [file size $root_filename]
if {$true_file_size<512} {
  puts stderr "Empty or malformed database: $root_filename"
  exit 1
}

# Compute the total file size assuming test_multiplexor is being used.
# Assume that SQLITE_ENABLE_8_3_NAMES might be enabled
#
set extension [file extension $root_filename]
set pattern $root_filename
append pattern {[0-3][0-9][0-9]}
foreach f [glob -nocomplain $pattern] {
  incr true_file_size [file size $f]
  set extension {}
}
if {[string length $extension]>=2 && [string length $extension]<=4} {
  set pattern [file rootname $root_filename]
  append pattern {.[0-3][0-9][0-9]}
  foreach f [glob -nocomplain $pattern] {
    incr true_file_size [file size $f]
  }
}

# Open the database
#
if {[catch {sqlite3 db $file_to_analyze -uri 1} msg]} {
  puts stderr "error trying to open $file_to_analyze: $msg"
  exit 1
}
register_dbstat_vtab db

db eval {SELECT count(*) FROM sqlite_master}
set pageSize [expr {wide([db one {PRAGMA page_size}])}]

if {$flags(-pageinfo)} {
  db eval {CREATE VIRTUAL TABLE temp.stat USING dbstat}
  db eval {SELECT name, path, pageno FROM temp.stat ORDER BY pageno} {
    puts "$pageno $name $path"
  }
  exit 0
}
if {$flags(-stats)} {
  db eval {CREATE VIRTUAL TABLE temp.stat USING dbstat}
  puts "BEGIN;"
  puts "CREATE TABLE stats("
  puts "  name       STRING,           /* Name of table or index */"
  puts "  path       INTEGER,          /* Path to page from root */"
  puts "  pageno     INTEGER,          /* Page number */"
  puts "  pagetype   STRING,           /* 'internal', 'leaf' or 'overflow' */"
  puts "  ncell      INTEGER,          /* Cells on page (0 for overflow) */"
  puts "  payload    INTEGER,          /* Bytes of payload on this page */"
  puts "  unused     INTEGER,          /* Bytes of unused space on this page */"
  puts "  mx_payload INTEGER,          /* Largest payload size of all cells */"
  puts "  pgoffset   INTEGER,          /* Offset of page in file */"
  puts "  pgsize     INTEGER           /* Size of the page */"
  puts ");"
  db eval {SELECT quote(name) || ',' ||
                  quote(path) || ',' ||
                  quote(pageno) || ',' ||
                  quote(pagetype) || ',' ||
                  quote(ncell) || ',' ||
                  quote(payload) || ',' ||
                  quote(unused) || ',' ||
                  quote(mx_payload) || ',' ||
                  quote(pgoffset) || ',' ||
                  quote(pgsize) AS x FROM stat} {
    puts "INSERT INTO stats VALUES($x);"
  }
  puts "COMMIT;"
  exit 0
}

# In-memory database for collecting statistics. This script loops through
# the tables and indices in the database being analyzed, adding a row for each
# to an in-memory database (for which the schema is shown below). It then
# queries the in-memory db to produce the space-analysis report.
#
sqlite3 mem :memory:
set tabledef {CREATE TABLE space_used(
   name clob,        -- Name of a table or index in the database file
   tblname clob,     -- Name of associated table
   is_index boolean, -- TRUE if it is an index, false for a table
   nentry int,       -- Number of entries in the BTree
   leaf_entries int, -- Number of leaf entries
   payload int,      -- Total amount of data stored in this table or index
   ovfl_payload int, -- Total amount of data stored on overflow pages
   ovfl_cnt int,     -- Number of entries that use overflow
   mx_payload int,   -- Maximum payload size
   int_pages int,    -- Number of interior pages used
   leaf_pages int,   -- Number of leaf pages used
   ovfl_pages int,   -- Number of overflow pages used
   int_unused int,   -- Number of unused bytes on interior pages
   leaf_unused int,  -- Number of unused bytes on primary pages
   ovfl_unused int,  -- Number of unused bytes on overflow pages
   gap_cnt int,      -- Number of gaps in the page layout
   compressed_size int  -- Total bytes stored on disk
);}
mem eval $tabledef

# Create a temporary "dbstat" virtual table.
#
db eval {CREATE VIRTUAL TABLE temp.stat USING dbstat}
db eval {CREATE TEMP TABLE dbstat AS SELECT * FROM temp.stat
         ORDER BY name, path}
db eval {DROP TABLE temp.stat}

proc isleaf {pagetype is_index} {
  return [expr {$pagetype == "leaf" || ($pagetype == "internal" && $is_index)}]
}
proc isoverflow {pagetype is_index} {
  return [expr {$pagetype == "overflow"}]
}
proc isinternal {pagetype is_index} {
  return [expr {$pagetype == "internal" && $is_index==0}]
}

db func isleaf isleaf
db func isinternal isinternal
db func isoverflow isoverflow

set isCompressed 0
set compressOverhead 0
set sql { SELECT name, tbl_name FROM sqlite_master WHERE rootpage>0 }
foreach {name tblname} [concat sqlite_master sqlite_master [db eval $sql]] {

  set is_index [expr {$name!=$tblname}]
  db eval {
    SELECT 
      sum(ncell) AS nentry,
      sum(isleaf(pagetype, $is_index) * ncell) AS leaf_entries,
      sum(payload) AS payload,
      sum(isoverflow(pagetype, $is_index) * payload) AS ovfl_payload,
      sum(path LIKE '%+000000') AS ovfl_cnt,
      max(mx_payload) AS mx_payload,
      sum(isinternal(pagetype, $is_index)) AS int_pages,
      sum(isleaf(pagetype, $is_index)) AS leaf_pages,
      sum(isoverflow(pagetype, $is_index)) AS ovfl_pages,
      sum(isinternal(pagetype, $is_index) * unused) AS int_unused,
      sum(isleaf(pagetype, $is_index) * unused) AS leaf_unused,
      sum(isoverflow(pagetype, $is_index) * unused) AS ovfl_unused,
      sum(pgsize) AS compressed_size
    FROM temp.dbstat WHERE name = $name
  } break

  set total_pages [expr {$leaf_pages+$int_pages+$ovfl_pages}]
  set storage [expr {$total_pages*$pageSize}]
  if {!$isCompressed && $storage>$compressed_size} {
    set isCompressed 1
    set compressOverhead 14
  }

  # Column 'gap_cnt' is set to the number of non-contiguous entries in the
  # list of pages visited if the b-tree structure is traversed in a top-down
  # fashion (each node visited before its child-tree is passed). Any overflow
  # chains present are traversed from start to finish before any child-tree
  # is.
  #
  set gap_cnt 0
  set pglist [db eval {
    SELECT pageno FROM temp.dbstat WHERE name = $name ORDER BY rowid
  }]
  set prev [lindex $pglist 0]
  foreach pgno [lrange $pglist 1 end] {
    if {$pgno != $prev+1} {incr gap_cnt}
    set prev $pgno
  }

  mem eval {
    INSERT INTO space_used VALUES(
      $name,
      $tblname,
      $is_index,
      $nentry,
      $leaf_entries,
      $payload,     
      $ovfl_payload,
      $ovfl_cnt,   
      $mx_payload,
      $int_pages,
      $leaf_pages,  
      $ovfl_pages, 
      $int_unused, 
      $leaf_unused,
      $ovfl_unused,
      $gap_cnt,
      $compressed_size
    );
  }
}

proc integerify {real} {
  if {[string is double -strict $real]} {
    return [expr {wide($real)}]
  } else {
    return 0
  }
}
mem function int integerify

# Quote a string for use in an SQL query. Examples:
#
# [quote {hello world}]   == {'hello world'}
# [quote {hello world's}] == {'hello world''s'}
#
proc quote {txt} {
  regsub -all ' $txt '' q
  return '$q'
}

# Generate a single line of output in the statistics section of the
# report.
#
proc statline {title value {extra {}}} {
  set len [string length $title]
  set dots [string range {......................................} $len end]
  set len [string length $value]
  set sp2 [string range {          } $len end]
  if {$extra ne ""} {
    set extra " $extra"
  }
  puts "$title$dots $value$sp2$extra"
}

# Generate a formatted percentage value for $num/$denom
#
proc percent {num denom {of {}}} {
  if {$denom==0.0} {return ""}
  set v [expr {$num*100.0/$denom}]
  set of {}
  if {$v==100.0 || $v<0.001 || ($v>1.0 && $v<99.0)} {
    return [format {%5.1f%% %s} $v $of]
  } elseif {$v<0.1 || $v>99.9} {
    return [format {%7.3f%% %s} $v $of]
  } else {
    return [format {%6.2f%% %s} $v $of]
  }
}

proc divide {num denom} {
  if {$denom==0} {return 0.0}
  return [format %.2f [expr double($num)/double($denom)]]
}

# Generate a subreport that covers some subset of the database.
# the $where clause determines which subset to analyze.
#
proc subreport {title where} {
  global pageSize file_pgcnt compressOverhead

  # Query the in-memory database for the sum of various statistics 
  # for the subset of tables/indices identified by the WHERE clause in
  # $where. Note that even if the WHERE clause matches no rows, the
  # following query returns exactly one row (because it is an aggregate).
  #
  # The results of the query are stored directly by SQLite into local 
  # variables (i.e. $nentry, $nleaf etc.).
  #
  mem eval "
    SELECT
      int(sum(nentry)) AS nentry,
      int(sum(leaf_entries)) AS nleaf,
      int(sum(payload)) AS payload,
      int(sum(ovfl_payload)) AS ovfl_payload,
      max(mx_payload) AS mx_payload,
      int(sum(ovfl_cnt)) as ovfl_cnt,
      int(sum(leaf_pages)) AS leaf_pages,
      int(sum(int_pages)) AS int_pages,
      int(sum(ovfl_pages)) AS ovfl_pages,
      int(sum(leaf_unused)) AS leaf_unused,
      int(sum(int_unused)) AS int_unused,
      int(sum(ovfl_unused)) AS ovfl_unused,
      int(sum(gap_cnt)) AS gap_cnt,
      int(sum(compressed_size)) AS compressed_size
    FROM space_used WHERE $where" {} {}

  # Output the sub-report title, nicely decorated with * characters.
  #
  puts ""
  set len [string length $title]
  set stars [string repeat * [expr 65-$len]]
  puts "*** $title $stars"
  puts ""

  # Calculate statistics and store the results in TCL variables, as follows:
  #
  # total_pages: Database pages consumed.
  # total_pages_percent: Pages consumed as a percentage of the file.
  # storage: Bytes consumed.
  # payload_percent: Payload bytes used as a percentage of $storage.
  # total_unused: Unused bytes on pages.
  # avg_payload: Average payload per btree entry.
  # avg_fanout: Average fanout for internal pages.
  # avg_unused: Average unused bytes per btree entry.
  # ovfl_cnt_percent: Percentage of btree entries that use overflow pages.
  #
  set total_pages [expr {$leaf_pages+$int_pages+$ovfl_pages}]
  set total_pages_percent [percent $total_pages $file_pgcnt]
  set storage [expr {$total_pages*$pageSize}]
  set payload_percent [percent $payload $storage {of storage consumed}]
  set total_unused [expr {$ovfl_unused+$int_unused+$leaf_unused}]
  set avg_payload [divide $payload $nleaf]
  set avg_unused [divide $total_unused $nleaf]
  if {$int_pages>0} {
    # TODO: Is this formula correct?
    set nTab [mem eval "
      SELECT count(*) FROM (
          SELECT DISTINCT tblname FROM space_used WHERE $where AND is_index=0
      )
    "]
    set avg_fanout [mem eval "
      SELECT (sum(leaf_pages+int_pages)-$nTab)/sum(int_pages) FROM space_used
          WHERE $where AND is_index = 0
    "]
    set avg_fanout [format %.2f $avg_fanout]
  }
  set ovfl_cnt_percent [percent $ovfl_cnt $nleaf {of all entries}]

  # Print out the sub-report statistics.
  #
  statline {Percentage of total database} $total_pages_percent
  statline {Number of entries} $nleaf
  statline {Bytes of storage consumed} $storage
  if {$compressed_size!=$storage} {
    set compressed_size [expr {$compressed_size+$compressOverhead*$total_pages}]
    set pct [expr {$compressed_size*100.0/$storage}]
    set pct [format {%5.1f%%} $pct]
    statline {Bytes used after compression} $compressed_size $pct
  }
  statline {Bytes of payload} $payload $payload_percent
  statline {Average payload per entry} $avg_payload
  statline {Average unused bytes per entry} $avg_unused
  if {[info exists avg_fanout]} {
    statline {Average fanout} $avg_fanout
  }
  if {$total_pages>1} {
    set fragmentation [percent $gap_cnt [expr {$total_pages-1}] {fragmentation}]
    statline {Fragmentation} $fragmentation
  }
  statline {Maximum payload per entry} $mx_payload
  statline {Entries that use overflow} $ovfl_cnt $ovfl_cnt_percent
  if {$int_pages>0} {
    statline {Index pages used} $int_pages
  }
  statline {Primary pages used} $leaf_pages
  statline {Overflow pages used} $ovfl_pages
  statline {Total pages used} $total_pages
  if {$int_unused>0} {
    set int_unused_percent [
         percent $int_unused [expr {$int_pages*$pageSize}] {of index space}]
    statline "Unused bytes on index pages" $int_unused $int_unused_percent
  }
  statline "Unused bytes on primary pages" $leaf_unused [
     percent $leaf_unused [expr {$leaf_pages*$pageSize}] {of primary space}]
  statline "Unused bytes on overflow pages" $ovfl_unused [
     percent $ovfl_unused [expr {$ovfl_pages*$pageSize}] {of overflow space}]
  statline "Unused bytes on all pages" $total_unused [
               percent $total_unused $storage {of all space}]
  return 1
}

# Calculate the overhead in pages caused by auto-vacuum. 
#
# This procedure calculates and returns the number of pages used by the 
# auto-vacuum 'pointer-map'. If the database does not support auto-vacuum,
# then 0 is returned. The two arguments are the size of the database file in
# pages and the page size used by the database (in bytes).
proc autovacuum_overhead {filePages pageSize} {

  # Set $autovacuum to non-zero for databases that support auto-vacuum.
  set autovacuum [db one {PRAGMA auto_vacuum}]

  # If the database is not an auto-vacuum database or the file consists
  # of one page only then there is no overhead for auto-vacuum. Return zero.
  if {0==$autovacuum || $filePages==1} {
    return 0
  }

  # The number of entries on each pointer map page. The layout of the
  # database file is one pointer-map page, followed by $ptrsPerPage other
  # pages, followed by a pointer-map page etc. The first pointer-map page
  # is the second page of the file overall.
  set ptrsPerPage [expr double($pageSize/5)]

  # Return the number of pointer map pages in the database.
  return [expr wide(ceil( ($filePages-1.0)/($ptrsPerPage+1.0) ))]
}


# Calculate the summary statistics for the database and store the results
# in TCL variables. They are output below. Variables are as follows:
#
# pageSize:      Size of each page in bytes.
# file_bytes:    File size in bytes.
# file_pgcnt:    Number of pages in the file.
# file_pgcnt2:   Number of pages in the file (calculated).
# av_pgcnt:      Pages consumed by the auto-vacuum pointer-map.
# av_percent:    Percentage of the file consumed by auto-vacuum pointer-map.
# inuse_pgcnt:   Data pages in the file.
# inuse_percent: Percentage of pages used to store data.
# free_pgcnt:    Free pages calculated as (<total pages> - <in-use pages>)
# free_pgcnt2:   Free pages in the file according to the file header.
# free_percent:  Percentage of file consumed by free pages (calculated).
# free_percent2: Percentage of file consumed by free pages (header).
# ntable:        Number of tables in the db.
# nindex:        Number of indices in the db.
# nautoindex:    Number of indices created automatically.
# nmanindex:     Number of indices created manually.
# user_payload:  Number of bytes of payload in table btrees 
#                (not including sqlite_master)
# user_percent:  $user_payload as a percentage of total file size.

### The following, setting $file_bytes based on the actual size of the file
### on disk, causes this tool to choke on zipvfs databases. So set it based
### on the return of [PRAGMA page_count] instead.
if 0 {
  set file_bytes  [file size $file_to_analyze]
  set file_pgcnt  [expr {$file_bytes/$pageSize}]
}
set file_pgcnt  [db one {PRAGMA page_count}]
set file_bytes  [expr {$file_pgcnt * $pageSize}]

set av_pgcnt    [autovacuum_overhead $file_pgcnt $pageSize]
set av_percent  [percent $av_pgcnt $file_pgcnt]

set sql {SELECT sum(leaf_pages+int_pages+ovfl_pages) FROM space_used}
set inuse_pgcnt   [expr wide([mem eval $sql])]
set inuse_percent [percent $inuse_pgcnt $file_pgcnt]

set free_pgcnt    [expr {$file_pgcnt-$inuse_pgcnt-$av_pgcnt}]
set free_percent  [percent $free_pgcnt $file_pgcnt]
set free_pgcnt2   [db one {PRAGMA freelist_count}]
set free_percent2 [percent $free_pgcnt2 $file_pgcnt]

set file_pgcnt2 [expr {$inuse_pgcnt+$free_pgcnt2+$av_pgcnt}]

set ntable [db eval {SELECT count(*)+1 FROM sqlite_master WHERE type='table'}]
set nindex [db eval {SELECT count(*) FROM sqlite_master WHERE type='index'}]
set sql {SELECT count(*) FROM sqlite_master WHERE name LIKE 'sqlite_autoindex%'}
set nautoindex [db eval $sql]
set nmanindex [expr {$nindex-$nautoindex}]

# set total_payload [mem eval "SELECT sum(payload) FROM space_used"]
set user_payload [mem one {SELECT int(sum(payload)) FROM space_used
     WHERE NOT is_index AND name NOT LIKE 'sqlite_master'}]
set user_percent [percent $user_payload $file_bytes]

# Output the summary statistics calculated above.
#
puts "/** Disk-Space Utilization Report For $root_filename"
catch {
  puts "*** As of [clock format [clock seconds] -format {%Y-%b-%d %H:%M:%S}]"
}
puts ""
statline {Page size in bytes} $pageSize
statline {Pages in the whole file (measured)} $file_pgcnt
statline {Pages in the whole file (calculated)} $file_pgcnt2
statline {Pages that store data} $inuse_pgcnt $inuse_percent
statline {Pages on the freelist (per header)} $free_pgcnt2 $free_percent2
statline {Pages on the freelist (calculated)} $free_pgcnt $free_percent
statline {Pages of auto-vacuum overhead} $av_pgcnt $av_percent
statline {Number of tables in the database} $ntable
statline {Number of indices} $nindex
statline {Number of named indices} $nmanindex
statline {Automatically generated indices} $nautoindex
if {$isCompressed} {
  statline {Size of uncompressed content in bytes} $file_bytes
  set efficiency [percent $true_file_size $file_bytes]
  statline {Size of compressed file on disk} $true_file_size $efficiency
} else {
  statline {Size of the file in bytes} $file_bytes
}
statline {Bytes of user payload stored} $user_payload $user_percent

# Output table rankings
#
puts ""
puts "*** Page counts for all tables with their indices ********************"
puts ""
mem eval {SELECT tblname, count(*) AS cnt, 
              int(sum(int_pages+leaf_pages+ovfl_pages)) AS size
          FROM space_used GROUP BY tblname ORDER BY size+0 DESC, tblname} {} {
  statline [string toupper $tblname] $size [percent $size $file_pgcnt]
}
if {$isCompressed} {
  puts ""
  puts "*** Bytes of disk space used after compression ***********************"
  puts ""
  set csum 0
  mem eval {SELECT tblname,
                  int(sum(compressed_size)) +
                         $compressOverhead*sum(int_pages+leaf_pages+ovfl_pages)
                        AS csize
          FROM space_used GROUP BY tblname ORDER BY csize+0 DESC, tblname} {} {
    incr csum $csize
    statline [string toupper $tblname] $csize [percent $csize $true_file_size]
  }
  set overhead [expr {$true_file_size - $csum}]
  if {$overhead>0} {
    statline {Header and free space} $overhead [percent $overhead $true_file_size]
  }
}

# Output subreports
#
if {$nindex>0} {
  subreport {All tables and indices} 1
}
subreport {All tables} {NOT is_index}
if {$nindex>0} {
  subreport {All indices} {is_index}
}
foreach tbl [mem eval {SELECT name FROM space_used WHERE NOT is_index
                       ORDER BY name}] {
  regsub ' $tbl '' qn
  set name [string toupper $tbl]
  set n [mem eval "SELECT count(*) FROM space_used WHERE tblname='$qn'"]
  if {$n>1} {
    subreport "Table $name and all its indices" "tblname='$qn'"
    subreport "Table $name w/o any indices" "name='$qn'"
    subreport "Indices of table $name" "tblname='$qn' AND is_index"
  } else {
    subreport "Table $name" "name='$qn'"
  }
}

# Output instructions on what the numbers above mean.
#
puts {
*** Definitions ******************************************************

Page size in bytes

    The number of bytes in a single page of the database file.  
    Usually 1024.

Number of pages in the whole file
}
puts "    The number of $pageSize-byte pages that go into forming the complete
    database"
puts {
Pages that store data

    The number of pages that store data, either as primary B*Tree pages or
    as overflow pages.  The number at the right is the data pages divided by
    the total number of pages in the file.

Pages on the freelist

    The number of pages that are not currently in use but are reserved for
    future use.  The percentage at the right is the number of freelist pages
    divided by the total number of pages in the file.

Pages of auto-vacuum overhead

    The number of pages that store data used by the database to facilitate
    auto-vacuum. This is zero for databases that do not support auto-vacuum.

Number of tables in the database

    The number of tables in the database, including the SQLITE_MASTER table
    used to store schema information.

Number of indices

    The total number of indices in the database.

Number of named indices

    The number of indices created using an explicit CREATE INDEX statement.

Automatically generated indices

    The number of indices used to implement PRIMARY KEY or UNIQUE constraints
    on tables.

Size of the file in bytes

    The total amount of disk space used by the entire database files.

Bytes of user payload stored

    The total number of bytes of user payload stored in the database. The
    schema information in the SQLITE_MASTER table is not counted when
    computing this number.  The percentage at the right shows the payload
    divided by the total file size.

Percentage of total database

    The amount of the complete database file that is devoted to storing
    information described by this category.

Number of entries

    The total number of B-Tree key/value pairs stored under this category.

Bytes of storage consumed

    The total amount of disk space required to store all B-Tree entries
    under this category.  The is the total number of pages used times
    the pages size.

Bytes of payload

    The amount of payload stored under this category.  Payload is the data
    part of table entries and the key part of index entries.  The percentage
    at the right is the bytes of payload divided by the bytes of storage 
    consumed.

Average payload per entry

    The average amount of payload on each entry.  This is just the bytes of
    payload divided by the number of entries.

Average unused bytes per entry

    The average amount of free space remaining on all pages under this
    category on a per-entry basis.  This is the number of unused bytes on
    all pages divided by the number of entries.

Fragmentation

    The percentage of pages in the table or index that are not
    consecutive in the disk file.  Many filesystems are optimized
    for sequential file access so smaller fragmentation numbers 
    sometimes result in faster queries, especially for larger
    database files that do not fit in the disk cache.

Maximum payload per entry

    The largest payload size of any entry.

Entries that use overflow

    The number of entries that user one or more overflow pages.

Total pages used

    This is the number of pages used to hold all information in the current
    category.  This is the sum of index, primary, and overflow pages.

Index pages used

    This is the number of pages in a table B-tree that hold only key (rowid)
    information and no data.

Primary pages used

    This is the number of B-tree pages that hold both key and data.

Overflow pages used

    The total number of overflow pages used for this category.

Unused bytes on index pages

    The total number of bytes of unused space on all index pages.  The
    percentage at the right is the number of unused bytes divided by the
    total number of bytes on index pages.

Unused bytes on primary pages

    The total number of bytes of unused space on all primary pages.  The
    percentage at the right is the number of unused bytes divided by the
    total number of bytes on primary pages.

Unused bytes on overflow pages

    The total number of bytes of unused space on all overflow pages.  The
    percentage at the right is the number of unused bytes divided by the
    total number of bytes on overflow pages.

Unused bytes on all pages

    The total number of bytes of unused space on all primary and overflow 
    pages.  The percentage at the right is the number of unused bytes 
    divided by the total number of bytes.
}

# Output a dump of the in-memory database. This can be used for more
# complex offline analysis.
#
puts "**********************************************************************"
puts "The entire text of this report can be sourced into any SQL database"
puts "engine for further analysis.  All of the text above is an SQL comment."
puts "The data used to generate this report follows:"
puts "*/"
puts "BEGIN;"
puts $tabledef
unset -nocomplain x
mem eval {SELECT * FROM space_used} x {
  puts -nonewline "INSERT INTO space_used VALUES"
  set sep (
  foreach col $x(*) {
    set v $x($col)
    if {$v=="" || ![string is double $v]} {set v [quote $v]}
    puts -nonewline $sep$v
    set sep ,
  }
  puts ");"
}
puts "COMMIT;"

} err]} {
  puts "ERROR: $err"
  puts $errorInfo
  exit 1
}
