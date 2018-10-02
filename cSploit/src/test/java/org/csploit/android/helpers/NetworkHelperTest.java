package org.csploit.android.helpers;

import junit.framework.Assert;
import junit.framework.TestCase;

import java.net.InetAddress;

import static org.hamcrest.CoreMatchers.is;
import static org.hamcrest.CoreMatchers.not;
import static org.junit.Assert.assertThat;

/**
 * Test NetworkHelper class
 */
public class NetworkHelperTest extends TestCase {

    public void testGetOUICode() throws Exception {
        byte[] address = new byte[]{0x01, 0x02, 0x03};

        int fromString = NetworkHelper.getOUICode("010203");
        int fromMAC = NetworkHelper.getOUICode(address);

        Assert.assertEquals(fromString + " differs from " + fromMAC, fromString, fromMAC);
    }

    public void testCompareInetAddress() throws Exception {
        InetAddress a, b;

        a = InetAddress.getLocalHost();
        b = InetAddress.getByAddress("127.0.0.1", new byte[]{127, 0, 0, 1});

        assertThat(a, is(b));

        int res = NetworkHelper.compareInetAddresses(a, b);

        assertThat(res, is(0));

        b = InetAddress.getByAddress(new byte[]{(byte) 192, (byte) 168, 1, 1});

        assertThat(a, not(b));

        res = NetworkHelper.compareInetAddresses(a, b);

        assertThat(a + " should be less than " + b, res < 0, is(true));
    }
}
