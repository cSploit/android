package org.csploit.msf.module;

/**
 * A reference to some sort of information.  This is typically a URL, but could
 * be any type of referential value that people could use to research a topic.
 */
public class Reference {
  protected String str;

  public Reference(String str) {
    this.str = "" + str;
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    Reference reference = (Reference) o;

    return str.equals(reference.str);
  }

  @Override
  public int hashCode() {
    return str.hashCode();
  }

  @Override
  public String toString() {
    return str;
  }

  public static Reference fromString(String s) {
    return new Reference(s);
  }
}
