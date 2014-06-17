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

import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.UpdateService;
import it.evilsocket.dsploit.gui.DirectoryPicker;
import it.evilsocket.dsploit.gui.dialogs.ChoiceDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog;

import java.io.File;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.widget.Toast;

import com.actionbarsherlock.app.SherlockPreferenceActivity;
import com.actionbarsherlock.view.MenuItem;

@SuppressWarnings("deprecation")
public class SettingsActivity extends SherlockPreferenceActivity implements OnSharedPreferenceChangeListener
{
  public static final int SETTINGS_DONE = 101285;
  public static final String SETTINGS_WIPE_START = "SettingsActivity.WIPE_START";
  public static final String SETTINGS_WIPE_DIR = "SettingsActivity.data.WIPE_DIR";
  public static final String SETTINGS_WIPE_DONE = "SettingsActivity.WIPE_DONE";
  public static final String SETTINGS_MSF_CHANGED = "SettingsActivity.MSF_CHANGED";
  public static final String SETTINGS_MSF_BRANCHES_AVAILABLE = "SettingsActivity.MSF_MSF_BRANCHES_AVAILABLE";

  private Preference mSavePath = null;
  private Preference mWipeMSF  = null;
  private Preference mRubyDir = null;
  private Preference mMsfDir = null;
  private EditTextPreference mSnifferSampleTime = null;
  private EditTextPreference mProxyPort = null;
  private EditTextPreference mServerPort = null;
  private EditTextPreference mRedirectorPort = null;
  private EditTextPreference mMsfPort = null;
  private EditTextPreference mHttpBufferSize = null;
  private EditTextPreference mPasswordFilename = null;
  private CheckBoxPreference mThemeChooser = null;
  private CheckBoxPreference mMsfEnabled  = null;
  private ListPreference mMsfBranch = null;
  private static int mMsfSize = 0;
  private BroadcastReceiver mReceiver = null;
  private Thread mBranchesWaiter = null;

  @SuppressWarnings("ConstantConditions")
  @Override
  protected void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    getSupportActionBar().setDisplayHomeAsUpEnabled(true);
    addPreferencesFromResource(R.layout.preferences);

    mSavePath = getPreferenceScreen().findPreference("PREF_SAVE_PATH");
    mWipeMSF  = getPreferenceScreen().findPreference("PREF_MSF_WIPE");
    mRubyDir = getPreferenceScreen().findPreference("RUBY_DIR");
    mMsfDir = getPreferenceScreen().findPreference("MSF_DIR");
    mMsfPort = (EditTextPreference) getPreferenceScreen().findPreference("MSF_RPC_PORT");
    mSnifferSampleTime = (EditTextPreference) getPreferenceScreen().findPreference("PREF_SNIFFER_SAMPLE_TIME");
    mProxyPort = (EditTextPreference) getPreferenceScreen().findPreference("PREF_HTTP_PROXY_PORT");
    mServerPort = (EditTextPreference) getPreferenceScreen().findPreference("PREF_HTTP_SERVER_PORT");
    mRedirectorPort = (EditTextPreference) getPreferenceScreen().findPreference("PREF_HTTPS_REDIRECTOR_PORT");
    mHttpBufferSize = (EditTextPreference) getPreferenceScreen().findPreference("PREF_HTTP_MAX_BUFFER_SIZE");
    mPasswordFilename = (EditTextPreference) getPreferenceScreen().findPreference("PREF_PASSWORD_FILENAME");
    mThemeChooser = (CheckBoxPreference) getPreferenceScreen().findPreference("PREF_DARK_THEME");
    mMsfBranch = (ListPreference) getPreferenceScreen().findPreference("MSF_BRANCH");
    mMsfEnabled = (CheckBoxPreference) getPreferenceScreen().findPreference("MSF_ENABLED");

    mThemeChooser.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {

      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue) {
        SharedPreferences themePrefs = getBaseContext().getSharedPreferences("THEME", 0);
        themePrefs.edit().putBoolean("isDark", (Boolean) newValue).commit();
        Toast.makeText(getBaseContext(), getString(R.string.please_restart), Toast.LENGTH_LONG).show();
        return true;
      }
    });

    mSavePath.setOnPreferenceClickListener(new OnPreferenceClickListener(){
      @Override
      public boolean onPreferenceClick(Preference preference) {
        startDirectoryPicker(preference);
        return true;
      }
    });

    if(mMsfEnabled.isEnabled())
      onMsfEnabled();
  }

  private void wipe_prompt() {
    String message = getString(R.string.pref_msfwipe_message);
    if(mMsfSize>0) {
      message += "\n" + String.format(getString(R.string.pref_msfwipe_size), mMsfSize);
    }
    new ConfirmDialog(getString(R.string.warning),message,SettingsActivity.this, new ConfirmDialog.ConfirmDialogListener() {
      @Override
      public void onConfirm() {
        sendBroadcast(new Intent(SETTINGS_WIPE_START));
      }

      @Override
      public void onCancel() {

      }
    }).show();
  }

  private void wipe_prompt_older(final File oldDir) {
    new ConfirmDialog(getString(R.string.warning),getString(R.string.delete_previous_location),SettingsActivity.this, new ConfirmDialog.ConfirmDialogListener() {
      @Override
      public void onConfirm() {
        Intent i = new Intent(SETTINGS_WIPE_START);
        i.putExtra(SETTINGS_WIPE_DIR, oldDir.getAbsolutePath());
        sendBroadcast(i);
      }

      @Override
      public void onCancel() {

      }
    }).show();
  }

  private void measureMsfSize() {
    Shell.async(String.format("du -xsm '%s' '%s'",System.getRubyPath(), System.getMsfPath()),
            new Shell.OutputReceiver() {
              private int size = 0;

              @Override
              public void onStart(String command) {

              }

              @SuppressWarnings("StatementWithEmptyBody")
              @Override
              public void onNewLine(String line) {
                if(line.isEmpty())
                  return;
                try {
                  int start,end;
                  for(start=0;start<line.length()&&java.lang.Character.isSpaceChar(line.charAt(start));start++);
                  for(end=start+1;end<line.length()&&java.lang.Character.isDigit(line.charAt(end));end++);
                  size += Integer.parseInt(line.substring(start,end));
                } catch ( Exception e ) {
                  System.errorLogging(e);
                }
              }

              @Override
              public void onEnd(int exitCode) {
                if(exitCode==0)
                  mMsfSize = size;
              }
            }
            ).start();
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent intent){
    if(requestCode == DirectoryPicker.PICK_DIRECTORY && resultCode != RESULT_CANCELED){
      Bundle extras = intent.getExtras();
      String path;
      String key;
      File folder;
      String oldPath = null;

      if(extras==null) {
        Logger.debug("null extra: " + intent);
        return;
      }

      path = (String) extras.get(DirectoryPicker.CHOSEN_DIRECTORY);
      key = (String) extras.get(DirectoryPicker.AFFECTED_PREF);

      if(path == null || key == null) {
        Logger.debug("null path or key: "+intent);
        return;
      }

      folder = new File(path);


      if(key.equals("RUBY_DIR"))
        oldPath = System.getRubyPath();
      else if (key.equals("MSF_DIR"))
        oldPath = System.getMsfPath();

      if(!folder.exists())
        Toast.makeText(SettingsActivity.this, getString(R.string.pref_folder) + " " + path + " " + getString(R.string.pref_err_exists), Toast.LENGTH_SHORT).show();

      else if(!folder.canWrite())
        Toast.makeText(SettingsActivity.this, getString(R.string.pref_folder) + " " + path + " " + getString(R.string.pref_err_writable), Toast.LENGTH_SHORT).show();

      else if(!Shell.canExecuteInDir(folder.getAbsolutePath()) && !Shell.canRootExecuteInDir(Shell.getRealPath(folder.getAbsolutePath())))
        Toast.makeText(SettingsActivity.this, getString(R.string.pref_folder) + " " + path + " " + getString(R.string.pref_err_executable), Toast.LENGTH_LONG).show();

      else {
        //noinspection ConstantConditions
        getPreferenceManager().getSharedPreferences().edit().putString(key, path).commit();
        if(oldPath!=null && !oldPath.equals(path)) {
          File current = new File(oldPath);

          if (current.exists() && current.isDirectory() && current.listFiles().length > 2) {
            wipe_prompt_older(current);
          }
        }
      }
    }
  }

  private void startDirectoryPicker(Preference preference) {
    Intent i = new Intent(SettingsActivity.this, DirectoryPicker.class);
    i.putExtra(DirectoryPicker.AFFECTED_PREF, preference.getKey());
    startActivityForResult(i, DirectoryPicker.PICK_DIRECTORY);
  }

  @SuppressWarnings("ConstantConditions")
  @Override
  protected void onResume(){
    super.onResume();
    getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
  }

  @SuppressWarnings("ConstantConditions")
  @Override
  protected void onPause(){
    super.onPause();
    getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
  }

  public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key){
    String message = null;

    if(key.equals("PREF_SNIFFER_SAMPLE_TIME")){
      double sampleTime;

      try{
        sampleTime = Double.parseDouble(mSnifferSampleTime.getText());
        if(sampleTime < 0.4 || sampleTime > 1.0){
          message = getString(R.string.pref_err_sample_time);
          sampleTime = 1.0;
        }
      }
      catch(Throwable t){
        message = getString(R.string.pref_err_invalid_number);
        sampleTime = 1.0;
      }

      if(message != null)
        Toast.makeText(SettingsActivity.this, message, Toast.LENGTH_SHORT).show();

      mSnifferSampleTime.setText(Double.toString(sampleTime));
    }
    else if(key.endsWith("_PORT")) {
      int port;

      try{
        port = Integer.parseInt(mProxyPort.getText());
        if(port < 1024 || port > 65536){
          message = getString(R.string.pref_err_port_range);
          port = 0;
        }
        else if(!System.isPortAvailable(port)){
          message = getString(R.string.pref_err_busy_port);
          port = 0;
        }
      }
      catch(Throwable t){
        message = getString(R.string.pref_err_invalid_number);
        port = 0;
      }

      if(message!=null)
        Toast.makeText(SettingsActivity.this, message, Toast.LENGTH_SHORT).show();

      if(key.equals("PREF_HTTP_PROXY_PORT")) {
        if(port==0)
          port=8080;
        mProxyPort.setText(Integer.toString(port));
        System.HTTP_PROXY_PORT = port;
      } else if(key.equals("PREF_HTTP_SERVER_PORT")) {
        if(port==0)
          port = 8081;
        mServerPort.setText(Integer.toString(port));
        System.HTTP_SERVER_PORT = port;
      } else if(key.equals("PREF_HTTPS_REDIRECTOR_PORT")) {
        if(port==0)
          port = 8082;
        mRedirectorPort.setText(Integer.toString(port));
        System.HTTPS_REDIR_PORT = port;
      } else if(key.equals("MSF_RPC_PORT")) {
        if(port==0)
          port = 55553;
        mMsfPort.setText(Integer.toString(port));
        System.MSF_RPC_PORT = port;
      }
    }
    else if(key.equals("PREF_HTTP_MAX_BUFFER_SIZE")){
      int maxBufferSize;

      try{
        maxBufferSize = Integer.parseInt(mHttpBufferSize.getText());
        if(maxBufferSize < 1024 || maxBufferSize > 104857600){
          message = getString(R.string.pref_err_buffer_size);
          maxBufferSize = 10485760;
        }
      }
      catch(Throwable t){
        message = getString(R.string.pref_err_invalid_number);
        maxBufferSize = 10485760;
      }

      if(message != null)
        Toast.makeText(SettingsActivity.this, message, Toast.LENGTH_SHORT).show();

      mHttpBufferSize.setText(Integer.toString(maxBufferSize));
    }
    else if(key.equals("PREF_PASSWORD_FILENAME")){
      String passFileName;

      try{
        passFileName = mPasswordFilename.getText();
        if(!passFileName.matches("[^/?*:;{}\\]+]")){
          message = getString(R.string.invalid_filename);
          passFileName = "dsploit-password-sniff.log";
        }
      }
      catch(Throwable t){
        message = getString(R.string.invalid_filename);
        passFileName = "dsploit-password-sniff.log";
      }

      if(message != null)
        Toast.makeText(SettingsActivity.this, message, Toast.LENGTH_SHORT).show();

      mPasswordFilename.setText(passFileName);
    } else if (key.equals("MSF_ENABLED")) {
      if (mMsfEnabled.isChecked())
        onMsfEnabled();
      sendBroadcast(new Intent(SETTINGS_MSF_CHANGED));
    } else if (key.equals("MSF_BRANCH")) {
      // don't send SETTINGS_MSF_CHANGED, this change will affect he app on next restart
    } else if (key.contains("MSF")) {
      sendBroadcast(new Intent(SETTINGS_MSF_CHANGED));
    } else if (key.contains("RUBY")) {
      sendBroadcast(new Intent(SETTINGS_MSF_CHANGED));
      Shell.rubySettingsChanged();
    }
  }

  private void onMsfEnabled() {
    // use mReceiver as "already did that"
    if(mReceiver!=null)
      return;

    // start measureMsfSize ASAP
    onMsfPathChanged();

    OnPreferenceClickListener directoryPickerWithDefaultPath = new OnPreferenceClickListener() {
      @Override
      public boolean onPreferenceClick(Preference preference) {

        final String currentValue;
        final String defaultValue;
        final String key = preference.getKey();

        if(key.equals("RUBY_DIR")) {
          currentValue = System.getRubyPath();
          defaultValue = System.getDefaultRubyPath();
        } else if(key.equals("MSF_DIR")) {
          currentValue = System.getMsfPath();
          defaultValue = System.getDefaultMsfPath();
        } else
          return true;

        if (!currentValue.equals(defaultValue)) {
          final Preference fPref = preference;
          (new ChoiceDialog(
                  SettingsActivity.this,
                  getString(R.string.choose_an_option),
                  new String[]{getString(R.string.restore_default_path), getString(R.string.choose_a_custom_path)},
                  new ChoiceDialog.ChoiceDialogListener() {
                    @Override
                    public void onChoice(int choice) {
                      if (choice == 0) {
                        // create default directory if it does not exists
                        File f = new File(defaultValue);
                        if(!f.exists())
                          f.mkdirs();
                        // simulate directory picker
                        Intent i = new Intent();
                        i.putExtra(DirectoryPicker.AFFECTED_PREF, key);
                        i.putExtra(DirectoryPicker.CHOSEN_DIRECTORY, defaultValue);
                        onActivityResult(DirectoryPicker.PICK_DIRECTORY, RESULT_OK, i);
                      } else {
                        startDirectoryPicker(fPref);
                      }
                    }
                  }
          )).show();
        } else {
          startDirectoryPicker(preference);
        }
        return true;
      }
    };

    mRubyDir.setDefaultValue(System.getDefaultRubyPath());
    mRubyDir.setOnPreferenceClickListener(directoryPickerWithDefaultPath);

    mMsfDir.setDefaultValue(System.getDefaultMsfPath());
    mMsfDir.setOnPreferenceClickListener(directoryPickerWithDefaultPath);

    mWipeMSF.setOnPreferenceClickListener(new OnPreferenceClickListener() {
      @Override
      public boolean onPreferenceClick(Preference preference) {
        wipe_prompt();
        return true;
      }
    });

    getMsfBranches();

    mReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        if(intent.getAction().equals(SETTINGS_WIPE_DONE)) {
          onMsfPathChanged();
        } else if(intent.getAction().equals(SETTINGS_MSF_BRANCHES_AVAILABLE)) {
          onMsfBranchesAvailable();
        }
      }
    };
    registerReceiver(mReceiver, new IntentFilter(SETTINGS_WIPE_DONE));
  }

  private void getMsfBranches() {
    if(mBranchesWaiter!=null) { // run it once per settings activity
      if(mBranchesWaiter.getState() == Thread.State.TERMINATED)
        try {
          mBranchesWaiter.join();
        } catch (InterruptedException e) {
          Logger.error(e.getMessage());
        }
      return;
    }

    final ListPreference pref = mMsfBranch;
    mMsfBranch.setEnabled(false);
    mBranchesWaiter = new Thread(new Runnable() {
      @Override
      public void run() {
        UpdateService.getMsfBranches();
        sendBroadcast(new Intent(SETTINGS_MSF_BRANCHES_AVAILABLE));
      }
    });
    mBranchesWaiter.start();
  }

  private void onMsfPathChanged() {
    measureMsfSize();
    System.updateLocalRubyVersion();
    System.updateLocalMsfVersion();
    boolean haveMsf = false;
    File dir;
    File [] content;

    if((dir=new File(System.getRubyPath())).isDirectory() ||
            (dir=new File(System.getMsfPath())).isDirectory()) {
      content = dir.listFiles();
      haveMsf = content!=null && content.length > 2;
    }

    mWipeMSF.setEnabled(haveMsf);
  }

  private void onMsfBranchesAvailable() {
    String [] branches = UpdateService.getMsfBranches();
    boolean hasRelease = false;
    mMsfBranch.setEntryValues(branches);
    mMsfBranch.setEntries(branches);
    for(int i = 0;!hasRelease&&i<branches.length; i++) {
      hasRelease = branches[i].equals("release");
    }
    mMsfBranch.setDefaultValue((hasRelease ? "release" : "master"));
    mMsfBranch.setEnabled(true);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    switch(item.getItemId()){
      case android.R.id.home:
        onBackPressed();
        return true;

      default:
        return super.onOptionsItemSelected(item);
    }
  }

  @Override
  protected void onDestroy() {
    if(mReceiver!=null) {
      unregisterReceiver(mReceiver);
      mReceiver = null;
    }
    super.onDestroy();
  }
}
