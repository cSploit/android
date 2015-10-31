package org.csploit.msf.impl;

import org.csploit.msf.api.MsfException;

/**
 * Help manage modules names
 */
final class NameHelper {
  public static String[] getTypeAndRefnameFromFullname(String fullname) throws BadModuleNameException, BadModuleTypeException {
    int index = fullname.indexOf('/');

    if(index == -1 || index == fullname.length()) {
      throw new BadModuleNameException(fullname);
    }

    String type = fullname.substring(0, index);

    if(!isModuleTypeValid(type)) {
      throw new BadModuleTypeException(type);
    }

    return new String[]{type, fullname.substring(index + 1)};
  }


  public static boolean isModuleTypeValid(String type) {
    if(type == null) {
      return false;
    }
    type = type.toLowerCase();
    for(String validType : ModuleManager.validModuleTypes) {
      if(validType.equals(type)) {
        return true;
      }
    }
    return false;
  }

  public static void assertValidModuleType(String type) throws BadModuleTypeException {
    if (!isModuleTypeValid(type)) {
      throw new BadModuleTypeException(type);
    }
  }

  public static class BadModuleNameException extends MsfException {
    public BadModuleNameException(String fullname) {
      super("invalid module name: " + fullname);
    }
  }

  public static class BadModuleTypeException extends MsfException {
    public BadModuleTypeException(String type) {
      super("invalid module type: " + type);
    }
  }

  public static class BadSessionTypeException extends MsfException {
    public BadSessionTypeException(String message) {
      super("invalid session type: " + message);
    }
  }
}
