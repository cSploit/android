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

import static java.lang.Thread.MAX_PRIORITY;
import static org.csploit.android.core.UpdateChecker.AVAILABLE_VERSION;
import static org.csploit.android.core.UpdateChecker.CORE_AVAILABLE;
import static org.csploit.android.core.UpdateChecker.GEMS_AVAILABLE;
import static org.csploit.android.core.UpdateChecker.MSF_AVAILABLE;
import static org.csploit.android.core.UpdateChecker.RUBY_AVAILABLE;
import static org.csploit.android.core.UpdateChecker.UPDATE_AVAILABLE;
import static org.csploit.android.core.UpdateChecker.UPDATE_CHECKING;
import static org.csploit.android.core.UpdateChecker.UPDATE_NOT_AVAILABLE;
import static org.csploit.android.net.NetworkRadar.NRDR_STOPPED;
import static org.csploit.android.net.NetworkRadar.TARGET_UPDATE;

import org.csploit.android.core.Child;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.ManagedReceiver;
import org.csploit.android.core.MultiAttackService;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.core.UpdateChecker;
import org.csploit.android.core.UpdateService;
import org.csploit.android.events.Event;
import org.csploit.android.gui.dialogs.AboutDialog;
import org.csploit.android.gui.dialogs.ChangelogDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.FatalDialog;
import org.csploit.android.gui.dialogs.InputDialog;
import org.csploit.android.gui.dialogs.InputDialog.InputDialogListener;
import org.csploit.android.gui.dialogs.MultipleChoiceDialog;
import org.csploit.android.gui.dialogs.SpinnerDialog;
import org.csploit.android.gui.dialogs.SpinnerDialog.SpinnerDialogListener;
import org.csploit.android.net.Network;
import org.csploit.android.net.NetworkRadar;
import org.csploit.android.net.Target;
import org.csploit.android.net.metasploit.RPCClient;
import org.csploit.android.plugins.ExploitFinder;
import org.csploit.android.plugins.Inspector;
import org.csploit.android.plugins.LoginCracker;
import org.csploit.android.plugins.PacketForger;
import org.csploit.android.plugins.PortScanner;
import org.csploit.android.plugins.RouterPwn;
import org.csploit.android.plugins.Sessions;
import org.csploit.android.plugins.Traceroute;
import org.csploit.android.plugins.VulnerabilityFinder;
import org.csploit.android.plugins.mitm.MITM;
import org.csploit.android.net.metasploit.MsfRpcd;
import org.csploit.android.tools.ToolBox;

import java.io.IOException;
import java.net.NoRouteToHostException;
import java.util.ArrayList;

import android.annotation.SuppressLint;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Build;
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
import com.actionbarsherlock.view.ActionMode;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;

@SuppressLint("NewApi")
public class MainActivity extends SherlockListActivity {
	private String UPDATE_MESSAGE;
	private static final int WIFI_CONNECTION_REQUEST = 1012;
	private boolean isWifiAvailable = false;
	private TargetAdapter mTargetAdapter = null;
  private NetworkRadar mNetworkRadar = null;
  private MsfRpcd mMsfRpcd = null;
	private RadarReceiver mRadarReceiver = new RadarReceiver();
	private UpdateReceiver mUpdateReceiver = new UpdateReceiver();
	private WipeReceiver mWipeReceiver = new WipeReceiver();
  private UpdateChecker mUpdateChecker = null;
	private Menu mMenu = null;
	private TextView mUpdateStatus = null;
	private Toast mToast = null;
  private ProgressDialog progressDialog;
	private long mLastBackPressTime = 0;
  private ActionMode mActionMode = null;
  private boolean mInitialization = false;

	private void createUpdateLayout() {

		getListView().setVisibility(View.GONE);
		findViewById(R.id.textView).setVisibility(View.GONE);

		RelativeLayout layout = (RelativeLayout) findViewById(R.id.layout);
		LayoutParams params = new LayoutParams(LayoutParams.MATCH_PARENT,
				LayoutParams.MATCH_PARENT);

		mUpdateStatus = new TextView(this);

		mUpdateStatus.setGravity(Gravity.CENTER);
		mUpdateStatus.setLayoutParams(params);
		mUpdateStatus
				.setText(UPDATE_MESSAGE.replace("#STATUS#", "..."));

		layout.addView(mUpdateStatus);

		mUpdateReceiver.register(MainActivity.this);

		startUpdateChecker();
		stopNetworkRadar(true);

		if (Build.VERSION.SDK_INT >= 11)
			invalidateOptionsMenu();
	}

	private void createOfflineLayout() {

		getListView().setVisibility(View.GONE);
		findViewById(R.id.textView).setVisibility(View.GONE);

		RelativeLayout layout = (RelativeLayout) findViewById(R.id.layout);
		LayoutParams params = new LayoutParams(LayoutParams.MATCH_PARENT,
				LayoutParams.MATCH_PARENT);

		mUpdateStatus = new TextView(this);

		mUpdateStatus.setGravity(Gravity.CENTER);
		mUpdateStatus.setLayoutParams(params);
		mUpdateStatus.setText(getString(R.string.no_connectivity));

		layout.addView(mUpdateStatus);

		stopNetworkRadar(true);
		if (Build.VERSION.SDK_INT >= 11)
			invalidateOptionsMenu();
	}

	public void createOnlineLayout() {
    if(mTargetAdapter != null)
      return;

		mTargetAdapter = new TargetAdapter();

		setListAdapter(mTargetAdapter);

		getListView().setOnItemLongClickListener(new OnItemLongClickListener() {
      @Override
      public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        Target t = System.getTarget(position);
        if (t.getType() == Target.Type.NETWORK) {
          if (mActionMode == null)
            targetAliasPrompt(t);
          return true;
        }
        if (mActionMode == null) {
          mTargetAdapter.clearSelection();
          mActionMode = startActionMode(mActionModeCallback);
        }
        mTargetAdapter.toggleSelection(position);
        return true;
      }
    });

		mRadarReceiver.register(MainActivity.this);
		mUpdateReceiver.register(MainActivity.this);
		mWipeReceiver.register(MainActivity.this);

		startUpdateChecker();
		startNetworkRadar(false);
		StartRPCServer();

		// if called for the second time after wifi connection
		if (Build.VERSION.SDK_INT >= 11)
			invalidateOptionsMenu();
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode,
			Intent intent) {
		if (requestCode == WIFI_CONNECTION_REQUEST && resultCode == RESULT_OK
				&& intent.hasExtra(WifiScannerActivity.CONNECTED)) {
			System.reloadNetworkMapping();
			onCreate(null);
		}
	}

  private void createLayout() {
    boolean wifiAvailable = Network.isWifiConnected(this);
    boolean connectivityAvailable = wifiAvailable || Network.isConnectivityAvailable(this);
    boolean coreBeating = System.isCoreInitialized();

    if(coreBeating && wifiAvailable) {
      createOnlineLayout();
    } else if(connectivityAvailable) {
      createUpdateLayout();
    } else {
      createOfflineLayout();
    }
  }

  private void onInitializationStart() {
    if(progressDialog == null) {
      progressDialog = ProgressDialog.show(MainActivity.this, "",
              getString(R.string.initializing), true, false);
    } else {
      progressDialog.show();
    }

    mInitialization = true;
  }

  private void onInitializationError(final String message) {
    MainActivity.this.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        progressDialog.dismiss();
        new FatalDialog(getString(R.string.initialization_error),
                message, message.contains(">"),
                MainActivity.this).show();
      }
    });
    mInitialization = false;
  }

  private void onInitializationSuccess() {
    MainActivity.this.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        try {
          if (progressDialog != null)
            progressDialog.dismiss();
        } catch (Exception e) {
          new FatalDialog(getString(R.string.error), e
                  .getMessage(), MainActivity.this)
                  .show();
          return;
        }

        if (!System.getAppVersionName().equals(
                System.getSettings().getString(
                        "CSPLOIT_INSTALLED_VERSION", "0"))) {
          new ChangelogDialog(MainActivity.this).show();
          Editor editor = System.getSettings().edit();
          editor.putString("CSPLOIT_INSTALLED_VERSION",
                  System.getAppVersionName());
          editor.apply();
        }
      }
    });
    mInitialization = false;
  }

  //TODO: this is horrible...
  //TODO: create an "heartless" layout ... OK
  //TODO: start UpdateChecker ... OK
  //TODO: start UpdateService ... OK
  //TODO: reinit

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
    boolean connectivityAvailable;

		if (isDark)
			setTheme(R.style.Sherlock___Theme);
		else
			setTheme(R.style.AppTheme);
	
		setContentView(R.layout.target_layout);
		isWifiAvailable = Network.isWifiConnected(this);
    connectivityAvailable = isWifiAvailable || Network.isConnectivityAvailable(this);

    // make sure system object was correctly initialized during application
    // startup
    if(!System.isInitialized()) {
      // wifi available but system failed to initialize, this is a fatal
      // :(
      if(isWifiAvailable) {

        // retry
        try {
          onInitializationStart();

          System.init(MainActivity.this.getApplicationContext());

          System.registerPlugin(new RouterPwn());
          System.registerPlugin(new Traceroute());
          System.registerPlugin(new PortScanner());
          System.registerPlugin(new Inspector());
          System.registerPlugin(new VulnerabilityFinder());
          System.registerPlugin(new ExploitFinder());
          System.registerPlugin(new LoginCracker());
          System.registerPlugin(new Sessions());
          System.registerPlugin(new MITM());
          System.registerPlugin(new PacketForger());
        } catch (Exception e) {
          if(!(e instanceof NoRouteToHostException))
            System.errorLogging(e);

          onInitializationError(System.getLastError());

          return;
        }
      }
    }

    boolean coreInstalled = System.isCoreInstalled();
    boolean coreBeating = System.isCoreInitialized();

    if(coreInstalled && !coreBeating) {
      try {
        onInitializationStart();
        System.initCore();
        coreBeating = true;
      } catch (System.SuException e) {
        onInitializationError(getString(R.string.only_4_root));
        return;
      } catch (System.DaemonException e) {
        Logger.error(e.getMessage());
      }
    }

    if(!connectivityAvailable) {
      if(!coreInstalled) {
        onInitializationError(getString(R.string.no_core_no_connectivity));
        return;
      } else if(!coreBeating) {
        onInitializationError(getString(R.string.heart_attack));
        return;
      }
    }

    if(!coreInstalled) {
      onInitializationStart();
      UPDATE_MESSAGE = getString(R.string.missing_core_update);
    } else if(!coreBeating) {
      onInitializationStart();
      UPDATE_MESSAGE = getString(R.string.heart_attack_update);
    } else if(!isWifiAvailable) {
      UPDATE_MESSAGE = getString(R.string.no_wifi_available);
    }

    createLayout();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getSupportMenuInflater();
		inflater.inflate(R.menu.main, menu);

		if (!isWifiAvailable) {
			menu.findItem(R.id.add).setVisible(false);
			menu.findItem(R.id.scan).setVisible(false);
			menu.findItem(R.id.new_session).setEnabled(false);
			menu.findItem(R.id.save_session).setEnabled(false);
			menu.findItem(R.id.restore_session).setEnabled(false);
			menu.findItem(R.id.ss_monitor).setEnabled(false);
			menu.findItem(R.id.ss_msfrpcd).setEnabled(false);
		}

		mMenu = menu;

		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		MenuItem item = menu.findItem(R.id.ss_monitor);

		if (mNetworkRadar != null && mNetworkRadar.isRunning())
			item.setTitle(getString(R.string.stop_monitor));
		else
			item.setTitle(getString(R.string.start_monitor));

		item = menu.findItem(R.id.ss_msfrpcd);
    ToolBox tools = System.getTools();

    if(MsfRpcd.isLocal()) {
      if (System.getMsfRpc() != null
              || (mMsfRpcd != null && mMsfRpcd.isRunning()))
        item.setTitle(getString(R.string.stop_msfrpcd));
      else
        item.setTitle(getString(R.string.start_msfrpcd));
    } else {
      if(System.getMsfRpc()==null)
        item.setTitle(getString(R.string.connect_msf));
      else
        item.setTitle(getString(R.string.disconnect_msf));
    }

    item.setEnabled(!MsfRpcd.isLocal() ||
                    (tools != null && tools.msf.isEnabled() &&
                            !System.isServiceRunning("org.csploit.android.core.UpdateService")));

		mMenu = menu;

		return super.onPrepareOptionsMenu(menu);
	}

  private void targetAliasPrompt(final Target target) {

    new InputDialog(getString(R.string.target_alias),
            getString(R.string.set_alias),
            target.hasAlias() ? target.getAlias() : "", true,
            false, MainActivity.this, new InputDialogListener() {
              @Override
              public void onInputEntered(String input) {
                target.setAlias(input);
                mTargetAdapter.notifyDataSetChanged();
              }
    }).show();
  }

  private ActionMode.Callback mActionModeCallback = new ActionMode.Callback() {

    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
      MenuInflater inflater = mode.getMenuInflater();
      inflater.inflate(R.menu.main_multi, menu);
      return true;
    }

    public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
      int i = mTargetAdapter.getSelectedCount();
      mode.setTitle(i + " " + getString((i>1 ? R.string.targets_selected : R.string.target_selected)));
      MenuItem item = menu.findItem(R.id.multi_action);
      if(item!=null)
        item.setIcon((i>1 ? android.R.drawable.ic_dialog_dialer : android.R.drawable.ic_menu_edit));
      return false;
    }

    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
      ArrayList<Plugin> commonPlugins = null;

      switch (item.getItemId()) {
        case R.id.multi_action:
          final int[] selected = mTargetAdapter.getSelectedPositions();
          if(selected.length>1) {
             commonPlugins = System.getPluginsForTarget(System.getTarget(selected[0]));
            for(int i =1; i <selected.length;i++) {
              ArrayList<Plugin> targetPlugins = System.getPluginsForTarget(System.getTarget(selected[i]));
              ArrayList<Plugin> removeThem = new ArrayList<Plugin>();
              for(Plugin p : commonPlugins) {
                if(!targetPlugins.contains(p))
                  removeThem.add(p);
              }
              for(Plugin p : removeThem) {
                commonPlugins.remove(p);
              }
            }
            if(commonPlugins.size()>0) {
              final int[] actions = new int[commonPlugins.size()];
              for(int i=0; i<actions.length; i++)
                actions[i] = commonPlugins.get(i).getName();

              (new MultipleChoiceDialog(R.string.choose_method,actions, MainActivity.this, new MultipleChoiceDialog.MultipleChoiceDialogListener() {
                @Override
                public void onChoice(int[] choices) {
                  Intent intent = new Intent(MainActivity.this,MultiAttackService.class);
                  int[] selectedActions = new int[choices.length];
                  int j=0;

                  for(int i =0; i< selectedActions.length;i++)
                    selectedActions[i] = actions[choices[i]];

                  intent.putExtra(MultiAttackService.MULTI_TARGETS, selected);
                  intent.putExtra(MultiAttackService.MULTI_ACTIONS, selectedActions);

                  startService(intent);
                }
              })).show();
            } else {
              (new ErrorDialog(getString(R.string.error),"no common actions found", MainActivity.this)).show();
            }
          } else {
            targetAliasPrompt(System.getTarget(selected[0]));
          }
          mode.finish(); // Action picked, so close the CAB
          return true;
        default:
          return false;
      }
    }

    // called when the user exits the action mode
    public void onDestroyActionMode(ActionMode mode) {
      mActionMode = null;
      mTargetAdapter.clearSelection();
    }
  };

	public void startUpdateChecker() {
		if (System.getSettings().getBoolean("PREF_CHECK_UPDATES", true)) {

      if(mUpdateChecker == null)
			  mUpdateChecker = new UpdateChecker(this);

      if(!mUpdateChecker.isAlive())
        mUpdateChecker.start();
		}
	}

	public void startNetworkRadar(boolean silent) {
		stopNetworkRadar(silent);

		if(mNetworkRadar == null) {
      mNetworkRadar = new NetworkRadar(this);
    }

    try {
      mNetworkRadar.start();

      if (!silent)
        Toast.makeText(this, getString(R.string.net_discovery_started),
                Toast.LENGTH_SHORT).show();
    } catch (ChildManager.ChildNotStartedException e) {
      Toast.makeText(this, "cannot start process", Toast.LENGTH_LONG).show();
    }
	}

	public void stopNetworkRadar(boolean silent) {
		if (mNetworkRadar != null && mNetworkRadar.isRunning()) {
      mNetworkRadar.stop(!silent);
		}
	}

  /**
   * start MSF RPC Daemon
   */
	public void StartRPCServer() {
    StopRPCServer(true);

    if(mMsfRpcd == null)
      mMsfRpcd = new MsfRpcd();

    new Thread( new Runnable() {
      @Override
      public void run() {
        SharedPreferences prefs = System.getSettings();

        final String msfHost     = prefs.getString("MSF_RPC_HOST", "127.0.0.1");
        final String msfUser     = prefs.getString("MSF_RPC_USER", "msf");
        final String msfPassword = prefs.getString("MSF_RPC_PSWD", "msf");
        final int msfPort        = System.MSF_RPC_PORT;
        final boolean msfSsl     = prefs.getBoolean("MSF_RPC_SSL", false);

        if(msfHost.equals("127.0.0.1")) {
          try {
            mMsfRpcd.start(msfUser, msfPassword, msfPort, msfSsl, new MsfRpcd.MsfRpcdReceiver() {
              @Override
              public void onReady() {
                try {
                  System.setMsfRpc(new RPCClient(msfHost, msfUser, msfPassword, msfPort, msfSsl));
                  Logger.info("successfully connected to MSF RPC Daemon ");
                  MainActivity.this.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                      Toast.makeText(MainActivity.this, "connected to MSF RPC Daemon", Toast.LENGTH_SHORT).show();
                    }
                  });
                } catch (Exception e) {
                  Logger.error(e.getClass().getName() + ": " + e.getMessage());
                  MainActivity.this.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                      Toast.makeText(MainActivity.this, "connection to MSF RPC Daemon failed", Toast.LENGTH_LONG).show();
                    }
                  });
                }
              }

              @Override
              public void onEnd(final int exitValue) {
                MainActivity.this.runOnUiThread(new Runnable() {
                  @Override
                  public void run() {
                    Toast.makeText(MainActivity.this, "MSF RPC Daemon returned #" + exitValue, Toast.LENGTH_LONG).show();
                  }
                });
              }

              @Override
              public void onDeath(final int signal) {
                MainActivity.this.runOnUiThread(new Runnable() {
                  @Override
                  public void run() {
                    Toast.makeText(MainActivity.this, " MSF RPC Daemon killed by signal #" + signal, Toast.LENGTH_LONG).show();
                  }
                });
              }
            });
          } catch (ChildManager.ChildNotStartedException e) {
            Logger.error(e.getMessage());
            MainActivity.this.runOnUiThread(new Runnable() {
              @Override
              public void run() {
                Toast.makeText(MainActivity.this, "cannot start process", Toast.LENGTH_LONG).show();
              }
            });
          }
        } else {
          try {
            System.setMsfRpc(new RPCClient(msfHost, msfUser, msfPassword, msfPort, msfSsl));
            Logger.info("successfully connected to MSF RPC Daemon ");
            MainActivity.this.runOnUiThread(new Runnable() {
              @Override
              public void run() {
                Toast.makeText(MainActivity.this, "connected to MSF RPC Daemon", Toast.LENGTH_SHORT).show();
              }
            });
          } catch (Exception e) {
            Logger.error(e.getClass().getName() + ": " + e.getMessage());
            MainActivity.this.runOnUiThread(new Runnable() {
              @Override
              public void run() {
                Toast.makeText(MainActivity.this, "connection to MSF RPC Daemon failed", Toast.LENGTH_LONG).show();
              }
            });
          }
        }
      }
    }).start();
	}

  /**
   * stop MSF RPC Daemon
   * @param silent show an information Toast if {@code false}
   */
	public void StopRPCServer(final boolean silent) {

    if(System.getMsfRpc() == null && ( mMsfRpcd == null || !mMsfRpcd.isRunning() ))
      return;

    new Thread( new Runnable() {
      @Override
      public void run() {
        try {
          System.setMsfRpc(null);

          if(mMsfRpcd.isRunning())
            mMsfRpcd.stop();

          if(!silent) {
            MainActivity.this.runOnUiThread(new Runnable() {
              @Override
              public void run() {
                Toast.makeText(MainActivity.this, getString(R.string.rpcd_stopped), Toast.LENGTH_SHORT).show();
              }
            });
          }
        } catch (InterruptedException e) {
          Logger.error("interrupted while stopping rpc daemon");
        }
      }
    }).start();
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		
		case R.id.add:
			new InputDialog(getString(R.string.add_custom_target),
					getString(R.string.enter_url), MainActivity.this,
					new InputDialogListener() {
						@Override
						public void onInputEntered(String input) {
							final Target target = Target.getFromString(input);
							if (target != null) {
								// refresh the target listview
								MainActivity.this.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										if (System.addOrderedTarget(target)
												&& mTargetAdapter != null) {
											mTargetAdapter
													.notifyDataSetChanged();
										}
									}
								});
							} else
								new ErrorDialog(getString(R.string.error),
										getString(R.string.invalid_target),
										MainActivity.this).show();
						}
					}).show();
			return true;

		case R.id.scan:
			if (mMenu != null)
				mMenu.findItem(R.id.scan).setActionView(new ProgressBar(this));

			new Thread(new Runnable() {
				@Override
				public void run() {
					startNetworkRadar(true);

					MainActivity.this.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							if (mMenu != null)
								mMenu.findItem(R.id.scan).setActionView(null);
						}
					});
				}
			}).start();

			item.setTitle(getString(R.string.stop_monitor));
			return true;

		case R.id.wifi_scan:
			stopNetworkRadar(true);

      mRadarReceiver.unregister();
      mUpdateReceiver.unregister();

			startActivityForResult(new Intent(MainActivity.this,
					WifiScannerActivity.class), WIFI_CONNECTION_REQUEST);
			return true;

		case R.id.new_session:
			new ConfirmDialog(getString(R.string.warning),
					getString(R.string.warning_new_session), this,
					new ConfirmDialogListener() {
						@Override
						public void onConfirm() {
							try {
								System.reset();
								mTargetAdapter.notifyDataSetChanged();

								Toast.makeText(
										MainActivity.this,
										getString(R.string.new_session_started),
										Toast.LENGTH_SHORT).show();
							} catch (Exception e) {
								new FatalDialog(getString(R.string.error), e
										.toString(), MainActivity.this).show();
							}
						}

						@Override
						public void onCancel() {
						}

					}).show();

			return true;

		case R.id.save_session:
			new InputDialog(getString(R.string.save_session),
					getString(R.string.enter_session_name),
					System.getSessionName(), true, false, MainActivity.this,
					new InputDialogListener() {
						@Override
						public void onInputEntered(String input) {
							String name = input.trim().replace("/", "")
									.replace("..", "");

							if (!name.isEmpty()) {
								try {
									String filename = System.saveSession(name);

									Toast.makeText(
											MainActivity.this,
											getString(R.string.session_saved_to)
													+ filename + " .",
											Toast.LENGTH_SHORT).show();
								} catch (IOException e) {
									new ErrorDialog(getString(R.string.error),
											e.toString(), MainActivity.this)
											.show();
								}
							} else
								new ErrorDialog(getString(R.string.error),
										getString(R.string.invalid_session),
										MainActivity.this).show();
						}
					}).show();
			return true;

		case R.id.restore_session:
			final ArrayList<String> sessions = System
					.getAvailableSessionFiles();

			if (sessions != null && sessions.size() > 0) {
				new SpinnerDialog(getString(R.string.select_session),
						getString(R.string.select_session_file),
						sessions.toArray(new String[sessions.size()]),
						MainActivity.this, new SpinnerDialogListener() {
							@Override
							public void onItemSelected(int index) {
								String session = sessions.get(index);

								try {
									System.loadSession(session);
									mTargetAdapter.notifyDataSetChanged();
								} catch (Exception e) {
									e.printStackTrace();
									new ErrorDialog(getString(R.string.error),
											e.getMessage(), MainActivity.this)
											.show();
								}
							}
						}).show();
			} else
				new ErrorDialog(getString(R.string.error),
						getString(R.string.no_session_found), MainActivity.this)
						.show();
			return true;

		case R.id.settings:
			startActivity(new Intent(MainActivity.this, SettingsActivity.class));
			return true;

		case R.id.ss_monitor:
			if (mNetworkRadar != null && mNetworkRadar.isRunning()) {
				stopNetworkRadar(false);

				item.setTitle(getString(R.string.start_monitor));
			} else {
        try {
          startNetworkRadar(false);

          item.setTitle(getString(R.string.stop_monitor));
        } catch (Exception e) {
          new ErrorDialog(getString(R.string.error), e.getMessage(), MainActivity.this).show();
        }
			}
			return true;

		case R.id.ss_msfrpcd:
      if(System.getMsfRpc()!=null || (mMsfRpcd != null && mMsfRpcd.isRunning())) {
				StopRPCServer(false);
        if(MsfRpcd.isLocal())
				  item.setTitle(R.string.start_msfrpcd);
        else
          item.setTitle(R.string.connect_msf);
			} else {
				StartRPCServer();
        if(MsfRpcd.isLocal())
				  item.setTitle(R.string.stop_msfrpcd);
        else
          item.setTitle(R.string.disconnect_msf);
			}
			return true;

		case R.id.submit_issue:
			String uri = getString(R.string.github_issues);
			Intent browser = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
			startActivity(browser);
			return true;

		case R.id.about:
			new AboutDialog(this).show();
			return true;

		default:
			return super.onOptionsItemSelected(item);

		}

	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		super.onListItemClick(l, v, position, id);

    if(mActionMode!=null) {
      ((TargetAdapter)getListAdapter()).toggleSelection(position);
      return;
    }

		new Thread(new Runnable() {
			@Override
			public void run() {

				startActivityForResult(new Intent(MainActivity.this,
						ActionActivity.class), WIFI_CONNECTION_REQUEST);

				overridePendingTransition(R.anim.slide_in_left,
						R.anim.slide_out_left);
			}
		}).start();

		System.setCurrentTarget(position);
		Toast.makeText(MainActivity.this,
				getString(R.string.selected_) + System.getCurrentTarget(),
				Toast.LENGTH_SHORT).show();
	}

	@Override
	public void onBackPressed() {
		if (mLastBackPressTime < java.lang.System.currentTimeMillis() - 4000) {
			mToast = Toast.makeText(this, getString(R.string.press_back),
					Toast.LENGTH_SHORT);
			mToast.show();
			mLastBackPressTime = java.lang.System.currentTimeMillis();
		} else {
			if (mToast != null)
				mToast.cancel();

			new ConfirmDialog(getString(R.string.exit),
					getString(R.string.close_confirm), this,
					new ConfirmDialogListener() {
						@Override
						public void onConfirm() {
							MainActivity.this.finish();
						}

						@Override
						public void onCancel() {
						}
					}).show();

			mLastBackPressTime = 0;
		}
	}

	@Override
	public void onDestroy() {
		stopNetworkRadar(true);
		StopRPCServer(true);

    mRadarReceiver.unregister();
    mUpdateReceiver.unregister();
    mWipeReceiver.unregister();

		// make sure no zombie process is running before destroying the activity
		System.clean(true);

		super.onDestroy();
	}

	public class TargetAdapter extends ArrayAdapter<Target> {
		public TargetAdapter() {
			super(MainActivity.this, R.layout.target_list_item);
		}

		@Override
		public int getCount() {
			return System.getTargets().size();
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View row = convertView;
			TargetHolder holder;

			if (row == null) {
				LayoutInflater inflater = (LayoutInflater) MainActivity.this
						.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
				row = inflater
						.inflate(R.layout.target_list_item, parent, false);

				holder = new TargetHolder();
				holder.itemImage = (ImageView) (row != null ? row
						.findViewById(R.id.itemIcon) : null);
				holder.itemTitle = (TextView) (row != null ? row
						.findViewById(R.id.itemTitle) : null);
				holder.itemDescription = (TextView) (row != null ? row
						.findViewById(R.id.itemDescription) : null);

				if (row != null)
					row.setTag(holder);
			} else
				holder = (TargetHolder) row.getTag();

			Target target = System.getTarget(position);

			if (target.hasAlias())
				holder.itemTitle.setText(Html.fromHtml("<b>"
						+ target.getAlias() + "</b> <small>( "
						+ target.getDisplayAddress() + " )</small>"));

			else
				holder.itemTitle.setText(target.toString());

      holder.itemTitle.setTextColor(getResources().getColor((target.isConnected() ? R.color.app_color : R.color.gray_text)));

      if(row!=null)
        row.setBackgroundColor(getResources().getColor((target.isSelected() ? R.color.abs__background_holo_dark : android.R.color.transparent)));

			holder.itemTitle.setTypeface(null, Typeface.NORMAL);
			holder.itemImage.setImageResource(target.getDrawableResourceId());
			holder.itemDescription.setText(target.getDescription());

			return row;
		}

    public void clearSelection() {
      for(Target t : System.getTargets())
        t.setSelected(false);
      notifyDataSetChanged();
      if(mActionMode!=null)
        mActionMode.finish();
    }

    public void toggleSelection(int position) {
      Target t = System.getTarget(position);
      t.setSelected(!t.isSelected());
      notifyDataSetChanged();
      if(mActionMode!=null) {
        if(getSelectedCount() > 0)
          mActionMode.invalidate();
        else
          mActionMode.finish();
      }
    }

    public int getSelectedCount() {
      int i = 0;
      for(Target t : System.getTargets())
        if(t.isSelected())
          i++;
      return i;
    }

    public ArrayList<Target> getSelected() {
      ArrayList<Target> result = new ArrayList<Target>();
      for(Target t : System.getTargets())
        if(t.isSelected())
          result.add(t);
      return result;
    }

    public int[] getSelectedPositions() {
      int[] res = new int[getSelectedCount()];
      int j=0;

      for(int i = 0; i < System.getTargets().size(); i++)
        if(System.getTarget(i).isSelected())
          res[j++]=i;
      return res;
    }

		class TargetHolder {
			ImageView itemImage;
			TextView itemTitle;
			TextView itemDescription;
		}
	}

	private class RadarReceiver extends ManagedReceiver {
		private IntentFilter mFilter = null;

		public RadarReceiver() {
			mFilter = new IntentFilter();

			mFilter.addAction(TARGET_UPDATE);
			mFilter.addAction(NRDR_STOPPED);
		}

		public IntentFilter getFilter() {
			return mFilter;
		}

		@SuppressWarnings("ConstantConditions")
		@Override
		public void onReceive(Context context, Intent intent) {
      if(intent.getAction() == null)
        return;

      if (intent.getAction().equals(TARGET_UPDATE)) {
        // refresh the target listview
        MainActivity.this.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            mTargetAdapter.notifyDataSetChanged();
          }
        });
      } else if (intent.getAction().equals(NRDR_STOPPED)) {

        Toast.makeText(MainActivity.this, R.string.net_discovery_stopped,
                Toast.LENGTH_SHORT).show();
      }
		}
	}

	private class WipeReceiver extends ManagedReceiver {
		private IntentFilter mFilter = null;

		public WipeReceiver() {
			mFilter = new IntentFilter();

      mFilter.addAction(SettingsActivity.SETTINGS_WIPE_START);
		}

		public IntentFilter getFilter() {
			return mFilter;
		}

		@Override
		public void onReceive(Context context, Intent intent) {

      if(intent.getAction().equals(SettingsActivity.SETTINGS_WIPE_START)) {
        try {
          String path;

          if(intent.hasExtra(SettingsActivity.SETTINGS_WIPE_DIR)) {
            path = intent.getStringExtra(SettingsActivity.SETTINGS_WIPE_DIR);
          } else {
            path = System.getRubyPath() + "' '" + System.getMsfPath();
          }

          StopRPCServer(true);
          System.getTools().raw.async("rm -rf '" + path + "'", new Child.EventReceiver() {
            @Override
            public void onEnd(int exitCode) {
              MainActivity.this.sendBroadcast(new Intent(SettingsActivity.SETTINGS_WIPE_DONE));
            }

            @Override
            public void onDeath(int signal) {
              MainActivity.this.sendBroadcast(new Intent(SettingsActivity.SETTINGS_WIPE_DONE));
            }

            @Override
            public void onEvent(Event e) { }
          });
        } catch ( Exception e) {
          System.errorLogging(e);
        }
      }
		}
	}

	private class UpdateReceiver extends ManagedReceiver {
		private IntentFilter mFilter = null;

		public UpdateReceiver() {
			mFilter = new IntentFilter();

			mFilter.addAction(UPDATE_CHECKING);
			mFilter.addAction(UPDATE_AVAILABLE);
			mFilter.addAction(UPDATE_NOT_AVAILABLE);
      mFilter.addAction(CORE_AVAILABLE);
      mFilter.addAction(RUBY_AVAILABLE);
      mFilter.addAction(GEMS_AVAILABLE);
      mFilter.addAction(MSF_AVAILABLE);
      mFilter.addAction(UpdateService.ERROR);
      mFilter.addAction(UpdateService.DONE);
		}

		public IntentFilter getFilter() {
			return mFilter;
		}

    private void onUpdateAvailable(final String desc, final UpdateService.action target, final boolean mandatory) {
      MainActivity.this.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          new ConfirmDialog(getString(R.string.update_available),
                desc, MainActivity.this, new ConfirmDialogListener() {
                  @Override
                  public void onConfirm() {
                    StopRPCServer(true);
                    Intent i = new Intent(MainActivity.this, UpdateService.class);
                    i.setAction(UpdateService.START);
                    i.putExtra(UpdateService.ACTION, target);

                    Logger.debug("starting UpdateService with: " + ((Context)MainActivity.this).toString());

                    startService(i);
                  }

                  @Override
                  public void onCancel() {
                    if(!mandatory) {
                      return;
                    }

                    onInitializationError(getString(R.string.mandatory_update));
                  }
                }
              ).show();
        }
      });
    }

    private void onUpdateAvailable(final String desc, final UpdateService.action target) {
      onUpdateAvailable(desc, target, false);
    }

    private void onUpdateDone(UpdateService.action target) {
      switch (target) {
        case core_update:
          System.shutdownCoreDaemon();
          try {
            System.initCore();
            onInitializationSuccess();
          } catch (System.SuException e) {
            onInitializationError(getString(R.string.only_4_root));
          } catch (System.DaemonException e) {
            onInitializationError(e.getMessage());
          }
          break;
        case ruby_update:
        case msf_update:
        case gems_update:
          StartRPCServer();
          break;
      }

      // restart update checker after a successful update
      startUpdateChecker();
    }

    private void onUpdateError(UpdateService.action target, final int message) {

      if(target == UpdateService.action.core_update && progressDialog != null) {
        onInitializationError(getString(message));
        return;
      }

      MainActivity.this.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          new ErrorDialog(getString(R.string.error),
                  getString(message),MainActivity.this).show();
        }
      });
    }

		@SuppressWarnings("ConstantConditions")
		@Override
		public void onReceive(Context context, Intent intent) {
      if (intent.getAction().equals(UPDATE_CHECKING)) {

        if(mUpdateStatus != null)
				  mUpdateStatus.setText(UPDATE_MESSAGE.replace(
					  	"#STATUS#", getString(R.string.checking)));

			} else if (intent.getAction().equals(UPDATE_NOT_AVAILABLE)) {

        if(mUpdateStatus != null)
				  mUpdateStatus.setText(UPDATE_MESSAGE.replace(
					  	"#STATUS#", getString(R.string.no_updates_available)));

        if(!System.isCoreInitialized()) {
          new FatalDialog(getString(R.string.initialization_error),
                  getString(R.string.no_core_found), MainActivity.this).show();
        }

      } else if (intent.getAction().equals(RUBY_AVAILABLE)) {
        final String description =  getString(R.string.new_ruby_update_desc) + " " +
                              getString(R.string.new_update_desc2);

        onUpdateAvailable(description, UpdateService.action.ruby_update);
      } else if (intent.getAction().equals(MSF_AVAILABLE)) {
        if (mUpdateStatus != null)
          mUpdateStatus.setText(UPDATE_MESSAGE.replace(
                  "#STATUS#",
                  getString(R.string.new_version) + " " +
                  getString(R.string.new_version2)
          ));

        final String description =  getString(R.string.new_msf_update_desc) + " " +
                              getString(R.string.new_update_desc2);

        onUpdateAvailable( description, UpdateService.action.msf_update);
      } else if ( intent.getAction().equals(GEMS_AVAILABLE)) {

        final String description = getString(R.string.new_gems_update_desc) + " " +
                                    getString(R.string.new_update_desc2);

        onUpdateAvailable(description, UpdateService.action.gems_update);
			} else if (intent.getAction().equals(UPDATE_AVAILABLE)) {
        final String remoteVersion = (String) intent.getExtras().get(
                AVAILABLE_VERSION);

        if (mUpdateStatus != null)
          mUpdateStatus.setText(UPDATE_MESSAGE.replace(
                  "#STATUS#", getString(R.string.new_version)
                          + remoteVersion
                          + getString(R.string.new_version2)));

        final String description = getString(R.string.new_update_desc)
                + " " + remoteVersion + " "
                + getString(R.string.new_update_desc2);

        onUpdateAvailable(description, UpdateService.action.apk_update);
      } else if(intent.getAction().equals(CORE_AVAILABLE)) {

        final String remoteVersion = (String) intent.getExtras().get(
                AVAILABLE_VERSION);

        if (mUpdateStatus != null)
          mUpdateStatus.setText(UPDATE_MESSAGE.replace(
                  "#STATUS#", getString(R.string.new_version) + " " +
                          getString(R.string.new_version2)));

        final String description = String.format(getString(R.string.new_core_found), remoteVersion);

        onUpdateAvailable(description, UpdateService.action.core_update, !System.isCoreInstalled());
			} else if(intent.getAction().equals(UpdateService.ERROR)) {
        onUpdateError((UpdateService.action)intent.getSerializableExtra(UpdateService.ACTION),
                intent.getIntExtra(UpdateService.MESSAGE, R.string.error_occured));
      } else if(intent.getAction().equals(UpdateService.DONE)) {
        onUpdateDone((UpdateService.action)intent.getSerializableExtra(UpdateService.ACTION));
      }
		}
	}
}
