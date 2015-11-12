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

  /**
   * show a string representation of an option
   * @param input the value to display
   * @return a string representation of an option
   */
  String display(T input);

  /**
   * check if a value is suitable for this option
   * @param value the value to check
   * @return {@code true} if value is valid, {@code false} otherwise
   */
  boolean isValid(T value);

  /**
   * normalize a string for this option
   * @param value the String to normalize
   * @return the normalized value
   */
  T normalize(String value);
}
