package org.csploit.msf.impl;

import org.csploit.msf.api.Auxiliary;
import org.csploit.msf.api.module.AuxiliaryAction;

/**
 * Give access to Auxiliary modifiers
 */
interface InternalAuxiliary extends Auxiliary, InternalModule {
  void setDefaultAction(String defaultAction);
  void setActions(AuxiliaryAction[] actions);
  void setDefaultAction(int defaultAction);
}
