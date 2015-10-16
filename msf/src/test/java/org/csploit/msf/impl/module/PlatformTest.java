package org.csploit.msf.impl.module;

import static org.junit.Assert.*;
import static org.hamcrest.CoreMatchers.*;

import org.csploit.msf.api.module.Platform;
import org.junit.Test;

/**
 * Test unit for Platform enum
 */
public class PlatformTest {

  @Test
  public void testFromString() throws Exception {
    assertThat(Platform.fromString("Windows"), is(Platform.WINDOWS));
    assertThat(Platform.fromString("Windows NT SP3"), is(Platform.WINDOWS_NT_SP3));
    assertThat(Platform.fromString(null), nullValue());
    assertThat(Platform.fromString("Unknown"), nullValue());
    assertThat(Platform.fromString("Unix"), is(Platform.UNIX));
  }

  @Test
  public void testSubsetOf() throws Exception {
    assertThat(Platform.WINDOWS_2000_SP0.subsetOf(Platform.WINDOWS), is(true));
    assertThat(Platform.WINDOWS.subsetOf(Platform.WINDOWS_2000), is(false));
    assertThat(Platform.WINDOWS_VISTA_SP0.subsetOf(Platform.WINDOWS_NT_SP0), is(false));
    assertThat(Platform.UNIX.subsetOf(Platform.AIX), is(false));
    assertThat(Platform.SOLARIS_V10.subsetOf(Platform.SOLARIS), is(true));

    for(Platform platform : Platform.values()) {
      assertThat(platform.subsetOf(platform), is(true));
    }
  }
}