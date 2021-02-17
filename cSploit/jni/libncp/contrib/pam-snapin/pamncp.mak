#
# Borland C++ IDE generated makefile
#
.AUTODEPEND


#
# Borland C++ tools
#
IMPLIB  = Implib
BCC32   = Bcc32 +BccW32.cfg 
TLINK32 = TLink32
TLIB    = TLib
BRC32   = Brc32
TASM32  = Tasm32
#
# IDE macros
#


#
# Options
#
IDE_LFLAGS32 =  -LC:\BC45\LIB
IDE_RFLAGS32 = 
LLATW32_pamncpdlib =  -Tpd -aa -c -LC:\BC45\LIB;C:\NOVELL\NDS\NWSDK\LIB\WIN32\BORLAND
RLATW32_pamncpdlib =  -w32
BLATW32_pamncpdlib = 
CNIEAT_pamncpdlib = -IC:\BC45\INCLUDE;C:\NOVELL\NDK\NWSDK\INCLUDE -D_RTLDLL;_BIDSDLL;_OWLDLL;_OWLALLPCH;
LNIEAT_pamncpdlib = -x
LEAT_pamncpdlib = $(LLATW32_pamncpdlib)
REAT_pamncpdlib = $(RLATW32_pamncpdlib)
BEAT_pamncpdlib = $(BLATW32_pamncpdlib)
CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = 
LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = 
RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = 
BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = 
CEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = $(CEAT_pamncpdlib) $(CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib)
CNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = -IC:\BC45\INCLUDE;C:\NOVELL\NDK\NWSDK\INCLUDE -D_RTLDLL;_BIDSDLL;_OWLDLL;_OWLALLPCH;
LNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = -x
LEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = $(LEAT_pamncpdlib) $(LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib)
REAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = $(REAT_pamncpdlib) $(RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib)
BEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib = $(BEAT_pamncpdlib) $(BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbcalwin32dlib)
CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = 
LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = 
RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = 
BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = 
CEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = $(CEAT_pamncpdlib) $(CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib)
CNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = -IC:\BC45\INCLUDE;C:\NOVELL\NDK\NWSDK\INCLUDE -D_RTLDLL;_BIDSDLL;_OWLDLL;_OWLALLPCH;
LNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = -x
LEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = $(LEAT_pamncpdlib) $(LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib)
REAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = $(REAT_pamncpdlib) $(RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib)
BEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib = $(BEAT_pamncpdlib) $(BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbclxwin32dlib)
CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = 
LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = 
RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = 
BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = 
CEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = $(CEAT_pamncpdlib) $(CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib)
CNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = -IC:\BC45\INCLUDE;C:\NOVELL\NDK\NWSDK\INCLUDE -D_RTLDLL;_BIDSDLL;_OWLDLL;_OWLALLPCH;
LNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = -x
LEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = $(LEAT_pamncpdlib) $(LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib)
REAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = $(REAT_pamncpdlib) $(RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib)
BEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib = $(BEAT_pamncpdlib) $(BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandblocwin32dlib)
CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = 
LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = 
RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = 
BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = 
CEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = $(CEAT_pamncpdlib) $(CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib)
CNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = -IC:\BC45\INCLUDE;C:\NOVELL\NDK\NWSDK\INCLUDE -D_RTLDLL;_BIDSDLL;_OWLDLL;_OWLALLPCH;
LNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = -x
LEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = $(LEAT_pamncpdlib) $(LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib)
REAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = $(REAT_pamncpdlib) $(RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib)
BEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib = $(BEAT_pamncpdlib) $(BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbnetwin32dlib)
CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = 
LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = 
RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = 
BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = 
CEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = $(CEAT_pamncpdlib) $(CLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib)
CNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = -IC:\BC45\INCLUDE;C:\NOVELL\NDK\NWSDK\INCLUDE -D_RTLDLL;_BIDSDLL;_OWLDLL;_OWLALLPCH;
LNIEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = -x
LEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = $(LEAT_pamncpdlib) $(LLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib)
REAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = $(REAT_pamncpdlib) $(RLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib)
BEAT_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib = $(BEAT_pamncpdlib) $(BLATW16_ddbddbddbnovellbndkbnwsdkblibbwin32bborlandbsnapin32dlib)

#
# Dependency List
#
Dep_pamncp = \
   pamncp.lib

pamncp : BccW32.cfg $(Dep_pamncp)
  echo MakeNode 

pamncp.lib : pamncp.dll
  $(IMPLIB) $@ pamncp.dll


Dep_pamncpddll = \
   ..\..\..\novell\ndk\nwsdk\lib\win32\borland\calwin32.lib\
   ..\..\..\novell\ndk\nwsdk\lib\win32\borland\clxwin32.lib\
   ..\..\..\novell\ndk\nwsdk\lib\win32\borland\locwin32.lib\
   ..\..\..\novell\ndk\nwsdk\lib\win32\borland\netwin32.lib\
   ..\..\..\novell\ndk\nwsdk\lib\win32\borland\snapin32.lib\
   pamncp.obj\
   pamncp.def\
   pamncp.res

pamncp.dll : $(Dep_pamncpddll)
  $(TLINK32) @&&|
 /v $(IDE_LFLAGS32) $(LEAT_pamncpdlib) $(LNIEAT_pamncpdlib) +
C:\BC45\LIB\c0d32.obj+
pamncp.obj
$<,$*
..\..\..\novell\ndk\nwsdk\lib\win32\borland\calwin32.lib+
..\..\..\novell\ndk\nwsdk\lib\win32\borland\clxwin32.lib+
..\..\..\novell\ndk\nwsdk\lib\win32\borland\locwin32.lib+
..\..\..\novell\ndk\nwsdk\lib\win32\borland\netwin32.lib+
..\..\..\novell\ndk\nwsdk\lib\win32\borland\snapin32.lib+
C:\BC45\LIB\owlwfi.lib+
C:\BC45\LIB\bidsfi.lib+
C:\BC45\LIB\import32.lib+
C:\BC45\LIB\cw32i.lib
pamncp.def
|
   $(BRC32) pamncp.res $<

pamncp.obj :  pamncp.c
  $(BCC32) -P- -c @&&|
 $(CEAT_pamncpdlib) $(CNIEAT_pamncpdlib) -o$@ pamncp.c
|

pamncp.res :  pamncp.rc
  $(BRC32) $(IDE_RFLAGS32) $(REAT_pamncpdlib) $(CNIEAT_pamncpdlib) -R -FO$@ pamncp.rc

# Compiler configuration file
BccW32.cfg : 
   Copy &&|
-R
-v
-vi
-H
-H=pamncp.csm
-WD
-H"owl\owlpch.h"
| $@


