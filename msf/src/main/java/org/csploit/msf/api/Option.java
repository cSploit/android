package org.csploit.msf.api;

/**
 * An option
 */
public interface Option<T> {
  boolean isAdvanced();
  boolean haveDefaultValue();
  boolean isEvasion();
  boolean isRequired();
  T getDefaultValue();
  String getDescription();
  String getName();
  Module getOwner();
}
