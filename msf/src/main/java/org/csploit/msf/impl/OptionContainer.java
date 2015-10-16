package org.csploit.msf.impl;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * A container of options
 */
class OptionContainer extends HashMap<String, Option> {

  public void addAdvancedOption(String name, Option option) {
    addOption(name, option, true, false);
  }

  public void addEvasionOption(String name, Option option) {
    addOption(name, option, false, true);
  }

  public void addOption(String name, Option option, boolean advanced, boolean evasion) {
    option.setEvasion(evasion);
    option.setAdvanced(advanced);
    put(name, option);
  }

  /**
   * this should be the validate method in the MSF.
   * I decided to change it because updating dataStore
   * within modules will write global values into it.
   */
  @SuppressWarnings("unchecked")
  public Collection<String> getInvalidOptions(DataStore dataStore) {
    ArrayList<String> errors = new ArrayList<>();

    for(Map.Entry<String, Option> entry : entrySet()) {
      String name = entry.getKey();
      Option option = entry.getValue();

      String val = dataStore.get(name);
      Object normalized = option.normalize(val);

      if(!option.isValid(normalized)) {
        errors.add(name);
      }
    }

    return errors;
  }
}
