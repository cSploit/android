package org.csploit.android.services;

import android.content.Context;

/**
 * Services provider
 */
public final class Services {
  private static Context context;
  private static NetworkRadar networkRadar;
  private static MsfRpcdService msfRpcdService;
  private static UpdateService updateService;

  public static void init(Context context) {
    Services.context = context;
  }

  public synchronized static NetworkRadar getNetworkRadar() {
    if(networkRadar == null) {
      networkRadar = new NetworkRadar(context);
    }
    return networkRadar;
  }

  public synchronized static MsfRpcdService getMsfRpcdService() {
    if(msfRpcdService == null) {
      msfRpcdService = new MsfRpcdService(context);
    }
    return msfRpcdService;
  }

  public synchronized static UpdateService getUpdateService() {
    if(updateService == null) {
      updateService = new UpdateService(context);
    }
    return updateService;
  }
}
