package org.csploit.msf.impl;

import org.csploit.msf.api.MsfException;
import org.csploit.msf.impl.module.AuxiliaryAction;

import java.io.IOException;

/**
 * The auxiliary class acts as a base class for all modules that perform
 * reconnaissance, retrieve data, brute force logins, or any other action
 * that doesn't fit our concept of an 'exploit' (involving payloads and
 * targets and whatnot).
 */
class Auxiliary extends Module implements org.csploit.msf.api.Auxiliary {

  private AuxiliaryAction[] actions;
  private int defaultAction;

  public Auxiliary(String refname) {
    super(refname);
    defaultAction = -1;
  }

  public void setDefaultAction(String defaultAction) {
    if(actions != null && defaultAction != null && defaultAction.length() > 0) {
      for(int i = 0; i < actions.length; i++) {
        if(defaultAction.equals(actions[i].getName())) {
          this.defaultAction = i;
          return;
        }
      }
    }
  }

  private void onActionsChanged() {
    if(actions == null || defaultAction >= actions.length) {
      defaultAction = -1;
    }
  }

  public void setActions(AuxiliaryAction[] actions) {
    this.actions = actions;
    onActionsChanged();
  }

  public void setDefaultAction(int defaultAction) {
    this.defaultAction = defaultAction;
    onActionsChanged();
  }

  @Override
  public String getType() {
    return "auxiliary";
  }

  @Override
  public org.csploit.msf.api.module.AuxiliaryAction[] getActions() {
    return actions;
  }

  @Override
  public org.csploit.msf.api.module.AuxiliaryAction getDefaultAction() {
    return actions == null || defaultAction < 0 ? null : actions[defaultAction];
  }

  @Override
  public void execute() throws IOException, MsfException {
    throw new UnsupportedOperationException("not implemented yet");
  }
}
