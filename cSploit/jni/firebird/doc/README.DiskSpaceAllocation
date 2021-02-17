
	Since the beginning, Firebird had no rules of how to allocate disk space
for database file(s). It just writes new allocated pages in not determined 
order (because of dependencies between pages to serve "careful write" strategy).

	This approach is very simple but has some drawbacks :
  
- because of not determined order of writes, there may be such situation when 
  page cache contains many dirty pages at time when new allocated page must be
  written but can't because out of disk space. In such cases often all other
  dirty pages are lost because administrators prefer to shutdown database
  before making some space on disk available. This leads to serious corruptions.

- allocating disk space by relatively small chunks may lead to significant file 
  fragmentation at file system level and reduce performance of large scans (for
  example during backup).


	Using new ODS 11.1, Firebird changes its disk space allocation algorithm to 
avoid corruptions in out of disk space conditions and to give the file system a 
chance to avoid fragmentation. These changes are described below.

a)  Every newly allocated page is written on disk immediately before returning to
  the engine. If page can't be written then allocation doesn't happen, PIP bit
  is not cleared and appropriate IO error is raised. This error can't lead to 
  corruptions as we have a guarantee that all dirty pages in cache have disk 
  space allocated and can be written safely.

	This change makes one additional write of every newly allocated page compared
  with old behavior. So performance penalty is expected during the database file
  growth. To reduce this penalty Firebird groups writes of newly allocated pages
  up to 128KB at a time and tracks number of "initialized" pages at PIP header.
   
	Note : newly allocated page will be written to disk twice only if this page
  is allocated first time. I.e. if page was allocated, freed and allocated again
  it will not be written twice on second allocation.
   
b) To avoid file fragmentation, Firebird used appropriate file system's API to
  preallocate disk space by relatively large chunks. Currently such API exists
  only in Windows but it was recently added into Linux API and may be implemented 
  in such popular file system's as ext2, etc in the future. So this feature is
  currently implemented only in Windows builds of Firebird and may be implemented 
  in Linux builds in the future.
  
	For better control of disk space preallocation, new setting in Firebird.conf
  was introduced : DatabaseGrowthIncrement. This is upper bound of preallocation 
  chunk size in bytes. Default value is 128MB. When engine needs more disk space 
  it allocates 1/16th of already allocated space but no less than 128KB and no more 
  than DatabaseGrowthIncrement value. If DatabaseGrowthIncrement is set to zero then
  preallocation is disabled. Space for database shadow files is not preallocated.
  Also preallocation is disabled if "No reserve" option is set for database.

	Note : preallocation also allows to avoid corruptions in out of disk space 
  condition - in such case there is a big chance that database has enough space
  preallocated to operate until administrator makes some disk space available.


	Author: Vlad Khorsun, <hvlad at users sourceforge net>

