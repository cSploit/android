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

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Typeface;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.System;
import org.csploit.android.gui.FileEdit;
import org.csploit.android.gui.dialogs.FatalDialog;
import org.csploit.android.tools.Ettercap.OnAccountListener;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

public class PasswordSniffer extends AppCompatActivity {
	private ToggleButton mSniffToggleButton = null;
	private ProgressBar mSniffProgress = null;
	private ExpandableListView mListView = null;
	private ListViewAdapter mAdapter = null;
	private boolean mRunning = false;
	private String mFileOutput = null;
	private FileWriter mFileWriter = null;
	private BufferedWriter mBufferedWriter = null;
	private SpoofSession mSpoofSession = null;
	private Menu mMenu = null;

	private final static int OPTIONS_FIELDS = 0;
	private final static int OPTIONS_FILTERS = 0;

	public class ListViewAdapter extends BaseExpandableListAdapter {
		private HashMap<String, ArrayList<String>> mGroups = null;
		private Context mContext = null;

		public ListViewAdapter(Context context) {
			mGroups = new HashMap<String, ArrayList<String>>();
			mContext = context;
		}

		public boolean children(String name) {
			return mGroups.containsKey(name);
		}

		public void addGroup(String name) {
			mGroups.put(name, new ArrayList<String>());
			notifyDataSetChanged();
		}

		public boolean hasChild(String group, String line) {
			ArrayList<String> children = mGroups.get(group);

			if (children != null) {
				for (String child : children) {
					if (child.equals(line))
						return true;
				}
			}

			return false;
		}

		public synchronized void addChild(String group, String child) {
			if (mGroups.get(group) == null)
				addGroup(group);

			mGroups.get(group).add(child);

			Object[] keys = mGroups.keySet().toArray();
			int groups = keys.length;

			for (int i = 0; i < groups; i++) {
				if (keys[i].toString().equals(group)) {
					mListView.expandGroup(i);
					break;
				}
			}

			notifyDataSetChanged();
		}

		private ArrayList<String> getGroupAt(int position) {
			return mGroups.get(mGroups.keySet().toArray()[position]);
		}

		@Override
		public Object getChild(int groupPosition, int childPosition) {
			return getGroupAt(groupPosition).get(childPosition);
		}

		@Override
		public long getChildId(int groupPosition, int childPosition) {
			return (groupPosition * 10) + childPosition;
		}

		@Override
		public int getChildrenCount(int groupPosition) {
			return getGroupAt(groupPosition).size();
		}

		@Override
		public Object getGroup(int groupPosition) {
			return mGroups.keySet().toArray()[groupPosition];
		}

		@Override
		public int getGroupCount() {
			return mGroups.size();
		}

		@Override
		public long getGroupId(int groupPosition) {
			return groupPosition;
		}

		@Override
		public View getGroupView(int groupPosition, boolean isExpanded,
				View convertView, ViewGroup parent) {
			TextView row = (TextView) convertView;
			if (row == null)
				row = new TextView(mContext);

			row.setText(getGroup(groupPosition).toString());
			row.setTextSize(20);
			row.setTypeface(Typeface.DEFAULT_BOLD);
			row.setPadding(50, 0, 0, 0);

			return row;
		}

		@Override
		public View getChildView(int groupPosition, int childPosition,
				boolean isLastChild, View convertView, ViewGroup parent) {
			TextView row = (TextView) convertView;
			if (row == null)
				row = new TextView(mContext);

			row.setText(getChild(groupPosition, childPosition).toString());
			row.setPadding(30, 0, 0, 0);

			return row;
		}

		@Override
		public boolean hasStableIds() {
			return true;
		}

		@Override
		public boolean isChildSelectable(int groupPosition, int childPosition) {
			return true;
		}

	}

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
		if (isDark)
			setTheme(R.style.DarkTheme);
		else
			setTheme(R.style.AppTheme);
		setTitle(System.getCurrentTarget() + " > MITM > Password Sniffer");
		setContentView(R.layout.plugin_mitm_password_sniffer);
		getSupportActionBar().setDisplayHomeAsUpEnabled(true);

		mFileOutput = (new File(System.getStoragePath(), System.getSettings()
				.getString("PREF_PASSWORD_FILENAME",
						"csploit-password-sniff.log"))).getAbsolutePath();
		mSniffToggleButton = (ToggleButton) findViewById(R.id.sniffToggleButton);
		mSniffProgress = (ProgressBar) findViewById(R.id.sniffActivity);
		mListView = (ExpandableListView) findViewById(R.id.listView);
		mAdapter = new ListViewAdapter(this);
		mSpoofSession = new SpoofSession(false, false, null, null);

		mListView.setAdapter(mAdapter);

		mSniffToggleButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mRunning) {
					setStoppedState();
				} else {
					setStartedState();
				}
			}
		});
	}

	@Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.password_sniffer, menu);

        mMenu = menu;

        return super.onCreateOptionsMenu(menu);
    }

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
			case R.id.action_fields:
				if (mSniffToggleButton.isEnabled() == false)
					Toast.makeText(this, "The changes won't take effect until you stop the current traffic sniffing", Toast.LENGTH_SHORT).show();

				Intent _fields = new Intent(PasswordSniffer.this, FileEdit.class);
				_fields.putExtra(FileEdit.KEY_FILEPATH, "/tools/ettercap/share/etter.fields");
				startActivityForResult(_fields, 0);

				return true;
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

	private void setStartedState() {
		try {
			// open file in appending mode
			mFileWriter = new FileWriter(mFileOutput, true);
			mBufferedWriter = new BufferedWriter(mFileWriter);
		} catch (IOException e) {
			new FatalDialog("Error", e.toString(), PasswordSniffer.this).show();
		}


    try {
      mSpoofSession.start(new OnAccountListener() {
        @Override
        public void onAccount(final String protocol, final String address,
            final String username, final String password) {
          final String line = username + ": " + password;
          PasswordSniffer.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
              if (mAdapter.hasChild(protocol, line) == false) {
                try {
                  mBufferedWriter.write(line + "\n");
                } catch (IOException e) {
                  System.errorLogging(e);
                }

                mAdapter.addChild(protocol, line);
              }
            }
          });
        }

        @Override
        public void onReady() {

        }

        @Override
        public void onEnd(final int exitValue) {
          PasswordSniffer.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
              if(exitValue!=0) {
                Toast.makeText(PasswordSniffer.this, "ettercap returned #" + exitValue, Toast.LENGTH_LONG).show();
              }
              setStoppedState();
            }
          });
        }

        @Override
        public void onDeath(final int signal) {
          PasswordSniffer.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
              Toast.makeText(PasswordSniffer.this, "ettercap killed by signal #" + signal, Toast.LENGTH_LONG).show();
              setStoppedState();
            }
          });
        }
      });

      Toast.makeText(PasswordSniffer.this, "Logging to " + mFileOutput,
              Toast.LENGTH_LONG).show();

      mSniffProgress.setVisibility(View.VISIBLE);
      mRunning = true;

    } catch (ChildManager.ChildNotStartedException e) {
      System.errorLogging(e);
      mSniffToggleButton.setChecked(false);
      Toast.makeText(PasswordSniffer.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
    }
	}

	@Override
	public void onBackPressed() {
		setStoppedState();
		super.onBackPressed();
		overridePendingTransition(R.anim.fadeout, R.anim.fadein);
	}
}