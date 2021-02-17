Internationalization (I18N) of the Win32 installer.
===================================================

The intention of adding internationalization to the Win32 Firebird
installer is to provide a minimum level of translation to support
the installation process only. This means just translating the install
dialogues, the readme file and the installation readme file. All other
documentatation i18n should be available separately. I18n is a
good thing, but bloating the installer with large amounts of translated
documentation is not desirable. 

The current version of InnoSetup used by Firebird $MAJOR.$MINOR - 5.4.2 - provides 
generic support for the following languages:

	Basque, BrazilianPortuguese, Catalan, Czech, Danish, Dutch, Finnish, French,
	German, Hebrew, Hungarian, Italian, Japanese, Norwegian, Polish, Portuguese, 
	Russian, Slovak, Slovenian, and Spanish	

In addition, the InnoSetup user community has made other language packs
available for download. See here for details:

  http://www.jrsoftware.org/files/istrans/

Therefore adding i18n support to the Firebird installer is extremely
simple as all we need are translations of the Firebird specific messages.

Currently the Firebird installer has support for Bosnian, Czech, Spanish, 
French, German, Hungarian, Italian, Polish, Portuguese (standard), Russian, 
Slovakian and Slovenian installs. So there are still opportunities for others 
to provide support for their native language.


How to add new languages
------------------------

The simplest way to understand this is to study the implementation of
an existing translation. The steps to follow are these:

o The Win32 install files are located in install\arch-specific\win32.
  This sub-directory is located as follows:

    Firebird 1.5  - firebird2\src
    Firebird 2.n  - firebird2\builds
    Firebird 3.n  - firebird2\builds

o You can use a tool such as TortoiseSVN to checkout the Win32 install 
  kit. Just open the SVN checkout dialogue and enter something similar 
  to this Repository URL:
     https://firebird.svn.sourceforge.net/svnroot/firebird/firebird/branch/B2_5_Release/builds/install/arch-specific/win32
 or this:
     https://firebird.svn.sourceforge.net/svnroot/firebird/firebird/tags/R2_5_1/builds/install/arch-specific/win32
For 2.1 the equivalent URLs would be:
     https://firebird.svn.sourceforge.net/svnroot/firebird/firebird/branch/B2_1_Release/builds/install/arch-specific/win32
     https://firebird.svn.sourceforge.net/svnroot/firebird/firebird/tags/R2_1_4/builds/install/arch-specific/win32



o Each language has its own sub-directory under install\arch-specific\win32.
  If possible a two-letter language code should be used for the
  directory name. This two letter code should, if possible, be the 
  internationally recognised two-letter identifier for the langauge.


o Three files are required:

  * custom_messages_%LANG_CODE%.inc

    where %LANG_CODE% represents the language identifier used in the
    directory name. This file contains the translated strings that are
    used throughout the installation process. The original, english
    strings are stored in install\arch-specific\win32\custom_messages.inc

  * readme.txt

    where the actual file name can be either in english or in the
    translated equivalent of the name. The readme file stores last
    minute documentation changes as well as general notes on using
    firebird. It is displayed at the end of the installation process.

    The readme file is always subject to change from one beta or 
    pre-release to the next (or final release.) For this reason the 
    translated readme should always explain that the english language 
    readme.txt is the definitive version. This is because it will 
    not be possible to hold up a release while waiting for translators 
    to complete their work.

  * installation_readme.txt

    This filename can be translated or left with the original english
    title. It contains installation specific details. It is displayed
    near the beginning of the installation process. The installation 
    readme is usually static and only changes from point release to 
    point release.


o Issues to bear in mind

  * Use ascii characters for filenames where possible.

  * All strings that appear as text labels in dialogue screens should contain 
    an ampersand ( & ) to support keyboard input during setup. 

  * The installation readme file and the readme file must be formatted to 
    a column width of 55 chars. Otherwise the installer will wrap lines 
    awkwardly. This is because the installer screens use the Courier New
    font to display these text files. Courier New guarantees to maintain
    column alignment.
    

o Adding the new language to the InnoSetup install script

  Changes have been kept to the absolute minimum. Only the following
  sections need to be altered:

    [Languages]
    Name: en; MessagesFile: compiler:Default.isl; InfoBeforeFile: output\doc\installation_readme.txt; InfoAfterFile: src\install\arch-specific\win32\readme.txt;
    Name: fr; MessagesFile: compiler:Languages\French.isl; InfoBeforeFile: src\install\arch-specific\win32\fr\installation_lisezmoi.txt; InfoAfterFile: src\install\arch-specific\win32\fr\lisezmoi.txt;

    [Messages]
    en.BeveledLabel=English
    fr.BeveledLabel=Français

    [CustomMessages]
    #include "custom_messages.inc"
    #include "fr\custom_messages_fr.inc"

  You are not required to make these changes.


o Submitting your changes

  All i18n is being co-ordinated by Paul Reeves who maintains the Win32
  installation kits. He can be contacted at:

     mailto: preeves at ibphoenix.com


