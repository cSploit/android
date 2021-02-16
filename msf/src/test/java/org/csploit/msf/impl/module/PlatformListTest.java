package org.csploit.msf.impl.module;

import org.junit.Test;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

/**
 * Test unit for PlatformList class
 */
public class PlatformListTest {

  @Test
  public void testIntersect() throws Exception {
    PlatformList l1, l2, ex;

    l1 = new PlatformList("Windows 98, Solaris V4");
    l2 = new PlatformList("Windows 98 FE, Solaris");
    ex = new PlatformList("Windows 98 FE, Solaris V4");

    assertThat(l1.intersect(l2), is(ex));
    assertThat(l2.intersect(l1), is(ex));
  }
}