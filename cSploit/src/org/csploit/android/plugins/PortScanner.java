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

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.InputDialog;
import org.csploit.android.gui.dialogs.InputDialog.InputDialogListener;
import org.csploit.android.net.Network;
import org.csploit.android.net.Target;
import org.csploit.android.net.Target.Port;
import org.csploit.android.tools.NMap;

import java.util.ArrayList;

public class PortScanner extends Plugin {
	private TextView mTextDoc = null;
	private EditText mTextParameters = null;
	private ToggleButton mScanToggleButton = null;
	private ProgressBar mScanProgress = null;
	private boolean mRunning = false;
	private ArrayList<String> mPortList = null;
	private ArrayAdapter<String> mListAdapter = null;
	private Receiver mScanReceiver = null;
	private String mCustomPorts = null;
	private Menu mMenu = null;
	private static final String CUSTOM_PARAMETERS = "PortScanner.Prefs.CustomParameters";
	private static final String CUSTOM_PARAMETERS_TEXT = "PortScanner.Prefs.CustomParameters.Text";
	private SharedPreferences mPreferences = null;

	public PortScanner() {
		super(R.string.port_scanner, R.string.port_scanner_desc,

		new Target.Type[] { Target.Type.ENDPOINT, Target.Type.REMOTE },
				R.layout.plugin_portscanner, R.drawable.action_scanner);

		mPortList = new ArrayList<String>();
		mScanReceiver = new Receiver();
	}

	/**
	 * Sets visible the custom parameters text field, and loads the saved parameters
	 */
	private void displayParametersField (){
		mTextDoc.setVisibility(View.VISIBLE);
		mTextParameters.setVisibility(View.VISIBLE);
		mTextParameters.setText(mPreferences.getString(CUSTOM_PARAMETERS_TEXT, ""));

		saveCustomParameters(true);
	}

	/**
	 * Hides the custom parameters text field, saving the typed parameters
	 */
	private void hideParametersField (){
		saveCustomParameters(false);

		mTextDoc.setVisibility(View.GONE);
		mTextParameters.setVisibility(View.GONE);
	}

	/**
	 * Saves customs parameters entered by the user
	 *
	 * @param displayed Sets if the custom parameters text field must be displayed or not
	 */
	private void saveCustomParameters (boolean displayed){
		SharedPreferences.Editor edit = mPreferences.edit();
		edit.putBoolean(CUSTOM_PARAMETERS, displayed);
		edit.putString(CUSTOM_PARAMETERS_TEXT, mTextParameters.getText().toString());
		edit.apply();
	}

	private void setStoppedState() {
		if(mProcess!=null) {
      mProcess.kill();
      mProcess = null;
    }
		saveCustomParameters(mTextParameters.isShown());

		mScanProgress.setVisibility(View.INVISIBLE);
		mRunning = false;
		mScanToggleButton.setChecked(false);

		if (mPortList.size() == 0)
			Toast.makeText(this, getString(R.string.no_open_ports),
					Toast.LENGTH_LONG).show();
	}

	private void setStartedState() {
		createPortList();

    try {
		if (mTextParameters.isShown()) {
			mListAdapter.clear();
			mListAdapter.notifyDataSetChanged();
			mProcess = System.getTools().nmap
					.customScan(System.getCurrentTarget(), mScanReceiver, mTextParameters.getText().toString());
		}
		else {
			mProcess = System.getTools().nmap
					.synScan(System.getCurrentTarget(), mScanReceiver, mCustomPorts);
		}

      mRunning = true;
    } catch (ChildManager.ChildNotStartedException e) {
      System.errorLogging(e);
      Toast.makeText(PortScanner.this, getString(R.string.child_not_started) + "\n" + e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
    }
  }

  private void createPortList() {
    mPortList.clear();

    for(Port p : System.getCurrentTarget().getOpenPorts()) {
      int pNumber = p.getNumber();
      String resolvedProtocol = System.getProtocolByPort(pNumber);
      String str;

      if (resolvedProtocol != null)
        str = pNumber + " ( " + resolvedProtocol + " )";
      else
        str = p.getProtocol().toString().toLowerCase() + " : " + pNumber;

      if(!mPortList.contains(str))
        mPortList.add(str);
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

		mPreferences = System.getSettings();
		mTextDoc = (TextView) findViewById(R.id.scanDoc);
		mTextParameters = (EditText) findViewById(R.id.scanParameters);
		mScanToggleButton = (ToggleButton) findViewById(R.id.scanToggleButton);
		mScanProgress = (ProgressBar) findViewById(R.id.scanActivity);

		new Thread(new Runnable() {
			@Override
			public void run() {
				System.preloadServices();
			}
		}).start();

		if (mPreferences.getBoolean(CUSTOM_PARAMETERS, false))
			displayParametersField();

		mScanToggleButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mRunning) {
					setStoppedState();
				} else {
					setStartedState();
				}
			}
		});

		ListView mScanList = (ListView) findViewById(R.id.scanListView);

		createPortList();

		mListAdapter = new ArrayAdapter<String>(this,
				android.R.layout.simple_list_item_1, mPortList);
		mScanList.setAdapter(mListAdapter);
		mScanList.setOnItemLongClickListener(new OnItemLongClickListener() {
			@Override
			public boolean onItemLongClick(AdapterView<?> parent, View view,
					int position, long id) {
				int portNumber = System.getCurrentTarget().getOpenPorts()
						.get(position).getNumber();
				String url;

				if (portNumber == 80)
					url = "http://"
							+ System.getCurrentTarget()
									.getCommandLineRepresentation() + "/";

				else if (portNumber == 443)
					url = "https://"
							+ System.getCurrentTarget()
									.getCommandLineRepresentation() + "/";

				else if (portNumber == 21)
					url = "ftp://"
							+ System.getCurrentTarget()
									.getCommandLineRepresentation();

				else if (portNumber == 22)
					url = "ssh://"
							+ System.getCurrentTarget()
									.getCommandLineRepresentation();

				else
					url = "telnet://"
							+ System.getCurrentTarget()
									.getCommandLineRepresentation() + ":"
							+ portNumber;

				final String furl = url;

				new ConfirmDialog("Open", "Open " + url + " ?",
						PortScanner.this, new ConfirmDialogListener() {
							@Override
							public void onConfirm() {
								try {
									Intent browser = new Intent(
											Intent.ACTION_VIEW, Uri.parse(furl));

									PortScanner.this.startActivity(browser);
								} catch (ActivityNotFoundException e) {
									System.errorLogging(e);

									new ErrorDialog(
											getString(R.string.error),
											getString(R.string.no_activities_for_url),
											PortScanner.this).show();
								}

							}

							@Override
							public void onCancel() {
							}
						}).show();

				return false;
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.port_scanner, menu);
		mMenu = menu;
		mMenu.findItem(R.id.scanner_custom_parameters).
				setChecked(mPreferences.getBoolean(CUSTOM_PARAMETERS, false));

		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
			case R.id.scanner_custom_parameters:
				if (item.isChecked())
					hideParametersField();
				else
					displayParametersField();

				item.setChecked(!item.isChecked());
			return true;
		case R.id.select_ports:

			new InputDialog(getString(R.string.select_ports),
					getString(R.string.enter_ports_list), this,
					new InputDialogListener() {
						@Override
						public void onInputEntered(String input) {
							input = input.trim();

							if (!input.isEmpty()) {
								String[] ports = input.split("[^\\d]+");
								for (String port : ports) {
									try {
										if (port.isEmpty())
											throw new Exception(
													getString(R.string.invalid_port_)
															+ port + "'.");

										else {
											int iport = Integer.parseInt(port);
											if (iport <= 0 || iport > 65535)
												throw new Exception(
														getString(R.string.port_must_be_greater));
										}
									} catch (Exception e) {
										new ErrorDialog("Error", e.toString(),
												PortScanner.this).show();
										return;
									}
								}

								mCustomPorts = "";
								for (int i = 0, last = ports.length - 1; i < ports.length; i++) {
									mCustomPorts += ports[i];
									if (i != last)
										mCustomPorts += ",";
								}

								if (mCustomPorts.isEmpty()) {
									mCustomPorts = null;
									new ErrorDialog(getString(R.string.error),
											getString(R.string.invalid_ports),
											PortScanner.this).show();
								}

								hideParametersField();
								mMenu.findItem(R.id.scanner_custom_parameters).setChecked(false);

								Logger.debug("mCustomPorts = " + mCustomPorts);
							} else
								new ErrorDialog(getString(R.string.error),
										getString(R.string.empty_port_list),
										PortScanner.this).show();
						}
					}).show();

			return true;

		default:
			return super.onOptionsItemSelected(item);
		}
	}

	@Override
	public void onBackPressed() {
		setStoppedState();
		super.onBackPressed();
	}

	private class Receiver extends NMap.SynScanReceiver {
		@Override
		public void onStart(String commandLine) {
			super.onStart(commandLine);

			PortScanner.this.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					mRunning = true;
					mScanProgress.setVisibility(View.VISIBLE);
				}
			});
		}

		@Override
		public void onEnd(int exitCode) {
			super.onEnd(exitCode);

			PortScanner.this.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					setStoppedState();
				}
			});
		}

    @Override
    public void onPortFound(final int port, String protocol) {
			final String portProtocol = protocol;
      final String resolvedProtocol = System.getProtocolByPort(port);

      System.addOpenPort(port, Network.Protocol.fromString(protocol));

			PortScanner.this.runOnUiThread(new Runnable() {
				@Override
				public void run() {
          String entry;

					if (resolvedProtocol != null) {
						entry = port + " ( " + resolvedProtocol + " )";
					} else {
						entry = portProtocol + " : " + port;
					}

					// add open port to the listview and notify the environment
					// about the event
          if(!mPortList.contains(entry)) {
					  mPortList.add(entry);
					  mListAdapter.notifyDataSetChanged();
          }
				}
			});
		}
	}
}
