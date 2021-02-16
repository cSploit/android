package org.csploit.msf.impl;

import org.csploit.msf.api.sessions.Meterpreter;

/**
 * Access meterpreter modifiers
 */
interface InternalMeterpreter extends Meterpreter {
  void setPlatform(String platform);
}
