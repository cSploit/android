package org.csploit.msf.impl;

import org.csploit.msf.api.Module;

/**
 * Represent an option
 */
public abstract class Option<T> implements org.csploit.msf.api.Option<T> {
  protected boolean advanced;
  protected T defaultValue;
  protected String description;
  protected boolean evasion;
  protected String name;
  protected Module owner;
  protected boolean required;

  public Option(String name, boolean required, String description, T defaultValue) {
    this.name = name;
    this.required = required;
    this.description = description;
    this.defaultValue = defaultValue;
  }

  public Option(String name, String description, T defaultValue) {
    this(name, false, description, defaultValue);
  }

  public boolean isAdvanced() {
    return advanced;
  }

  public void setAdvanced(boolean advanced) {
    this.advanced = advanced;
  }

  public T getDefaultValue() {
    return defaultValue;
  }

  public boolean haveDefaultValue() {
    return defaultValue != null;
  }

  public String getDescription() {
    return description;
  }

  public boolean isEvasion() {
    return evasion;
  }

  public void setEvasion(boolean evasion) {
    this.evasion = evasion;
  }

  public String getName() {
    return name;
  }

  public void setName(String name) {
    this.name = name;
  }

  public Module getOwner() {
    return owner;
  }

  public void setOwner(Module owner) {
    this.owner = owner;
  }

  public boolean isRequired() {
    return required;
  }

  protected abstract T normalize(String input);

  protected String display(T value) {
    return value.toString();
  }

  protected boolean isValid(T input) {
    if(isRequired() && (input == null || input.toString().length() == 0)) {
      return false;
    }
    return true;
  }
}
