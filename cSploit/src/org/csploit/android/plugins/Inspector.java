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
package org.csploit.android.plugins;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.content.ContextCompat;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.helpers.ThreadHelper;
import org.csploit.android.net.Network;
import org.csploit.android.net.Target;
import org.csploit.android.net.Target.Port;
import org.csploit.android.tools.NMap.InspectionReceiver;

public class Inspector extends Plugin{

  private FloatingActionButton mStartButton = null;
  private ProgressBar mActivity = null;
  private TextView mDeviceType = null;
  private TextView mDeviceOS = null;
  private TextView mDeviceServices = null;
  private boolean mRunning = false;
  private boolean mFocusedScan = false;
  private Receiver mReceiver = null;
  private String empty = null;

  public Inspector(){
    super(
            R.string.inspector,
            R.string.inspector_desc,

            new Target.Type[]{Target.Type.ENDPOINT, Target.Type.REMOTE},
            R.layout.plugin_inspector,
            R.drawable.action_inspect
    );
  }

  private void setStoppedState(){
    if(mProcess!=null) {
      mProcess.kill();
      mProcess = null;
    }

    mActivity.setVisibility(View.INVISIBLE);
    mRunning = false;
    mStartButton.setImageDrawable(ContextCompat.getDrawable(this, R.drawable.ic_play_arrow_24dp));
  }

  private void write_services()
  {
    StringBuilder sb = new StringBuilder();
    String service;
    String version;

    for (Port port : System.getCurrentTarget().getOpenPorts()) {
      service = port.getService();
      version = port.getVersion();

      if (service != null && !service.isEmpty()) {
        sb.append(port.getNumber());
        sb.append(" ( ");
        sb.append(port.getProtocol().toString());
        sb.append(" ) : ");
        sb.append(service);

        if (version != null && !version.isEmpty()) {
          sb.append(" - ");
          sb.append(version);
        }
        sb.append("\n");
      }
    }
    if (sb.length() > 0)
      mDeviceServices.setText(sb.toString());
    else
      mDeviceServices.setText(empty);
  }

  private void updateView() {
    if(ThreadHelper.isOnMainThread()) {
      write_services();
    } else {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          write_services();
        }
      });
    }
  }

  private void setStartedState(){

    try {
      Target target = System.getCurrentTarget();

      updateView();

      mProcess = System.getTools().nmap.inpsect( target, mReceiver, mFocusedScan);
      mStartButton.setImageDrawable(ContextCompat.getDrawable(this, R.drawable.ic_stop_24dp));

      mActivity.setVisibility(View.VISIBLE);
      mRunning = true;
    } catch (ChildManager.ChildNotStartedException e) {
      System.errorLogging(e);
      Toast.makeText(Inspector.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
    }
  }

  @Override
  public void onCreate(Bundle savedInstanceState){
	  SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
		if (isDark)
			setTheme(R.style.DarkTheme);
		else
			setTheme(R.style.AppTheme);
    super.onCreate(savedInstanceState);

    mStartButton = (FloatingActionButton) findViewById(R.id.inspectToggleButton);
    mActivity = (ProgressBar) findViewById(R.id.inspectActivity);
    TextView mDeviceName = (TextView) findViewById(R.id.deviceName);
    mDeviceType = (TextView) findViewById(R.id.deviceType);
    mDeviceOS = (TextView) findViewById(R.id.deviceOS);
    mDeviceServices = (TextView) findViewById(R.id.deviceServices);

    mFocusedScan = System.getCurrentTarget().hasOpenPorts();

    mDeviceName.setText(System.getCurrentTarget().toString());

    if(System.getCurrentTarget().getDeviceType() != null)
      mDeviceType.setText(System.getCurrentTarget().getDeviceType());

    if(System.getCurrentTarget().getDeviceOS() != null)
      mDeviceOS.setText(System.getCurrentTarget().getDeviceOS());

    empty = getText(R.string.unknown).toString();

    // yep, we're on main thread here
    write_services();

    mStartButton.setOnClickListener(new OnClickListener(){
      @Override
      public void onClick(View v){
        if(mRunning){
          setStoppedState();
        } else{
          setStartedState();
        }
      }
    }
    );

    mReceiver = new Receiver(System.getCurrentTarget());
  }

  @Override
  public void onBackPressed(){
    setStoppedState();
    super.onBackPressed();
    overridePendingTransition(R.anim.fadeout, R.anim.fadein);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu){
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.inspector, menu);
    return super.onCreateOptionsMenu(menu);
  }

  @Override
  public boolean onPrepareOptionsMenu(Menu menu) {
    MenuItem item = menu.findItem(R.id.focused_scan);
    if(item != null) {
      item.setChecked(mFocusedScan);
      item.setEnabled(System.getCurrentTarget().hasOpenPorts());
    }
    return super.onPrepareOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    switch (item.getItemId()) {
      case R.id.focused_scan:
        if(item.isChecked()) {
          item.setChecked(false);
          mFocusedScan =false;
        } else {
          item.setChecked(true);
          mFocusedScan =true;
        }
        return true;
      default:
        return super.onOptionsItemSelected(item);
    }
  }

  private class Receiver extends InspectionReceiver{

    private final Target target;

    public Receiver(Target target) {
      this.target = target;
    }

    @Override
    public void onServiceFound(int port, String protocol, String service, String version ){
      Port p = new Port(port, Network.Protocol.fromString(protocol));

      service = service.trim();
      service = service.isEmpty() ? null : service;

      version = version != null ? version.trim() : null;
      version = version != null && !version.isEmpty() ? version : null;

      p.setService(service);
      p.setVersion(version);

      target.addOpenPort(p);

      updateView();
    }

    @Override
    public void onOpenPortFound(int port, String protocol){
      target.addOpenPort(port, Network.Protocol.fromString(protocol));
    }

    @Override
    public void onOsFound(final String os){
      target.setDeviceOS(os);

      Inspector.this.runOnUiThread(new Runnable(){
        @Override
        public void run(){
          mDeviceOS.setText(os);
        }
      });
    }

    @Override
    public void onDeviceFound(final String device){
      target.setDeviceType(device);

      Inspector.this.runOnUiThread(new Runnable(){
        @Override
        public void run(){
          mDeviceType.setText(device);
        }
      });
    }

    @Override
    public void onEnd(int code){
      Inspector.this.runOnUiThread(new Runnable(){
        @Override
        public void run(){
          if(mRunning)
            setStoppedState();
        }
      });
		}
	}
}
