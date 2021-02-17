AM_CPPFLAGS = -I$(top_srcdir)/include/csploit

headers:=$(top_srcdir)/include/csploit/arraylist.h\
$(top_srcdir)/include/csploit/buffer.h\
$(top_srcdir)/include/csploit/control.h\
$(top_srcdir)/include/csploit/control_messages.h\
$(top_srcdir)/include/csploit/file_logger.h\
$(top_srcdir)/include/csploit/io.h\
$(top_srcdir)/include/csploit/list.h\
$(top_srcdir)/include/csploit/logger.h\
$(top_srcdir)/include/csploit/message.h\
$(top_srcdir)/include/csploit/msgqueue.h\
$(top_srcdir)/include/csploit/sequence.h\
$(top_srcdir)/include/csploit/sorted_arraylist.h\
$(top_srcdir)/include/csploit/str_array.h

lib_LTLIBRARIES = libcsploit-common.la
libcsploit_common_la_SOURCES = arraylist.c buffer.c control.c file_logger.c io.c list.c logger.c message.c msgqueue.c sequence.c sorted_arraylist.c str_array.c
libcsploit_common_la_LDFLAGS=-version-info 1:0
csploitincludedir = $(includedir)/csploit
csploitinclude_HEADERS = $(headers)
