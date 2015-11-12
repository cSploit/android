package org.csploit.msf.impl;

import org.csploit.msf.impl.module.PlatformListTest;
import org.csploit.msf.impl.module.PlatformTest;
import org.junit.runner.RunWith;
import org.junit.runners.Suite;

/**
 * A suite of offline tests
 */
@RunWith(Suite.class)
@Suite.SuiteClasses({
  DataStoreTest.class,
  MsgpackLoaderTest.class,
  NameHelperTest.class,
  RpcSampleTest.class,
  PlatformListTest.class,
  PlatformTest.class,
  SessionManagerTest.class,
  EventManagerTest.class
})
public class OfflineTestSuite {
}
