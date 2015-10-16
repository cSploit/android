package org.csploit.msf.impl;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Represent an author
 */
class Author implements org.csploit.msf.api.Author {
  private static final Pattern MAIL = Pattern.compile("<(.+)>");

  private final String name;
  private final String email;

  public Author(String name, String email) {
    this.name = name;
    this.email = email;
  }

  public String getName() {
    return name;
  }

  public String getEmail() {
    return email;
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
