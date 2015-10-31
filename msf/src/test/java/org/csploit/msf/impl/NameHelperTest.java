package org.csploit.msf.impl;

import org.junit.Test;

import static org.junit.Assert.*;
import static org.hamcrest.CoreMatchers.*;

/**
 * Test name helper class
 */
public class NameHelperTest {

  @Test
  public void testGetTypeAndRefnameFromFullname() throws Exception {
    String[] names = NameHelper.getTypeAndRefnameFromFullname("exploit/test//test/-$%^");

    assertThat(names.length, is(2));
    assertThat(names[0], is("exploit"));
    assertThat(names[1], startsWith("test//test"));

    boolean exceptionFired = false;

    try {
      NameHelper.getTypeAndRefnameFromFullname("no-slashes-here");
    } catch (NameHelper.BadModuleNameException e) {
      exceptionFired = true;
    }

    assertThat(exceptionFired, is(true));
  }

  @Test
  public void testValidModuleType() throws Exception {
    boolean exceptionFired = false;
    String valid = "exploit";
    String invalid = "not-a-valid-type";

    assertThat(NameHelper.isModuleTypeValid(valid), is(true));
    assertThat(NameHelper.isModuleTypeValid(invalid), is(false));

    NameHelper.assertValidModuleType(valid);

    try {
      NameHelper.assertValidModuleType(invalid);
    } catch (NameHelper.BadModuleTypeException e) {
      exceptionFired = true;
    }

    assertThat(exceptionFired, is(true));
  }
}