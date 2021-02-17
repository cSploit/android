From: Kirill Simonov <xi@resolvent.net>
To: yaml-core@lists.sourceforge.net
Subject: LibYAML-0.1.4: A minor bugfix release

This is a minor bugfix release of LibYAML, a YAML parser and emitter
written in C:

* Fixed a bug that prevented an empty mapping being used as a simple key
  (thank to spitzak(at)rhythm(dot)com).
* Fixed pointer overflow when calculating the position of a potential
  simple key (thank to ppelletier(at)oblong(dot)com).
* Fixed yaml.dll not exporting any symbols
  (thank to pxn11432(at)nifty(dot)com).
* Added pkg-config support (thank to rainwoodman(at)gmail(dot)com).

LibYAML homepage:   http://pyyaml.org/wiki/LibYAML
TAR.GZ package:     http://pyyaml.org/download/libyaml/yaml-0.1.4.tar.gz
SVN repository:     http://svn.pyyaml.org/libyaml/branches/stable
Bug tracker:        http://pyyaml.org/newticket?component=libyaml

The library is functionally complete, but the documentation is scarce
and the API may change.  For more information, you may check the project
homepage, the doxygen-generated documentation in the `doc` directory of
the source distribution, and examples in the `tests` directory.

LibYAML is written by Kirill Simonov <xi@resolvent.net> and released
under the MIT license; see the file LICENSE for more details.

