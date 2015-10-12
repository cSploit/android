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
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.net.Target;
import org.csploit.android.tools.NMap;

public class Traceroute extends Plugin {
	private FloatingActionButton mTraceFloatingActionButton = null;
	private ProgressBar mTraceProgress = null;
	private boolean mRunning = false;
	private ArrayAdapter<String> mListAdapter = null;
	private Receiver mTraceReceiver = null;
	private boolean resolveNames = false;

	public Traceroute() {
		super(R.string.trace, R.string.trace_desc,

		new Target.Type[] { Target.Type.ENDPOINT, Target.Type.REMOTE },
				R.layout.plugin_traceroute, R.drawable.action_traceroute);

		mTraceReceiver = new Receiver();
	}

	private void setStoppedState() {
		if(mProcess!=null) {
      mProcess.kill();
      mProcess = null;
    }
		mTraceProgress.setVisibility(View.INVISIBLE);
		mRunning = false;
		mTraceFloatingActionButton.setImageDrawable(ContextCompat.getDrawable(this, R.drawable.ic_play_arrow_24dp));
	}

	private void setStartedState() {
		mListAdapter.clear();
		mTraceFloatingActionButton.setImageDrawable(ContextCompat.getDrawable(this, R.drawable.ic_stop_24dp));

    try {
      System.getTools().nmap.trace(System.getCurrentTarget(), resolveNames, mTraceReceiver);

      mRunning = true;
    } catch (ChildManager.ChildNotStartedException e) {
      System.errorLogging(e);
      Toast.makeText(Traceroute.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
    }
  }

	@Override
	public void onCreate(Bundle savedInstanceState) {
		SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
		if (isDark)
			setTheme(R.style.DarkTheme);
		else
			setTheme(R.style.AppTheme);
		super.onCreate(savedInstanceState);

		mTraceFloatingActionButton = (FloatingActionButton) findViewById(R.id.traceToggleButton);
		mTraceProgress = (ProgressBar) findViewById(R.id.traceActivity);

		mTraceFloatingActionButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mRunning) {
					setStoppedState();
				} else {
					setStartedState();
				}
			}
		});

		ListView mTraceList = (ListView) findViewById(R.id.traceListView);

		mListAdapter = new ArrayAdapter<String>(this,
				android.R.layout.simple_list_item_1);
		mTraceList.setAdapter(mListAdapter);
	}

  @Override
  public boolean onCreateOptionsMenu(Menu menu){
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.traceroute, menu);
    return super.onCreateOptionsMenu(menu);
  }

  @Override
  public boolean onPrepareOptionsMenu(Menu menu) {
    MenuItem item = menu.findItem(R.id.resolve_names);
    if(item != null) {
      item.setChecked(resolveNames);
    }
    return super.onPrepareOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    switch (item.getItemId()) {
      case R.id.resolve_names:
        resolveNames=!item.isChecked();
        item.setChecked(resolveNames);
        return true;
      default:
        return super.onOptionsItemSelected(item);
    }
  }

	@Override
	public void onBackPressed() {
		setStoppedState();
		super.onBackPressed();
		overridePendingTransition(R.anim.fadeout, R.anim.fadein);
	}

	private class Receiver extends NMap.TraceReceiver {
		@Override
		public void onStart(String commandLine) {
			super.onStart(commandLine);

			Traceroute.this.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					mRunning = true;
					mTraceProgress.setVisibility(View.VISIBLE);
				}
			});
		}

		@Override
		public void onEnd(int exitCode) {
			super.onEnd(exitCode);

			Traceroute.this.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					setStoppedState();
				}
			});
		}

    private String formatTime(long usec) {

      if(usec == 0)
        return "0 ms";

      long msec = (usec / 1000);
      long sec = (msec / 1000);
      long min = (sec / 60);

      usec %= 1000;
      msec %= 1000;
      sec %= 60;
      min %= 60;

      if(min > 0) {
        return min + "m " + sec + " s";
      } else if(sec > 0) {
        return sec + "." + msec + " s";
      } else if(msec > 0) {
        return msec + "." + usec + " ms";
      } else {
        return "0." + usec + " ms";
      }
    }

    @Override
    public void onHop(int hop, final long usec, final String address, final String name) {

			Traceroute.this.runOnUiThread(new Runnable() {
				@Override
				public void run() {

          String addr;

          if (name != null && !name.isEmpty()) {
            addr = name + " [" + address + "]";
          } else {
            addr = address;
          }

					if (usec > 0)
						mListAdapter.add(addr + " ( " + formatTime(usec) + " )");

					else
						mListAdapter.add(addr + getString(R.string.untraced_hops));

					mListAdapter.notifyDataSetChanged();
				}
			});
		}
	}
}
