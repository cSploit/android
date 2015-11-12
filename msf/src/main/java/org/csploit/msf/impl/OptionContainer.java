package org.csploit.msf.impl;

import org.csploit.msf.api.Option;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * A container of options
 */
class OptionContainer extends HashMap<String, Option> {

  public void add(Option option) {
    put(option.getName(), option);
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
