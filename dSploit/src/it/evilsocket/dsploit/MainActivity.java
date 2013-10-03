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

import android.annotation.SuppressLint;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences.Editor;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.text.Html;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.actionbarsherlock.app.SherlockListActivity;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;
import com.bugsense.trace.BugSenseHandler;

import java.io.IOException;
import java.util.ArrayList;

import it.evilsocket.dsploit.core.ManagedReceiver;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.ToolsInstaller;
import it.evilsocket.dsploit.core.UpdateChecker;
import it.evilsocket.dsploit.gui.dialogs.AboutDialog;
import it.evilsocket.dsploit.gui.dialogs.ChangelogDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.FatalDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.gui.dialogs.SpinnerDialog;
import it.evilsocket.dsploit.gui.dialogs.SpinnerDialog.SpinnerDialogListener;
import it.evilsocket.dsploit.net.Endpoint;
import it.evilsocket.dsploit.net.metasploit.RPCServer;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.NetworkDiscovery;
import it.evilsocket.dsploit.net.Target;

import static it.evilsocket.dsploit.core.UpdateChecker.AVAILABLE_VERSION;
import static it.evilsocket.dsploit.core.UpdateChecker.UPDATE_AVAILABLE;
import static it.evilsocket.dsploit.core.UpdateChecker.UPDATE_CHECKING;
import static it.evilsocket.dsploit.core.UpdateChecker.UPDATE_NOT_AVAILABLE;
import static it.evilsocket.dsploit.net.NetworkDiscovery.ENDPOINT_ADDRESS;
import static it.evilsocket.dsploit.net.NetworkDiscovery.ENDPOINT_HARDWARE;
import static it.evilsocket.dsploit.net.NetworkDiscovery.ENDPOINT_NAME;
import static it.evilsocket.dsploit.net.NetworkDiscovery.ENDPOINT_UPDATE;
import static it.evilsocket.dsploit.net.NetworkDiscovery.NEW_ENDPOINT;

@SuppressLint("NewApi")
public class MainActivity extends SherlockListActivity
{
  private String NO_WIFI_UPDATE_MESSAGE;
  private static final int WIFI_CONNECTION_REQUEST = 1012;
  private boolean isWifiAvailable = false;
  private TargetAdapter mTargetAdapter = null;
  private NetworkDiscovery mNetworkDiscovery = null;
  private EndpointReceiver mEndpointReceiver = null;
  private UpdateReceiver mUpdateReceiver = null;
  private RPCServer mRPCServer = null;
  private MsfrpcdReceiver  mMsfRpcdReceiver	= null;
  private Menu mMenu = null;
  private TextView mUpdateStatus = null;
  private Toast mToast = null;
  private long mLastBackPressTime = 0;

  private void createUpdateLayout(){

    getListView().setVisibility(View.GONE);
    findViewById(R.id.textView).setVisibility(View.GONE);

    RelativeLayout layout = (RelativeLayout) findViewById(R.id.layout);
    LayoutParams params = new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT);

    mUpdateStatus = new TextView(this);

    mUpdateStatus.setGravity(Gravity.CENTER);
    mUpdateStatus.setLayoutParams(params);
    mUpdateStatus.setText(NO_WIFI_UPDATE_MESSAGE.replace("#STATUS#", "..."));

    layout.addView(mUpdateStatus);

    if(mUpdateReceiver == null)
      mUpdateReceiver = new UpdateReceiver();

    if( mMsfRpcdReceiver == null)
      mMsfRpcdReceiver = new MsfrpcdReceiver();

    mUpdateReceiver.unregister();
    mMsfRpcdReceiver.unregister();

    mUpdateReceiver.register(MainActivity.this);

    startUpdateChecker();
    stopNetworkDiscovery(true);

    invalidateOptionsMenu();
  }

  private void createOfflineLayout(){

    getListView().setVisibility(View.GONE);
    findViewById(R.id.textView).setVisibility(View.GONE);

    RelativeLayout layout = (RelativeLayout) findViewById(R.id.layout);
    LayoutParams params = new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT);

    mUpdateStatus = new TextView(this);

    mUpdateStatus.setGravity(Gravity.CENTER);
    mUpdateStatus.setLayoutParams(params);
    mUpdateStatus.setText(getString(R.string.no_connectivity));

    layout.addView(mUpdateStatus);

    stopNetworkDiscovery(true);

    invalidateOptionsMenu();
  }

  public void createOnlineLayout(){
    mTargetAdapter = new TargetAdapter();

    setListAdapter(mTargetAdapter);

    getListView().setOnItemLongClickListener(new OnItemLongClickListener(){
      @Override
      public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id){
        final Target target = System.getTarget(position);

        new InputDialog
          (
            getString(R.string.target_alias),
            getString(R.string.set_alias),
            target.hasAlias() ? target.getAlias() : "",
            true,
            false,
            MainActivity.this,
            new InputDialogListener(){
              @Override
              public void onInputEntered(String input){
                target.setAlias(input);
                mTargetAdapter.notifyDataSetChanged();
              }
            }
          ).show();

        return false;
      }
    });

    if(mEndpointReceiver == null)
      mEndpointReceiver = new EndpointReceiver();

    if(mUpdateReceiver == null)
      mUpdateReceiver = new UpdateReceiver();

    if( mMsfRpcdReceiver == null)
      mMsfRpcdReceiver = new MsfrpcdReceiver();

    mEndpointReceiver.unregister();
    mUpdateReceiver.unregister();
    mMsfRpcdReceiver.unregister();

    mEndpointReceiver.register(MainActivity.this);
    mUpdateReceiver.register(MainActivity.this);
    mMsfRpcdReceiver.register( MainActivity.this);

    startUpdateChecker();
    startNetworkDiscovery(false);
    StartMsfRpcd();

    // if called for the second time after wifi connection
    invalidateOptionsMenu();
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent intent){
    if(requestCode == WIFI_CONNECTION_REQUEST && resultCode == RESULT_OK && intent.hasExtra(WifiScannerActivity.CONNECTED)){
      System.reloadNetworkMapping();
      onCreate(null);
    }
  }

  @Override
  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    setContentView(R.layout.target_layout);
    NO_WIFI_UPDATE_MESSAGE = getString(R.string.no_wifi_available);
    isWifiAvailable = Network.isWifiConnected(this);
    boolean connectivityAvailable = isWifiAvailable || Network.isConnectivityAvailable(this);

    // make sure system object was correctly initialized during application startup
    if(!System.isInitialized()){
      // wifi available but system failed to initialize, this is a fatal :(
      if(isWifiAvailable){
        new FatalDialog(getString(R.string.initialization_error), System.getLastError(), this).show();
        return;
      }
    }

    // initialization ok, but wifi is down
    if(!isWifiAvailable){
      // just inform the user his wifi is down
      if(connectivityAvailable)
        createUpdateLayout();

       // no connectivity at all
      else
        createOfflineLayout();
    }
    // we are online, and the system was already initialized
    else if(mTargetAdapter != null)
      createOnlineLayout();
    // initialize the ui for the first time
    else{
      final ProgressDialog dialog = ProgressDialog.show(this, "", getString(R.string.initializing), true, false);

      // this is necessary to not block the user interface while initializing
      new Thread(new Runnable(){
        @Override
        public void run(){
          dialog.show();

          Context appContext = MainActivity.this.getApplicationContext();
          String fatal = null;
          ToolsInstaller installer = new ToolsInstaller(appContext);

          if(!Shell.isRootGranted())
            fatal = getString(R.string.only_4_root);

          else if(!Shell.isBinaryAvailable("killall"))
            fatal = getString(R.string.busybox_required);

          else if(!System.isARM())
            fatal = getString(R.string.arm_error) +
              getString(R.string.arm_error2);

          else if(installer.needed() && !installer.install())
            fatal = getString(R.string.install_error);

            // check for LD_LIBRARY_PATH issue ( https://github.com/evilsocket/dsploit/issues/35 )
          else if(!Shell.isLibraryPathOverridable(appContext))
            fatal = getString(R.string.fatal_error_html) +
              getString(R.string.fatal_error_html2);

          dialog.dismiss();

          if(fatal != null){
            BugSenseHandler.sendException(new Exception(fatal));

            final String ffatal = fatal;
            MainActivity.this.runOnUiThread(new Runnable(){
              @Override
              public void run(){
                new FatalDialog(getString(R.string.error), ffatal, ffatal.contains(">"), MainActivity.this).show();
              }
            });
          }

          MainActivity.this.runOnUiThread(new Runnable(){
            @Override
            public void run(){
              if(!System.getAppVersionName().equals(System.getSettings().getString("DSPLOIT_INSTALLED_VERSION", "0"))){
                new ChangelogDialog(MainActivity.this).show();
                Editor editor = System.getSettings().edit();
                editor.putString("DSPLOIT_INSTALLED_VERSION", System.getAppVersionName());
                editor.commit();
              }
              try{
                createOnlineLayout();
              }
              catch(Exception e){
                new FatalDialog(getString(R.string.error), e.getMessage(), MainActivity.this).show();
              }
            }
          });
        }
      }).start();
    }
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu){
    MenuInflater inflater = getSupportMenuInflater();
    inflater.inflate(R.menu.main, menu);

    if(!isWifiAvailable){
      menu.findItem(R.id.add).setVisible(false);
      menu.findItem(R.id.scan).setVisible(false);
      menu.findItem(R.id.new_session).setEnabled(false);
      menu.findItem(R.id.save_session).setEnabled(false);
      menu.findItem(R.id.restore_session).setEnabled(false);
      menu.findItem(R.id.settings).setEnabled(false);
      menu.findItem(R.id.ss_monitor).setEnabled(false);
      menu.findItem(R.id.ss_monitor).setEnabled(false);
      menu.findItem(R.id.ss_msfrpcd).setEnabled(false);
    }

    mMenu = menu;

    return super.onCreateOptionsMenu(menu);
  }

  @Override
  public boolean onPrepareOptionsMenu(Menu menu){
    MenuItem item = menu.findItem(R.id.ss_monitor);

    if(mNetworkDiscovery != null && mNetworkDiscovery.isRunning())
      item.setTitle(getString(R.string.stop_monitor));

    else
      item.setTitle(getString(R.string.start_monitor));

    item = menu.findItem(R.id.ss_msfrpcd);

    if(System.getMsfRpc()!=null)
      item.setTitle( getString(R.string.stop_msfrpcd) );
    else
      item.setTitle( getString(R.string.start_msfrpcd) );

    mMenu = menu;

    return super.onPrepareOptionsMenu(menu);
  }

  public void startUpdateChecker(){
    if(System.getSettings().getBoolean("PREF_CHECK_UPDATES", true)){
      UpdateChecker mUpdateChecker = new UpdateChecker(this);
      mUpdateChecker.start();
    }
  }

  public void startNetworkDiscovery(boolean silent){
    stopNetworkDiscovery(silent);

    mNetworkDiscovery = new NetworkDiscovery(this);
    mNetworkDiscovery.start();

    if(!silent)
      Toast.makeText(this, getString(R.string.net_discovery_started), Toast.LENGTH_SHORT).show();
  }

  public void stopNetworkDiscovery(boolean silent, boolean joinThreads){
    if(mNetworkDiscovery != null){
      if(mNetworkDiscovery.isRunning()){
        mNetworkDiscovery.exit();

        if( joinThreads ) {
          try{
            mNetworkDiscovery.join();
          } catch(Exception e){
          }
        }

        if(!silent)
          Toast.makeText(this, getString(R.string.net_discovery_stopped), Toast.LENGTH_SHORT).show();
      }

      mNetworkDiscovery = null;
    }
  }

  public void stopNetworkDiscovery(boolean silent) {
    stopNetworkDiscovery(silent,true);
  }

  public void StartMsfRpcd()
  {
    try
    {
      if(mRPCServer !=null)
      {
        mRPCServer.exit();
        mRPCServer.join();
        mRPCServer = null;
      }
    }
    catch ( InterruptedException ie)
    {
      // woop
    }
    mRPCServer = new RPCServer(this, 55553);
    mRPCServer.start();
  }

  public void StopMsfRpcd()
  {
    try
    {
      if(mRPCServer !=null)
      {
        mRPCServer.exit();
        mRPCServer.join();
        mRPCServer = null;
      }
      System.setMsfRpc(null);
      Shell.exec("killall msfrpcd");
    }
    catch ( Exception e)
    {
      // woop
    }
  }


  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    int itemId = item.getItemId();

    if(itemId == R.id.add){
      new InputDialog(getString(R.string.add_custom_target), getString(R.string.enter_url), MainActivity.this, new InputDialogListener(){
        @Override
        public void onInputEntered(String input){
          final Target target = Target.getFromString(input);
          if(target != null){
            // refresh the target listview
            MainActivity.this.runOnUiThread(new Runnable(){
              @Override
              public void run(){
                if(System.addOrderedTarget(target) && mTargetAdapter != null){
                  mTargetAdapter.notifyDataSetChanged();
                }
              }
            });
          }
          else
            new ErrorDialog(getString(R.string.error), getString(R.string.invalid_target), MainActivity.this).show();
        }
      }
      ).show();

      return true;
    }
    else if(itemId == R.id.scan){
      if(mMenu != null)
        mMenu.findItem(R.id.scan).setActionView(new ProgressBar(this));

      new Thread(new Runnable(){
        @Override
        public void run(){
          startNetworkDiscovery(true);

          MainActivity.this.runOnUiThread(new Runnable(){
            @Override
            public void run(){
              if(mMenu != null)
                mMenu.findItem(R.id.scan).setActionView(null);
            }
          });
        }
      }).start();

      item.setTitle(getString(R.string.stop_monitor));

      return true;
    }
    else if(itemId == R.id.wifi_scan){
      stopNetworkDiscovery(true);

      if(mEndpointReceiver != null)
        mEndpointReceiver.unregister();

      if(mUpdateReceiver != null)
        mUpdateReceiver.unregister();

      startActivityForResult(new Intent(MainActivity.this, WifiScannerActivity.class), WIFI_CONNECTION_REQUEST);

      return true;
    }
    else if(itemId == R.id.new_session){
      new ConfirmDialog(getString(R.string.warning), getString(R.string.warning_new_session), this, new ConfirmDialogListener(){
        @Override
        public void onConfirm(){
          try{
            System.reset();
            mTargetAdapter.notifyDataSetChanged();

            Toast.makeText(MainActivity.this, getString(R.string.new_session_started), Toast.LENGTH_SHORT).show();
          }
          catch(Exception e){
            new FatalDialog(getString(R.string.error), e.toString(), MainActivity.this).show();
          }
        }

        @Override
        public void onCancel(){
        }

      }).show();

      return true;
    }
    else if(itemId == R.id.save_session){
      new InputDialog(getString(R.string.save_session), getString(R.string.enter_session_name), System.getSessionName(), true, false, MainActivity.this, new InputDialogListener(){
        @Override
        public void onInputEntered(String input){
          String name = input.trim().replace("/", "").replace("..", "");

          if(!name.isEmpty()){
            try{
              String filename = System.saveSession(name);

              Toast.makeText(MainActivity.this, getString(R.string.session_saved_to) + filename + " .", Toast.LENGTH_SHORT).show();
            }
            catch(IOException e){
              new ErrorDialog(getString(R.string.error), e.toString(), MainActivity.this).show();
            }
          }
          else
            new ErrorDialog(getString(R.string.error), getString(R.string.invalid_session), MainActivity.this).show();
        }
      }
      ).show();

      return true;
    }
    else if(itemId == R.id.restore_session){
      final ArrayList<String> sessions = System.getAvailableSessionFiles();

      if(sessions != null && sessions.size() > 0){
        new SpinnerDialog(getString(R.string.select_session), getString(R.string.select_session_file), sessions.toArray(new String[sessions.size()]), MainActivity.this, new SpinnerDialogListener(){
          @Override
          public void onItemSelected(int index){
            String session = sessions.get(index);

            try{
              System.loadSession(session);
              mTargetAdapter.notifyDataSetChanged();
            }
            catch(Exception e){
              e.printStackTrace();
              new ErrorDialog(getString(R.string.error), e.getMessage(), MainActivity.this).show();
            }
          }
        }).show();
      }
      else
        new ErrorDialog(getString(R.string.error), getString(R.string.no_session_found), MainActivity.this).show();

      return true;
    }
    else if(itemId == R.id.settings){
      startActivity(new Intent(MainActivity.this, SettingsActivity.class));

      return true;
    }
    else if(itemId == R.id.ss_monitor){
      if(mNetworkDiscovery != null && mNetworkDiscovery.isRunning()){
        stopNetworkDiscovery(false);

        item.setTitle(getString(R.string.start_monitor));
      }
      else {
        startNetworkDiscovery(false);

        item.setTitle(getString(R.string.stop_monitor));
      }

      return true;
    }
    else if( itemId == R.id.ss_msfrpcd )
    {
      if(System.getMsfRpc()!=null)
      {
        StopMsfRpcd();
        item.setTitle("Start msfrpcd");
      }
      else
      {
        StartMsfRpcd();
        item.setTitle("Stop msfrpcd");
      }
      return true;
    }
    else if(itemId == R.id.submit_issue){
      String uri = getString(R.string.github_issues);
      Intent browser = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));

      startActivity(browser);

      return true;
    }
    else if(itemId == R.id.about){
      new AboutDialog(this).show();

      return true;
    }
    else
      return super.onOptionsItemSelected(item);
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id){
    super.onListItemClick(l, v, position, id);

    new Thread(new Runnable(){
      @Override
      public void run(){
        /*
         * Do not wait network discovery threads to exit since this would cause
         * a long waiting when it's scanning big networks.
         */
        stopNetworkDiscovery(true, false);

        startActivityForResult(new Intent(MainActivity.this, ActionActivity.class), WIFI_CONNECTION_REQUEST);

        overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
      }
    }).start();

    System.setCurrentTarget(position);
    Toast.makeText(MainActivity.this, getString(R.string.selected_) + System.getCurrentTarget(), Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onBackPressed(){
    if(mLastBackPressTime < java.lang.System.currentTimeMillis() - 4000){
      mToast = Toast.makeText(this, getString(R.string.press_back), Toast.LENGTH_SHORT);
      mToast.show();
      mLastBackPressTime = java.lang.System.currentTimeMillis();
    }
    else {
      if( mToast != null )
        mToast.cancel();

      new ConfirmDialog(getString(R.string.exit), getString(R.string.close_confirm), this, new ConfirmDialogListener(){
        @Override
        public void onConfirm(){
          MainActivity.this.finish();
        }

        @Override
        public void onCancel(){ }
      }).show();

      mLastBackPressTime = 0;
    }
  }

  @Override
  public void onDestroy(){
    stopNetworkDiscovery(true);
    StopMsfRpcd();

    if(mEndpointReceiver != null)
      mEndpointReceiver.unregister();

    if(mUpdateReceiver != null)
      mUpdateReceiver.unregister();

    if(mMsfRpcdReceiver != null)
      mMsfRpcdReceiver.unregister();

    // make sure no zombie process is running before destroying the activity
    System.clean(true);

    BugSenseHandler.closeSession(getApplicationContext());

    super.onDestroy();

    // remove the application from the cache
    java.lang.System.exit(0);
  }

  public class TargetAdapter extends ArrayAdapter<Target>{
    public TargetAdapter(){
      super(MainActivity.this, R.layout.target_list_item);
    }

    @Override
    public int getCount(){
      return System.getTargets().size();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent){
      View row = convertView;
      TargetHolder holder;

      if(row == null){
        LayoutInflater inflater = (LayoutInflater) MainActivity.this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        row = inflater.inflate(R.layout.target_list_item, parent, false);

        holder = new TargetHolder();
        holder.itemImage = (ImageView) (row != null ? row.findViewById(R.id.itemIcon) : null);
        holder.itemTitle = (TextView) (row != null ? row.findViewById(R.id.itemTitle) : null);
        holder.itemDescription = (TextView) (row != null ? row.findViewById(R.id.itemDescription) : null);

        if(row != null)
          row.setTag(holder);
      }
      else
        holder = (TargetHolder) row.getTag();

      Target target = System.getTarget(position);

      if(target.hasAlias())
        holder.itemTitle.setText(Html.fromHtml("<b>" + target.getAlias() + "</b> <small>( " + target.getDisplayAddress() + " )</small>"));

      else
        holder.itemTitle.setText(target.toString());

      holder.itemTitle.setTypeface(null, Typeface.NORMAL);
      holder.itemImage.setImageResource(target.getDrawableResourceId());
      holder.itemDescription.setText(target.getDescription());

      return row;
    }

    class TargetHolder{
      ImageView itemImage;
      TextView itemTitle;
      TextView itemDescription;
    }
  }

  private class EndpointReceiver extends ManagedReceiver{
    private IntentFilter mFilter = null;

    public EndpointReceiver(){
      mFilter = new IntentFilter();

      mFilter.addAction(NEW_ENDPOINT);
      mFilter.addAction(ENDPOINT_UPDATE);
    }

    public IntentFilter getFilter(){
      return mFilter;
    }

    @SuppressWarnings("ConstantConditions")
    @Override
    public void onReceive(Context context, Intent intent){
      if(intent.getAction() != null)
        if(intent.getAction().equals(NEW_ENDPOINT)){
          String address = (String) intent.getExtras().get(ENDPOINT_ADDRESS),
                 hardware = (String) intent.getExtras().get(ENDPOINT_HARDWARE),
                 name = (String) intent.getExtras().get(ENDPOINT_NAME);
          final Target target = Target.getFromString(address);

          if(target != null && target.getEndpoint() != null){
            if(name != null && !name.isEmpty())
              target.setAlias(name);

            target.getEndpoint().setHardware(Endpoint.parseMacAddress(hardware));

            // refresh the target listview
            MainActivity.this.runOnUiThread(new Runnable(){
              @Override
              public void run(){
                if(System.addOrderedTarget(target)){
                  mTargetAdapter.notifyDataSetChanged();
                }
              }
            });
          }
        }
        else if(intent.getAction().equals(ENDPOINT_UPDATE)){
          // refresh the target listview
          MainActivity.this.runOnUiThread(new Runnable(){
            @Override
            public void run(){
              mTargetAdapter.notifyDataSetChanged();
            }
          });
        }
    }
  }

  private class MsfrpcdReceiver extends ManagedReceiver
  {
    private IntentFilter mFilter = null;

    public MsfrpcdReceiver() {
      mFilter = new IntentFilter();

      mFilter.addAction( RPCServer.STARTING );
      mFilter.addAction( RPCServer.STARTED );
      mFilter.addAction( RPCServer.RUNNING );
      mFilter.addAction( RPCServer.STOPPED );
      mFilter.addAction( RPCServer.FAILED );
    }

    public IntentFilter getFilter( ) {
      return mFilter;
    }

    @Override
    public void onReceive( Context context, Intent intent )
    {
      MenuItem item = mMenu.findItem( R.id.ss_msfrpcd );

      if( intent.getAction().equals( RPCServer.FAILED ) )
      {
        final String message = ( String )intent.getExtras().get( RPCServer.ERROR );
        MainActivity.this.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            new ErrorDialog("RPCServer error", message, MainActivity.this);
          }
        });
        item.setEnabled( false );
        item.setTitle("msfrpcd KO");
        MainActivity.this.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            Toast.makeText( MainActivity.this, "unable to connect to msfrpcd, check settings", Toast.LENGTH_SHORT ).show();
          }
        });
      }
      else
      {
        String status = "";
        String action = "Stop";
        mMenu.findItem( R.id.ss_msfrpcd ).setEnabled( true );

        if(intent.getAction().equals( RPCServer.STARTING))
          status = "Starting RPCServer";
        else if(intent.getAction().equals( RPCServer.STARTED))
          status = "RPCServer started";
        else if(intent.getAction().equals(RPCServer.RUNNING))
          status = "RPCServer is already running";
        else if(intent.getAction().equals( RPCServer.STOPPED))
        {
          status = "RPCServer stopped";
          action = "Start";
        }
        item.setTitle(action + " msfrpcd");
        item.setEnabled(true); // should never change...
        final String final_status = status;
        MainActivity.this.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            Toast.makeText( MainActivity.this, final_status, Toast.LENGTH_SHORT ).show();
          }
        });
      }
    }
  }

  private class UpdateReceiver extends ManagedReceiver{
    private IntentFilter mFilter = null;

    public UpdateReceiver(){
      mFilter = new IntentFilter();

      mFilter.addAction(UPDATE_CHECKING);
      mFilter.addAction(UPDATE_AVAILABLE);
      mFilter.addAction(UPDATE_NOT_AVAILABLE);
    }

    public IntentFilter getFilter(){
      return mFilter;
    }

    @SuppressWarnings("ConstantConditions")
    @Override
    public void onReceive(Context context, Intent intent){
      if(mUpdateStatus != null && intent.getAction().equals(UPDATE_CHECKING) && mUpdateStatus != null){
        mUpdateStatus.setText(NO_WIFI_UPDATE_MESSAGE.replace("#STATUS#", getString(R.string.checking)));
      }
      else if(mUpdateStatus != null && intent.getAction().equals(UPDATE_NOT_AVAILABLE) && mUpdateStatus != null){
        mUpdateStatus.setText(NO_WIFI_UPDATE_MESSAGE.replace("#STATUS#", getString(R.string.no_updates_available)));
      }
      else if(intent.getAction().equals(UPDATE_AVAILABLE)){
        final String remoteVersion = (String) intent.getExtras().get(AVAILABLE_VERSION);

        if(mUpdateStatus != null)
          mUpdateStatus.setText(NO_WIFI_UPDATE_MESSAGE.replace("#STATUS#", getString(R.string.new_version) + remoteVersion + getString(R.string.new_version2)));

        MainActivity.this.runOnUiThread(new Runnable(){
          @Override
          public void run(){
            new ConfirmDialog(
              getString(R.string.update_available),
              getString(R.string.new_update_desc) + remoteVersion + getString(R.string.new_update_desc2),
              MainActivity.this,
              new ConfirmDialogListener(){
                @Override
                public void onConfirm(){
                  final ProgressDialog dialog = new ProgressDialog(MainActivity.this);
                  dialog.setTitle(getString(R.string.downloading_update));
                  dialog.setMessage(getString(R.string.connecting));
                  dialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                  dialog.setMax(100);
                  dialog.setCancelable(false);
                  dialog.setButton(DialogInterface.BUTTON_NEGATIVE, getString(R.string.cancel), new DialogInterface.OnClickListener(){
                    @Override
                    public void onClick(DialogInterface dialog, int which){
                      dialog.dismiss();
                    }
                  });
                  dialog.show();

                  new Thread(new Runnable(){
                    @Override
                    public void run(){
                      if(!System.getUpdateManager().downloadUpdate(MainActivity.this, dialog)){
                        MainActivity.this.runOnUiThread(new Runnable(){
                          @Override
                          public void run(){
                            new ErrorDialog(getString(R.string.error), getString(R.string.error_occured), MainActivity.this).show();
                          }
                        });
                      }

                      dialog.dismiss();
                    }
                  }).start();
                }

                @Override
                public void onCancel(){
                }
              }
            ).show();
          }
        });
      }
    }
  }
}