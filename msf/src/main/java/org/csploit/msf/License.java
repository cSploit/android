package org.csploit.msf;

/**
 * A license
 */
public enum License {
  MSF("Metasploit Framework License (BSD)"),
  GPL("GNU Public License v2.0"),
  BSD("BSD License"),
  ARTISTIC("Perl Artistic License"),
  UNKNOWN("Unknown License");

  String name;

  License(String name) {
    this.name = name;
  }

  public String toString() {
    return name;
  }

  public static License fromString(String value) {
    if(value != null) {
      for (License l : values()) {
        if (l.name.toLowerCase().startsWith(value.toLowerCase()))
          return l;
      }
    }
    return UNKNOWN;
  }
}
