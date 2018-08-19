
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
package org.csploit.android;

import android.Manifest;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

  MainFragment f;
  final static int MY_PERMISSIONS_WANTED = 1;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
      NotificationChannel mChannel = new NotificationChannel(getString(R.string.csploitChannelId),
              getString(R.string.cSploitChannelDescription), NotificationManager.IMPORTANCE_DEFAULT);
      NotificationManager mNotificationManager =
              (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
      if (mNotificationManager != null) {
          mNotificationManager.createNotificationChannel(mChannel);
      }
    }
    SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
    if (themePrefs.getBoolean("isDark", false))
      setTheme(R.style.DarkTheme);
    else
      setTheme(R.style.AppTheme);
    setContentView(R.layout.main);
    if (findViewById(R.id.mainframe) != null) {
      if (savedInstanceState != null) {
        return;
      }
      f = new MainFragment();
      getSupportFragmentManager().beginTransaction()
              .add(R.id.mainframe, f).commit();
    }
    verifyPerms();
  }

  public void verifyPerms() {
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED || ContextCompat.checkSelfPermission(this,
                Manifest.permission.READ_PHONE_STATE)
                != PackageManager.PERMISSION_GRANTED || ContextCompat.checkSelfPermission(this,
              Manifest.permission.WAKE_LOCK)
              != PackageManager.PERMISSION_GRANTED)
      {
          ActivityCompat.requestPermissions(this,
                  new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_PHONE_STATE,
                  Manifest.permission.WAKE_LOCK},
                  MY_PERMISSIONS_WANTED);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        switch (requestCode) {
            case MY_PERMISSIONS_WANTED: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(this, R.string.permissions_succeed, Toast.LENGTH_LONG).show();
                } else {
                    Toast.makeText(this, R.string.permissions_fail, Toast.LENGTH_LONG).show();
                    finish();
                }
            }
        }
    }
}