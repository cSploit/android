package org.csploit.msf;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Represent an author
 */
public class Author {
  private static Pattern MAIL = Pattern.compile("<(.+)>");

  private String name;
  private String email;

  public Author(String name, String email) {
    this.name = name;
    this.email = email;
  }

  @Override
  public String toString() {
    if(email != null) {
      return name + " <" + email + ">";
    } else {
      return name;
    }
  }

  public static Author fromString(String s) {
    Matcher matcher = MAIL.matcher(s);
    if(matcher.matches()) {
      return new Author(s.substring(0, matcher.start()), matcher.group(1));
    } else {
      return new Author(s, null);
    }
  }
}
