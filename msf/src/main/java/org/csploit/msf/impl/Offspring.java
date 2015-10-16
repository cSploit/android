package org.csploit.msf.impl;

/**
 * CLasses that ave a reference to the framework
 */
interface Offspring {
  boolean haveFramework();
  Framework getFramework();
  void setFramework(Framework framework);
}
