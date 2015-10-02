package org.csploit.msf.module;

/**
 * A platform
 */
public enum Platform {
  WINDOWS("Windows", 1, 0, 0),
  WINDOWS_95("Windows 95", 1, 1, 0),
  WINDOWS_98("Windows 98", 1, 2, 0),
  WINDOWS_98_FE("Windows 98 FE", 1, 2, 1),
  WINDOWS_98_SE("Windows 98 SE", 1, 2, 2),
  WINDOWS_ME("Windows ME", 1, 3, 0),
  WINDOWS_NT("Windows NT", 1, 4, 0),
  WINDOWS_NT_SP0("Windows NT SP0", 1, 4, 1),
  WINDOWS_NT_SP1("Windows NT SP1", 1, 4, 2),
  WINDOWS_NT_SP2("Windows NT SP2", 1, 4, 3),
  WINDOWS_NT_SP3("Windows NT SP3", 1, 4, 4),
  WINDOWS_NT_SP4("Windows NT SP4", 1, 4, 5),
  WINDOWS_NT_SP5("Windows NT SP5", 1, 4, 6),
  WINDOWS_NT_SP6("Windows NT SP6", 1, 4, 7),
  WINDOWS_NT_SP6A("Windows NT SP6a", 1, 4, 8),
  WINDOWS_2000("Windows 2000", 1, 5, 0),
  WINDOWS_2000_SP0("Windows 2000 SP0", 1, 5, 1),
  WINDOWS_2000_SP1("Windows 2000 SP1", 1, 5, 2),
  WINDOWS_2000_SP2("Windows 2000 SP2", 1, 5, 3),
  WINDOWS_2000_SP3("Windows 2000 SP3", 1, 5, 4),
  WINDOWS_2000_SP4("Windows 2000 SP4", 1, 5, 5),
  WINDOWS_XP("Windows XP", 1, 6, 0),
  WINDOWS_XP_SP0("Windows XP SP0", 1, 6, 1),
  WINDOWS_XP_SP1("Windows XP SP1", 1, 6, 2),
  WINDOWS_XP_SP2("Windows XP SP2", 1, 6, 3),
  WINDOWS_XP_SP3("Windows XP SP3", 1, 6, 4),
  WINDOWS_2003("Windows 2003", 1, 7, 0),
  WINDOWS_2003_SP0("Windows 2003 SP0", 1, 7, 1),
  WINDOWS_2003_SP1("Windows 2003 SP1", 1, 7, 2),
  WINDOWS_VISTA("Windows Vista", 1, 8, 0),
  WINDOWS_VISTA_SP0("Windows Vista SP0", 1, 8, 1),
  WINDOWS_VISTA_SP1("Windows Vista SP1", 1, 8, 2),
  WINDOWS_7("Windows 7", 1, 9, 0),
  WINDOWS_8("Windows 8", 1, 10, 0),
  NETWARE("Netware", 2, 0, 0),
  ANDROID("Android", 3, 0, 0),
  JAVA("Java", 4, 0, 0),
  RUBY("Ruby", 5, 0, 0),
  LINUX("Linux", 6, 0, 0),
  CISCO("Cisco", 7, 0, 0),
  SOLARIS("Solaris", 8, 0, 0),
  SOLARIS_V4("Solaris V4", 8, 1, 0),
  SOLARIS_V5("Solaris V5", 8, 2, 0),
  SOLARIS_V6("Solaris V6", 8, 3, 0),
  SOLARIS_V7("Solaris V7", 8, 4, 0),
  SOLARIS_V8("Solaris V8", 8, 5, 0),
  SOLARIS_V9("Solaris V9", 8, 6, 0),
  SOLARIS_V10("Solaris V10", 8, 7, 0),
  OSX("OSX", 9, 0, 0),
  BSD("BSD", 10, 0, 0),
  OPENBSD("OpenBSD", 11, 0, 0),
  BSDI("BSDi", 12, 0, 0),
  NETBSD("NetBSD", 13, 0, 0),
  FREEBSD("FreeBSD", 14, 0, 0),
  AIX("AIX", 15, 0, 0),
  HPUX("HPUX", 16, 0, 0),
  IRIX("Irix", 17, 0, 0),
  UNIX("Unix", 18, 0, 0),
  PHP("PHP", 19, 0, 0),
  JAVASCRIPT("JavaScript", 20, 0, 0),
  PYTHON("Python", 21, 0, 0),
  NODEJS("NodeJS", 22, 0, 0),
  FIREFOX("Firefox", 23, 0, 0),
  MAINFRAME("Mainframe", 24, 0, 0);

  protected String realName;
  protected int major;
  protected int minor;
  protected int patch;

  Platform(String realName, int major, int minor, int patch) {
    this.realName = realName;
    this.major = major;
    this.minor = minor;
    this.patch = patch;
  }

  public String getRealName() {
    return realName;
  }

  /**
   * is a platform a subset of another ?
   * @param other possible ancestor to test
   * @return true if {@code other} is a platform that contains {@code this}
   */
  public boolean subsetOf(Platform other) {
    if(other.major != major)
      return false;
    if(other.minor == 0)
      return true;
    if(other.minor != minor)
      return false;
    if(other.patch == 0)
      return true;
    if(other.patch != patch)
      return false;
    return true;
  }

  public static Platform fromString(String s) {
    if(s != null) {
      s = s.trim();
      for (Platform pl : values()) {
        if (pl.getRealName().equals(s))
          return pl;
      }
    }
    return null;
  }

}
