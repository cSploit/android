package org.csploit.msf.impl;

import org.csploit.msf.impl.options.StringOption;
import org.junit.Test;

import java.util.Set;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

/**
 * Test the DataStore class
 */
public class DataStoreTest {

  private DataStore buildDataStoreWithDefaultOption() {
    DataStore store = new DataStore();
    OptionContainer options = new OptionContainer();

    options.addOption("A",
            new StringOption("A", "description", "default value"), false, false);

    store.importOptions(options, true);

    return store;
  }

  @Test
  public void testImportOptions() throws Exception {
    DataStore store = buildDataStoreWithDefaultOption();

    assertThat(store.get("A"), is("default value"));
  }

  @Test
  public void testUserDefined() throws Exception {
    DataStore store = buildDataStoreWithDefaultOption();

    assertThat(store.userDefined().isEmpty(), is(true));

    store.put("A", "user-defined value");

    assertThat(store.userDefined().isEmpty(), is(false));
  }

  @Test
  public void testClearNotUserDefined() throws Exception {
    DataStore store = buildDataStoreWithDefaultOption();

    store.put("B", "user-defined value");

    store.clearNotUserDefined();

    Set<String> keys = store.keySet();

    assertThat(keys.contains("B"), is(true));
    assertThat(keys.contains("A"), is(false));
  }
}