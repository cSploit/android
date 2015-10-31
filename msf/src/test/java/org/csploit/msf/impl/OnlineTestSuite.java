package org.csploit.msf.impl;

import org.junit.runner.RunWith;
import org.junit.runners.Suite;

/**
 * Perform some tests that require a MFS connection
 */
@RunWith(Suite.class)
@Suite.SuiteClasses({
        MsgpackTest.class
})
public class OnlineTestSuite {
}
