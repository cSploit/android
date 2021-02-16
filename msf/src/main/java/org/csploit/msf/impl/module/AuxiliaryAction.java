package org.csploit.msf.impl.module;

/**
 * AuxiliaryAction implementation
 */
public class AuxiliaryAction implements org.csploit.msf.api.module.AuxiliaryAction {
  private final String name;
  private String description;

  public AuxiliaryAction(String name, String description) {
    this.name = name;
    this.description = description;
  }

  public AuxiliaryAction(String name) {
    this.name = name;
  }

  @Override
  public String getName() {
    return name;
  }

  @Override
  public String getDescription() {
    return description;
  }

  public void setDescription(String description) {
    this.description = description;
  }
}
