package org.csploit.msf;

/**
 * CLasses that ave a reference to the framework
 */
interface Offspring {
  boolean haveFramework();
  Framework getFramework();
  void setFramework(Framework framework);
}
