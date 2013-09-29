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

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.widget.Toast;

import com.actionbarsherlock.app.SherlockPreferenceActivity;
import com.actionbarsherlock.view.MenuItem;

import java.io.File;

import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.DirectoryPicker;

@SuppressWarnings("deprecation")
public class SettingsActivity extends SherlockPreferenceActivity implements OnSharedPreferenceChangeListener
{
  public static final int SETTINGS_DONE = 101285;
  private Preference mSavePath = null;
  private EditTextPreference mSnifferSampleTime = null;
  private EditTextPreference mProxyPort = null;
  private EditTextPreference mServerPort = null;
  private EditTextPreference mRedirectorPort = null;
  private EditTextPreference mHttpBufferSize = null;
  private EditTextPreference mPasswordFilename = null;

  @SuppressWarnings("ConstantConditions")
  @Override
  protected void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    getSupportActionBar().setDisplayHomeAsUpEnabled(true);
    addPreferencesFromResource(R.layout.preferences);

    mSavePath = getPreferenceScreen().findPreference("PREF_SAVE_PATH");
    mSnifferSampleTime = (EditTextPreference) getPreferenceScreen().findPreference("PREF_SNIFFER_SAMPLE_TIME");
    mProxyPort = (EditTextPreference) getPreferenceScreen().findPreference("PREF_HTTP_PROXY_PORT");
    mServerPort = (EditTextPreference) getPreferenceScreen().findPreference("PREF_HTTP_SERVER_PORT");
    mRedirectorPort = (EditTextPreference) getPreferenceScreen().findPreference("PREF_HTTPS_REDIRECTOR_PORT");
    mHttpBufferSize = (EditTextPreference) getPreferenceScreen().findPreference("PREF_HTTP_MAX_BUFFER_SIZE");
    mPasswordFilename = (EditTextPreference) getPreferenceScreen().findPreference("PREF_PASSWORD_FILENAME");

    mSavePath.setOnPreferenceClickListener(new OnPreferenceClickListener(){
      @Override
      public boolean onPreferenceClick(Preference preference){
        startActivityForResult(new Intent(SettingsActivity.this, DirectoryPicker.class), DirectoryPicker.PICK_DIRECTORY);
        return true;
      }
    });
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent intent){
    if(requestCode == DirectoryPicker.PICK_DIRECTORY && resultCode != RESULT_CANCELED){
      Bundle extras = intent.getExtras();
      String path = (String) (extras != null ? extras.get(DirectoryPicker.CHOSEN_DIRECTORY) : null);
      File folder = new File(path);

      if(!folder.exists())
        Toast.makeText(SettingsActivity.this, getString(R.string.pref_folder) + " " + path + " " + getString(R.string.pref_err_exists), Toast.LENGTH_SHORT).show();

      else if(!folder.canWrite())
        Toast.makeText(SettingsActivity.this, getString(R.string.pref_folder) + " " + path + " " + getString(R.string.pref_err_writable), Toast.LENGTH_SHORT).show();

      else
        //noinspection ConstantConditions
        getPreferenceManager().getSharedPreferences().edit().putString("PREF_SAVE_PATH", path).commit();
    }
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
    else if(key.equals("PREF_HTTP_PROXY_PORT")){
      int proxyPort;

      try{
        proxyPort = Integer.parseInt(mProxyPort.getText());
        if(proxyPort < 1024 || proxyPort > 65536){
          message = getString(R.string.pref_err_proxy_port);
          proxyPort = 8080;
        }
        else if(!System.isPortAvailable(proxyPort)){
          message = getString(R.string.pref_err_busy_port);
          proxyPort = 8080;
        }
      }
      catch(Throwable t){
        message = getString(R.string.pref_err_invalid_number);
        proxyPort = 8080;
      }

      if(message != null)
        Toast.makeText(SettingsActivity.this, message, Toast.LENGTH_SHORT).show();

      mProxyPort.setText(Integer.toString(proxyPort));
      System.HTTP_PROXY_PORT = proxyPort;
    }
    else if(key.equals("PREF_HTTP_SERVER_PORT")){
      int serverPort;

      try{
        serverPort = Integer.parseInt(mServerPort.getText());
        if(serverPort < 1024 || serverPort > 65535){
          message = getString(R.string.pref_err_server_port);
          serverPort = 8081;
        }
        else if(!System.isPortAvailable(serverPort)){
          message = getString(R.string.pref_err_busy_port);
          serverPort = 8081;
        }
      }
      catch(Throwable t){
        message = getString(R.string.pref_err_invalid_number);
        serverPort = 8081;
      }

      if(message != null)
        Toast.makeText(SettingsActivity.this, message, Toast.LENGTH_SHORT).show();

      mServerPort.setText(Integer.toString(serverPort));
      System.HTTP_SERVER_PORT = serverPort;
    }
    else if(key.equals("PREF_HTTPS_REDIRECTOR_PORT")){
      int redirPort;

      try{
        redirPort = Integer.parseInt(mRedirectorPort.getText());
        if(redirPort < 1024 || redirPort > 65535){
          message = getString(R.string.pref_err_redirector_port);
          redirPort = 8082;
        }
        else if(!System.isPortAvailable(redirPort)){
          message = getString(R.string.pref_err_busy_port);
          redirPort = 8082;
        }
      }
      catch(Throwable t){
        message = getString(R.string.pref_err_invalid_number);
        redirPort = 8082;
      }

      if(message != null)
        Toast.makeText(SettingsActivity.this, message, Toast.LENGTH_SHORT).show();

      mRedirectorPort.setText(Integer.toString(redirPort));
      System.HTTPS_REDIR_PORT = redirPort;
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
    }
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
}
