/*
 * This file is part of the cSploit.
 *
 * Copyleft
 *
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android.plugins.mitm;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.tools.Ettercap;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

public class DNSSpoofing extends AppCompatActivity {
    private ToggleButton mSniffToggleButton = null;
	private ProgressBar mSniffProgress = null;
	private TextView mTextDnsList = null;
    private Button mCmdSave = null;
	private boolean mRunning = false;
	private FileWriter mFileWriter = null;
	private BufferedWriter mBufferedWriter = null;
	private SpoofSession mSpoofSession = null;

    	public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

		SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
		if (isDark)
			setTheme(R.style.DarkTheme);
		else
			setTheme(R.style.AppTheme);

		setTitle(System.getCurrentTarget() + " > MITM > DNS spoofing");
		setContentView(R.layout.plugin_mitm_dns_spoofing);
		getSupportActionBar().setDisplayHomeAsUpEnabled(true);

		mTextDnsList = (TextView) findViewById(R.id.textViewDNSList);
        mCmdSave = (Button) findViewById(R.id.cmd_save);
		mSniffToggleButton = (ToggleButton) findViewById(R.id.sniffToggleButton);
		mSniffProgress = (ProgressBar) findViewById(R.id.sniffActivity);
        mTextDnsList.setHorizontallyScrolling(true);

		mSpoofSession = new SpoofSession(false, false, null, null);

        readDNSlist();

		mSniffToggleButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mRunning) {
					setStoppedState();
				} else {
					setStartedState();
				}
			}
		});
        mCmdSave.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                saveDNSlist();
            }
        });
        }

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case android.R.id.home:

			onBackPressed();

			return true;

		default:
			return super.onOptionsItemSelected(item);
		}
	}

    private void setStoppedState() {
		mSpoofSession.stop();

		try {
			if (mBufferedWriter != null)
				mBufferedWriter.close();
		} catch (IOException e) {
			System.errorLogging(e);
		}

		mSniffProgress.setVisibility(View.INVISIBLE);
		mRunning = false;
		mSniffToggleButton.setChecked(false);
	}

    public void readDNSlist(){
        String _line="";

        try {
            BufferedReader inputReader = new BufferedReader(new FileReader(System.getContext().getFilesDir().getAbsolutePath() + "/tools/ettercap/share/etter.dns"));
            while ((_line = inputReader.readLine()) != null) {
                mTextDnsList.append(_line + "\n");
            }

            inputReader.close();
        }
        catch (Exception e){
            Logger.error("readDNSList() error: " + e.getLocalizedMessage());
        }
    }

    public void saveDNSlist(){

        try {
            Logger.info("saveDNSList() saving dnss to: " + System.getContext().getFilesDir().getAbsolutePath() + "/tools/ettercap/share/etter.dns");
            File f = new File(System.getContext().getFilesDir().getAbsolutePath() + "/tools/ettercap/share/etter.dns");
            FileOutputStream fos = new FileOutputStream(f);
            fos.write(mTextDnsList.getText().toString().getBytes());
            fos.close();

            Toast.makeText(this, "Saved", Toast.LENGTH_SHORT).show();
        }
        catch (Exception e){
            Logger.error("readDNSList() error: " + e.getLocalizedMessage());
            Toast.makeText(this, "Error: " + e.getLocalizedMessage(), Toast.LENGTH_SHORT).show();
        }
    }

	private void setStartedState() {

    try {
      mSpoofSession.start(new Ettercap.OnDNSSpoofedReceiver() {
        @Override
        public void onSpoofed(String line) {
            Logger.info("DNSSpoofing.onevent() line: " + line);

            if (line.contains("spoofed to"))
                Toast.makeText(DNSSpoofing.this, line, Toast.LENGTH_LONG).show();

        }

        @Override
        public void onStderr(String line)
        {
            if (line.contains("spoofed to"))
                Toast.makeText(DNSSpoofing.this, line, Toast.LENGTH_LONG).show();
        }

        @Override
        public void onReady(){
        }

        @Override
        public void onEnd(final int exitValue) {
          DNSSpoofing.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
              if(exitValue!=0) {
                Toast.makeText(DNSSpoofing.this, "ettercap returned #" + exitValue, Toast.LENGTH_LONG).show();
              }
              setStoppedState();
            }
          });
        }

          @Override
          public void onError(final String error) {
              DNSSpoofing.this.runOnUiThread(new Runnable() {
                  @Override
                  public void run() {
                      if (!DNSSpoofing.this.isFinishing()) {
                          new ErrorDialog(getString(R.string.error), error,
                                  DNSSpoofing.this).show();
                          setStoppedState();
                      }
                  }
              });
          }

        @Override
        public void onDeath(final int signal) {
          DNSSpoofing.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
              Toast.makeText(DNSSpoofing.this, "ettercap killed by signal #" + signal, Toast.LENGTH_LONG).show();
              setStoppedState();
            }
          });
        }
      });


      mSniffProgress.setVisibility(View.VISIBLE);
      mRunning = true;

    } catch (ChildManager.ChildNotStartedException e) {
      System.errorLogging(e);
      mSniffToggleButton.setChecked(false);
      Toast.makeText(DNSSpoofing.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
    }
	}

	@Override
	public void onBackPressed() {
		setStoppedState();
		super.onBackPressed();
        overridePendingTransition(R.anim.fadeout, R.anim.fadein);
	}

}
