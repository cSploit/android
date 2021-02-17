icu.exe is a self-extract pre-built (by us) IBM ICU 52.1 library.

The sources was downloaded from http://site.icu-project.org/download.

Two customized data files was created using the tool at http://apps.icu-project.org/datacustom.

One was created empty and saved to icudt52l_empty.dat. This file is used when building ICU, to
make icudt a little stub. The file was saved in ICU sources directory at <icu>\source\data\in
with name icudt52l.dat, before start ICU building process.

The other is the data file we use in runtime. It was created selecting the options:
- Charset Mapping Tables: ibm-943_P15A-2003
- Collators
- Base data
