package org.csploit.android.helpers;

import junit.framework.Assert;
import junit.framework.TestCase;

/**
 * Test NetworkHelper class
 */
public class NetworkHelperTest extends TestCase {

  public void testGetOUICode() throws Exception {
    byte[] address = new byte[] { 0x01, 0x02, 0x03 };

    int fromString = NetworkHelper.getOUICode("010203");
    int fromMAC = NetworkHelper.getOUICode(address);

    Assert.assertEquals(fromString + " differs from " + fromMAC, fromString, fromMAC);
  }
}
