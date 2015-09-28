/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * dSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android.plugins.mitm;

import org.csploit.android.R;
import org.csploit.android.core.Child;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.System;
import org.csploit.android.net.Target;
import org.csploit.android.tools.ArpSpoof;
import org.csploit.android.tools.Ettercap.OnAccountListener;
import org.csploit.android.tools.Ettercap.OnDNSSpoofedReceiver;


public class SpoofSession
{
  private boolean mWithProxy = false;
  private boolean mWithServer = false;
  private String mServerFileName = null;
  private String mServerMimeType = null;
  private Child mArpSpoofProcess = null;
  private Child mEttercapProcess = null;

  public interface OnSessionReadyListener{
    void onSessionReady();
    void onError(String errorString, int errorStringId);
  }

  public SpoofSession(boolean withProxy, boolean withServer, String serverFileName, String serverMimeType){
    mWithProxy = withProxy;
    mWithServer = withServer;
    mServerFileName = serverFileName;
    mServerMimeType = serverMimeType;
  }

  public SpoofSession(){
    // Standard spoof session with only transparent proxy
    this(true, false, null, null);
  }

  public void start(Target target, final OnSessionReadyListener listener) throws ChildManager.ChildNotStartedException {
    this.stop();

    if(mWithProxy){
      if(System.getProxy() == null){
        listener.onError( null, R.string.error_mitm_proxy );
        return;
      }

      if(System.getSettings().getBoolean("PREF_HTTPS_REDIRECT", true)){
        if(System.getHttpsRedirector() == null){
          listener.onError( null, R.string.error_mitm_https_redirector );
          return;
        }
        new Thread(System.getHttpsRedirector()).start();
      }

      new Thread(System.getProxy()).start();
    }

    if(mWithServer){
      try{
        if(System.getServer() == null){
          listener.onError( null, R.string.error_mitm_resource_server );
          return;
        }

        System.getServer().setResource(mServerFileName, mServerMimeType);
        new Thread(System.getServer()).start();
      } catch(Exception e){
        System.errorLogging(e);
        mWithServer = false;
      }
    }

    mArpSpoofProcess = System.getTools().arpSpoof.spoof(target, new ArpSpoof.ArpSpoofReceiver() {
      @Override
      public void onError(String line) {
        listener.onError(line, 0);
      }
    });

    System.setForwarding(true);

    if (mWithProxy) {
      System.getTools().ipTables.portRedirect(80, System.HTTP_PROXY_PORT, true);

      if (System.getSettings().getBoolean("PREF_HTTPS_REDIRECT", true))
        System.getTools().ipTables.portRedirect(443, System.HTTPS_REDIR_PORT, false);
    }

    listener.onSessionReady();
  }

  public void start(final Target target, final OnAccountListener listener) throws ChildManager.ChildNotStartedException {

    this.stop();

    mArpSpoofProcess =
    System.getTools().arpSpoof.spoof(target, new ArpSpoof.ArpSpoofReceiver() {
      @Override
      public void onError(String line) {
        SpoofSession.this.stop();
      }
    });

    //System.setForwarding(true);


    try {
      mEttercapProcess = System.getTools().ettercap.dissect(target, listener);
    } catch (ChildManager.ChildNotStartedException e) {
      this.stop();
      throw e;
    }
  }

  public void start(final OnDNSSpoofedReceiver listener) throws ChildManager.ChildNotStartedException {

    mArpSpoofProcess =
            System.getTools().arpSpoof.spoof(System.getCurrentTarget(), new ArpSpoof.ArpSpoofReceiver() {
              @Override
              public void onError(String line) {
                SpoofSession.this.stop();
                listener.onError(line);
              }
            });


    try {
      mEttercapProcess = System.getTools().ettercap.dnsSpoof(System.getCurrentTarget(), listener);
    } catch (ChildManager.ChildNotStartedException e) {
      this.stop();
      throw e;
    }

    System.setForwarding(true);
  }

  public void start(final OnSessionReadyListener listener) throws ChildManager.ChildNotStartedException {
    this.start(System.getCurrentTarget(), listener);
  }

  public void start(OnAccountListener listener) throws ChildManager.ChildNotStartedException {
    this.start(System.getCurrentTarget(), listener);
  }

  public void stop(){
    if(mArpSpoofProcess!=null) {
      mArpSpoofProcess.kill(2);
      mArpSpoofProcess = null;
    }
    if(mEttercapProcess!=null) {
      mEttercapProcess.kill(2);
      mEttercapProcess = null;
    }
    System.setForwarding(false);

    if(mWithProxy){
      System.getTools().ipTables.undoPortRedirect(80, System.HTTP_PROXY_PORT);
      if(System.getSettings().getBoolean("PREF_HTTPS_REDIRECT", true)){
        System.getTools().ipTables.undoPortRedirect(443, System.HTTPS_REDIR_PORT);

        if(System.getHttpsRedirector() != null)
          System.getHttpsRedirector().stop();
      }

      if(System.getProxy() != null){
        System.getProxy().stop();
        System.getProxy().setRedirection(null, 0);
        System.getProxy().setFilter(null);
      }
    }

    if(mWithServer && System.getServer() != null)
      System.getServer().stop();
  }
}
