package org.csploit.android.net.metasploit;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Map;
import java.util.regex.Pattern;

import org.csploit.android.R;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.net.Target;

/**
 * Store info about an opened MSF session
 * References: https://github.com/rapid7/metasploit-framework/blob/master/lib/msf/core/rpc/v10/rpc_session.rb#L14
 */
public class Session extends Thread {

  protected final static Pattern WINDOWS_HASH = Pattern.compile("^[^:]+:[0-9]+:[a-f0-9]+:[a-f0-9]+:");
  protected final static Pattern WINDOWS_GET_NTLM_HASH = Pattern.compile("^[^:]+:[0-9]+:[a-f0-9]+:([a-f0-9]+):");
  protected final static Pattern WINDOWS_GET_LM_HASH = Pattern.compile("^[^:]+:[0-9]+:([a-f0-9]+):[a-f0-9]+:");

  protected final static Pattern LINUX_HASH = Pattern.compile("^([^:]+):\\$[123456][axy]?(\\$round=[0-9]+)?(\\$[a-zA-Z0-9]+)?\\$([^:]+):");
  protected final static Pattern LINUX_GET_HASH = Pattern.compile("^[^:]+:(\\$[^:]+):");
  protected final static Pattern LINUX_GET_USERNAME = Pattern.compile("^([^:]+):");

  protected final int mJobId;
  protected final String mType;
  protected final MsfExploit mExploit;
  protected final Payload mPayload;
  protected final String mDescription;
  protected final String mInfo;
  protected final String mHost;
  protected final int mPort;
  protected final String mTargetHost;
  protected final String mUsername;
  protected final String mUuid;
  protected final Target mTarget;
  protected boolean mRunning = true;


  public Session(Integer id, Map<String,Object> map) throws UnknownHostException {
    String via_exploit = (String)map.get("via_exploit");
    String via_payload = (String)map.get("via_payload");
    Target tTmp = null;
    MsfExploit eTmp = null;
    Payload pTmp = null;

    mJobId = id;
    mType = (String)map.get("type");
    mDescription = (String)map.get("desc");
    mInfo = (String)map.get("info");
    mHost = (String)map.get("session_host");
    mPort = (Integer)map.get("session_port");
    mTargetHost = (String)map.get("target_host");
    mUsername = (String)map.get("username");
    mUuid = (String)map.get("uuid");

    // Search my Target
    tTmp = System.getTargetByAddress(mTargetHost);
    if(tTmp == null)
      throw new UnknownHostException("no target matches `"+mTargetHost+"'");
    mTarget = tTmp;

    // Search my exploit
    if(via_exploit!=null && !via_exploit.isEmpty()) {
      for(Target.Exploit e : mTarget.getExploits()) {
        if(e instanceof  MsfExploit && e.getName().equals(via_exploit)) {
          eTmp = (MsfExploit)e;
          break;
        }
      }
    }
    mExploit = eTmp;

    // Search my payload
    if(mExploit!=null && via_payload != null && !via_payload.isEmpty()) {
      for(Payload p : mExploit.getPayloads())
        if(p.getName().equals(via_payload)) {
          pTmp = p;
          break;
        }
    }
    mPayload = pTmp;
  }

  public void stopSession() {
    try {
      synchronized (this) {
        // skip if already stopped
        if(!mRunning)
          return;

        // require msfrpcd to stop the session
        RPCClient client = System.getMsfRpc();
        if(client!=null)
          client.call("session.stop",mJobId);

        // interrupt myself
        if(isAlive())
          this.interrupt();
        mRunning=false;
      }
    } catch (RPCClient.MSFException e) {
      Logger.warning("cannot stop session #"+mJobId);
      System.errorLogging(e);
    } catch (IOException e) {
      Logger.warning("cannot stop session #" + mJobId);
      System.errorLogging(e);
    }
  }

  public boolean equals(Object o) {
    return (o!=null && o.getClass() == this.getClass() && ((Session)o).getUuid().equals(this.mUuid));
  }

  public String toString() {
    return mDescription;
  }

  public MsfExploit getExploit() {
    return mExploit;
  }

  public Payload getPayload() {
    return mPayload;
  }

  public Target getTarget() {
    return mTarget;
  }

  public String getUuid() {
    return mUuid;
  }

  public String getType() {
    return mType;
  }

  public String getDescription() {
    return mDescription;
  }

  public String getInfo() {
    return mInfo;
  }

  public int getResourceId() {
    return R.drawable.action_session;
  }

  // nothing to do here
  public void run() {
  }

  public boolean haveShell() {
    return false;
  }

  public boolean isMeterpreter() {
    return false;
  }
}
