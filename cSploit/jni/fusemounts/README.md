fusemounts
==========

this program list FUSE directory bindings.

FUSE binds are useful for restict permissions or change uid/gid of files on a directory.

for example Android use FUSE for show ext4 partitions as FAT ( like `/data/media` on `/sdcard` ).

retrieve source directories for these bindings allow you to access to the real filesystem,
thus to have full control over file modes and symlinks.

i wrote that for hacking Android permission restrictions, but you can use it everywhere.

report bugs to the [github issue page](https://github.com/tux-mind/fusemounts/issues).
