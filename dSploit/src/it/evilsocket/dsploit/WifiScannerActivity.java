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
package it.evilsocket.dsploit;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Typeface;
import android.net.wifi.ScanResult;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.text.ClipboardManager;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.actionbarsherlock.app.SherlockListActivity;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import it.evilsocket.dsploit.core.ManagedReceiver;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.gui.dialogs.WifiCrackDialog;
import it.evilsocket.dsploit.gui.dialogs.WifiCrackDialog.WifiCrackDialogListener;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.wifi.Keygen;
import it.evilsocket.dsploit.wifi.NetworkManager;
import it.evilsocket.dsploit.wifi.WirelessMatcher;

import static android.net.wifi.WifiManager.EXTRA_NEW_STATE;
import static android.net.wifi.WifiManager.SCAN_RESULTS_AVAILABLE_ACTION;
import static android.net.wifi.WifiManager.SUPPLICANT_STATE_CHANGED_ACTION;

@SuppressWarnings("deprecation")
public class WifiScannerActivity extends SherlockListActivity{
  public static final String CONNECTED = "WifiScannerActivity.CONNECTED";
  private static final String TAG = "WIFISCANNER";
  private WifiManager mWifiManager = null;
  private WirelessMatcher mWifiMatcher = null;
  private TextView mStatusText = null;
  private ScanReceiver mScanReceiver = null;
  private ConnectionReceiver mConnectionReceiver = null;
  private ScanAdapter mAdapter = null;
  private boolean mConnected = false;
  private boolean mScanning = false;
  private Menu mMenu = null;
  private ScanResult mCurrentAp = null;
  private List<String> mKeyList = null;
  private String mCurrentKey = null;
  private int mCurrentNetworkId = -1;
  private ClipboardManager mClipboard = null;

  public void onSuccessfulConnection(){
    List<WifiConfiguration> configurations = mWifiManager.getConfiguredNetworks();
    if(configurations != null){
      for(WifiConfiguration config : configurations){
        mWifiManager.enableNetwork(config.networkId, false);
      }
    }

    if(mCurrentKey != null){
      mStatusText.setText(Html.fromHtml(getString(R.string.connected_to) + mCurrentAp.SSID + getString(R.string.connected_to2) + mCurrentKey + getString(R.string.connected_to3)));
      Toast.makeText(this, getString(R.string.wifi_key_copied), Toast.LENGTH_SHORT).show();
      mClipboard.setText(mCurrentKey);
    } else
      mStatusText.setText(Html.fromHtml(getString(R.string.connected_to) + mCurrentAp.SSID + "</b> !"));

    mConnectionReceiver.unregister();
    mConnected = true;
  }

  public void onFailedConnection(){
    mWifiManager.removeNetwork(mCurrentNetworkId);
    if(mKeyList.size() == 0){
      mStatusText.setText(Html.fromHtml(getString(R.string.connection_to) + mCurrentAp.SSID + getString(R.string.connection_to2)));

      List<WifiConfiguration> configurations = mWifiManager.getConfiguredNetworks();
      if(configurations != null){
        for(WifiConfiguration config : configurations){
          mWifiManager.enableNetwork(config.networkId, false);
        }
      }

      mConnectionReceiver.unregister();
    } else
      nextConnectionAttempt();
  }

  @Override
  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    setContentView(R.layout.wifi_scanner);
    getSupportActionBar().setDisplayHomeAsUpEnabled(true);

    mWifiManager = (WifiManager) this.getSystemService(Context.WIFI_SERVICE);
    mClipboard = (ClipboardManager) this.getSystemService(Context.CLIPBOARD_SERVICE);
    mWifiMatcher = new WirelessMatcher(getResources().openRawResource(R.raw.alice));
    mScanReceiver = new ScanReceiver();
    mConnectionReceiver = new ConnectionReceiver();
    mStatusText = (TextView) findViewById(R.id.statusText);
    mAdapter = new ScanAdapter();
    mKeyList = new ArrayList<String>();

    getListView().setAdapter(mAdapter);

    mStatusText.setText("Initializing ...");

    if(!mWifiManager.isWifiEnabled()){
      mStatusText.setText("Activating WiFi interface ...");
      mWifiManager.setWifiEnabled(true);
      mStatusText.setText("WiFi activated.");
    }

    if(Network.isWifiConnected(this)){
      WifiInfo info = mWifiManager.getConnectionInfo();
      NetworkManager.cleanPreviousConfiguration(mWifiManager, info);
    }

    mScanReceiver.register(this);

    if(mMenu != null)
      mMenu.findItem(R.id.scan).setActionView(new ProgressBar(this));

    mStatusText.setText("Scanning ...");
    mScanning = true;

    mWifiManager.startScan();
  }

  private int performConnection(final ScanResult ap, final String key){
    mWifiManager.disconnect();

    mCurrentKey = key;
    mCurrentAp = ap;

    WifiScannerActivity.this.runOnUiThread(new Runnable(){
      @Override
      public void run(){
        if(key != null)
          mStatusText.setText(Html.fromHtml("Attempting connection to <b>" + ap.SSID + "</b> with key <b>" + key + "</b> ..."));
        else
          mStatusText.setText(Html.fromHtml("Connecting to <b>" + ap.SSID + "</b> ..."));
      }
    });

    WifiConfiguration config = new WifiConfiguration();
    int network = -1;

    config.SSID = "\"" + ap.SSID + "\"";
    config.BSSID = ap.BSSID;

		/*
         * Configure security.
		 */
    if(ap.capabilities.contains("WEP")){
      config.wepKeys[0] = "\"" + key + "\"";
      config.wepTxKeyIndex = 0;
      config.status = WifiConfiguration.Status.ENABLED;
      config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
      config.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.WEP40);
    } else if(ap.capabilities.contains("WPA"))
      config.preSharedKey = "\"" + key + "\"";

    else
      config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);

    network = mWifiManager.addNetwork(config);
    if(network != -1){
      if(mWifiManager.saveConfiguration()){
        config = NetworkManager.getWifiConfiguration(mWifiManager, config);

        // Make it the highest priority.
        network = config.networkId;

        int old_priority = config.priority,
          max_priority = NetworkManager.getMaxPriority(mWifiManager) + 1;

        if(max_priority > 9999){
          NetworkManager.shiftPriorityAndSave(mWifiManager);
          config = NetworkManager.getWifiConfiguration(mWifiManager, config);
        }

        // Set highest priority to this configured network
        config.priority = max_priority;
        network = mWifiManager.updateNetwork(config);

        if(network != -1){
          // Do not disable others
          if(mWifiManager.enableNetwork(network, false)){
            if(mWifiManager.saveConfiguration()){
              // We have to retrieve the WifiConfiguration after save.
              config = NetworkManager.getWifiConfiguration(mWifiManager, config);
              if(config != null){
                // Disable others, but do not save.
                // Just to force the WifiManager to connect to it.
                if(mWifiManager.enableNetwork(config.networkId, true)){
                  return mWifiManager.reassociate() ? config.networkId : -1;
                }
              }
            } else
              config.priority = old_priority;
          } else
            config.priority = old_priority;
        }
      }
    }

    return network;
  }

  private void nextConnectionAttempt(){
    if(mKeyList.size() > 0){
      mCurrentKey = mKeyList.get(0);

      mKeyList.remove(0);

      mCurrentNetworkId = performConnection(mCurrentAp, mCurrentKey);
      if(mCurrentNetworkId != -1)
        mConnectionReceiver.register(this);

      else
        mConnectionReceiver.unregister();
    } else
      mConnectionReceiver.unregister();
  }

  private void performCracking(final Keygen keygen, final ScanResult ap){

    final ProgressDialog dialog = ProgressDialog.show(this, "", getString(R.string.generating_keys), true, false);

    new Thread(new Runnable(){
      @Override
      public void run(){
        dialog.show();

        try{
          List<String> keys = keygen.getKeys();

          if(keys == null || keys.size() == 0){
            WifiScannerActivity.this.runOnUiThread(new Runnable(){
              @Override
              public void run(){
                new ErrorDialog("Error", keygen.getErrorMessage().isEmpty() ? "Could not generate keys." : keygen.getErrorMessage(), WifiScannerActivity.this).show();
              }
            });
          } else{
            mCurrentAp = ap;
            mKeyList = keys;

            nextConnectionAttempt();
          }
        } catch(Exception e){
          System.errorLogging(TAG, e);
        } finally{
          dialog.dismiss();
        }
      }
    }).start();
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id){
    super.onListItemClick(l, v, position, id);

    final ScanResult result = mAdapter.getItem(position);
    if(result != null){
      final Keygen keygen = mWifiMatcher.getKeygen(result);

      if(keygen != null && (result.capabilities.contains("WEP") || result.capabilities.contains("WPA"))){
        mKeyList.clear();
        new WifiCrackDialog
          (
            result.SSID,
            getString(R.string.enter_key_or_crack),
            this,
            new WifiCrackDialogListener(){
              @Override
              public void onManualConnect(String key){
                mCurrentNetworkId = performConnection(result, key);
                if(mCurrentNetworkId != -1)
                  mConnectionReceiver.register(WifiScannerActivity.this);

                else
                  mConnectionReceiver.unregister();
              }

              @Override
              public void onCrack(){
                performCracking(keygen, result);
              }
            }
          ).show();
      } else{
        if(result.capabilities.contains("WEP") || result.capabilities.contains("WPA")){
          new InputDialog(result.SSID, getString(R.string.enter_wifi_key), null, true, true, this, new InputDialogListener(){
            @Override
            public void onInputEntered(String input){
              mCurrentNetworkId = performConnection(result, input);
              if(mCurrentNetworkId != -1)
                mConnectionReceiver.register(WifiScannerActivity.this);
              else
                mConnectionReceiver.unregister();
            }
          }).show();
        } else
          performConnection(result, null);
      }
    }
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu){
    mMenu = menu;
    MenuInflater inflater = getSupportMenuInflater();
    inflater.inflate(R.menu.wifi_scanner, menu);

    if(mScanning)
      mMenu.findItem(R.id.scan).setActionView(new ProgressBar(this));

    return super.onCreateOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    if(item.getItemId() == R.id.scan){
      if(mMenu != null)
        mMenu.findItem(R.id.scan).setActionView(new ProgressBar(this));

      mWifiManager.startScan();

      mStatusText.setText(getString(R.string.scanning));
      mScanning = true;

      return true;
    }
    if(item.getItemId() == android.R.id.home){
      onBackPressed();

      return true;
    } else
      return super.onOptionsItemSelected(item);
  }

  @Override
  public void onBackPressed(){
    mScanReceiver.unregister();
    mConnectionReceiver.unregister();

    Bundle bundle = new Bundle();
    bundle.putBoolean(CONNECTED, mConnected);

    Intent intent = new Intent();
    intent.putExtras(bundle);

    setResult(RESULT_OK, intent);

    super.onBackPressed();
    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
  }

  public class ScanAdapter extends ArrayAdapter<ScanResult>{
    private ArrayList<ScanResult> mResults = null;

    public ScanAdapter(){
      super(WifiScannerActivity.this, R.layout.wifi_scanner_list_item);

      mResults = new ArrayList<ScanResult>();
    }

    public void addResult(ScanResult result){
      for(ScanResult res : mResults){
        if(res.BSSID.equals(result.BSSID))
          return;
      }

      mResults.add(result);

      Collections.sort(mResults, new Comparator<ScanResult>(){
        @Override
        public int compare(ScanResult lhs, ScanResult rhs){
          if(lhs.level > rhs.level)
            return -1;

          else if(rhs.level > lhs.level)
            return 1;

          else
            return 0;
        }
      });
    }

    public void reset(){
      mResults.clear();
      notifyDataSetChanged();
    }

    @Override
    public int getCount(){
      return mResults.size();
    }

    @Override
    public ScanResult getItem(int position){
      return mResults.get(position);
    }

    public int getWifiIcon(ScanResult wifi){
      int level = Math.abs(wifi.level);

      if(wifi.capabilities.contains("WPA") || wifi.capabilities.contains("WEP")){
        if(level <= 76)
          return R.drawable.ic_wifi_lock_signal_4;

        else if(level <= 87)
          return R.drawable.ic_wifi_lock_signal_3;

        else if(level <= 98)
          return R.drawable.ic_wifi_lock_signal_2;

        else
          return R.drawable.ic_wifi_lock_signal_1;
      } else{
        if(level <= 76)
          return R.drawable.ic_wifi_signal_4;

        else if(level <= 87)
          return R.drawable.ic_wifi_signal_3;

        else if(level <= 98)
          return R.drawable.ic_wifi_signal_2;

        else
          return R.drawable.ic_wifi_signal_1;
      }
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent){
      View row = convertView;
      ResultHolder holder = null;

      if(row == null){
        LayoutInflater inflater = (LayoutInflater) WifiScannerActivity.this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        row = inflater.inflate(R.layout.wifi_scanner_list_item, parent, false);

        holder = new ResultHolder();

        holder.supported = (ImageView) (row != null ? row.findViewById(R.id.supported) : null);
        holder.powerIcon = (ImageView) (row != null ? row.findViewById(R.id.powerIcon) : null);
        holder.ssid = (TextView) (row != null ? row.findViewById(R.id.ssid) : null);
        holder.bssid = (TextView) (row != null ? row.findViewById(R.id.bssid) : null);

        if(row != null)
          row.setTag(holder);

      } else{
        holder = (ResultHolder) row.getTag();
      }

      ScanResult result = mResults.get(position);

      holder.powerIcon.setImageResource(getWifiIcon(result));
      holder.ssid.setTypeface(null, Typeface.BOLD);
      holder.ssid.setText(result.SSID);

      String protection = "<b>Open</b>";
      boolean isOpen = true;

      List<String> capabilities = Arrays.asList(result.capabilities.split("[\\-\\[\\]]"));

      if(capabilities.contains("WEP")){
        isOpen = false;
        protection = "<b>WEP</b>";
      } else if(capabilities.contains("WPA2")){
        isOpen = false;
        protection = "<b>WPA2</b>";
      } else if(capabilities.contains("WPA")){
        isOpen = false;
        protection = "<b>WPA</b>";
      }

      if(capabilities.contains("PSK"))
        protection += " PSK";

      if(capabilities.contains("WPS"))
        protection += " ( WPS )";

      holder.bssid.setText(Html.fromHtml(
        result.BSSID.toUpperCase() + " " + protection + " <small>( " + (Math.round((result.frequency / 1000.0) * 10.0) / 10.0) + " Ghz )</small>")
      );

      if(mWifiMatcher.getKeygen(result) != null || isOpen)
        holder.supported.setImageResource(R.drawable.ic_possible);

      else
        holder.supported.setImageResource(R.drawable.ic_impossible);


      return row;
    }

    class ResultHolder{
      ImageView supported;
      ImageView powerIcon;
      TextView ssid;
      TextView bssid;
    }
  }

  private class ScanReceiver extends ManagedReceiver{
    private IntentFilter mFilter = null;

    public ScanReceiver(){
      mFilter = new IntentFilter(SCAN_RESULTS_AVAILABLE_ACTION);
    }

    public IntentFilter getFilter(){
      return mFilter;
    }

    @SuppressWarnings("ConstantConditions")
    @Override
    public void onReceive(Context context, Intent intent){
      if(intent.getAction().equals(SCAN_RESULTS_AVAILABLE_ACTION)){
        if(mScanning){
          mAdapter.reset();

          if(mMenu != null)
            mMenu.findItem(R.id.scan).setActionView(null);

          List<ScanResult> results = mWifiManager.getScanResults();

          for(ScanResult result : results){
            mAdapter.addResult(result);
          }

          mScanning = false;
          mStatusText.setText("Scanning finished.");
        }

        mAdapter.notifyDataSetChanged();
      }
    }
  }

  private class ConnectionReceiver extends ManagedReceiver{
    private IntentFilter mFilter = null;

    public ConnectionReceiver(){
      mFilter = new IntentFilter(SUPPLICANT_STATE_CHANGED_ACTION);
    }

    public IntentFilter getFilter(){
      return mFilter;
    }

    @Override
    public void onReceive(Context context, Intent intent){
      SupplicantState state = intent.getParcelableExtra(EXTRA_NEW_STATE);

      if(state != null){
        if(state.equals(SupplicantState.COMPLETED)){
          onSuccessfulConnection();
        } else if(state.equals(SupplicantState.DISCONNECTED)){
          onFailedConnection();
        }
      }
    }
  }
}
