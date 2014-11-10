package it.evilsocket.dsploit.net.metasploit;

import java.io.IOException;
import java.util.Iterator;
import java.util.Map;

import it.evilsocket.dsploit.core.System;

/**
 * this class store MSF payload metadata
 */
public class Payload {

  private Option[] mOptions = null;
  private String mName = null;


  public Payload(String name){
    mName = name;
    try {
      retrieveOptions();
    } catch (RPCClient.MSFException e) {
      System.errorLogging(e);
    } catch (IOException e) {
      System.errorLogging(e);
    } finally {
      if(mOptions==null)
        mOptions = new Option[0];
    }
  }

  @SuppressWarnings("unchecked")
  private void retrieveOptions() throws IOException, RPCClient.MSFException {
    int i = 0;
    Map<String,Map<String, Object>> map =  (Map<String,Map<String, Object>>) System.getMsfRpc().call("module.options", "payload", mName);
    Iterator<Map.Entry<String,Map<String, Object>>> it = map.entrySet().iterator();
    mOptions = new Option[map.size()];
    while(it.hasNext()) {
      Map.Entry<String,Map<String,Object>> item = it.next();
      String name = item.getKey();
      mOptions[i] = new Option(name,item.getValue());
      if(name.equals("LHOST"))
        mOptions[i].setValue(System.getNetwork().getLocalAddress().getHostAddress());
      i++;
    }
  }

  public Option[] getOptions() {
    return mOptions;
  }

  public String getName() {
    return mName;
  }

  public String toString() {
    return mName;
  }

}
