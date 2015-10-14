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

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.view.ActionMode;
import android.text.Html;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import org.csploit.android.core.Child;
import org.csploit.android.core.Client;
import org.csploit.android.core.CrashReporter;
import org.csploit.android.core.Logger;
import org.csploit.android.core.ManagedReceiver;
import org.csploit.android.core.MultiAttackService;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.events.Event;
import org.csploit.android.gui.dialogs.AboutDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.FatalDialog;
import org.csploit.android.gui.dialogs.InputDialog;
import org.csploit.android.gui.dialogs.InputDialog.InputDialogListener;
import org.csploit.android.gui.dialogs.MultipleChoiceDialog;
import org.csploit.android.gui.dialogs.SpinnerDialog;
import org.csploit.android.gui.dialogs.SpinnerDialog.SpinnerDialogListener;
import org.csploit.android.helpers.ThreadHelper;
import org.csploit.android.net.Network;
import org.csploit.android.net.Target;
import org.csploit.android.plugins.ExploitFinder;
import org.csploit.android.plugins.Inspector;
import org.csploit.android.plugins.LoginCracker;
import org.csploit.android.plugins.PacketForger;
import org.csploit.android.plugins.PortScanner;
import org.csploit.android.plugins.RouterPwn;
import org.csploit.android.plugins.Sessions;
import org.csploit.android.plugins.Traceroute;
import org.csploit.android.plugins.mitm.MITM;
import org.csploit.android.services.Services;
import org.csploit.android.services.UpdateChecker;
import org.csploit.android.services.UpdateService;
import org.csploit.android.services.receivers.MsfRpcdServiceReceiver;
import org.csploit.android.services.receivers.NetworkRadarReceiver;
import org.csploit.android.update.CoreUpdate;
import org.csploit.android.update.MsfUpdate;
import org.csploit.android.update.RubyUpdate;
import org.csploit.android.update.Update;

import java.io.IOException;
import java.net.NoRouteToHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Observable;
import java.util.Observer;

import static org.csploit.android.services.UpdateChecker.UPDATE_AVAILABLE;
import static org.csploit.android.services.UpdateChecker.UPDATE_CHECKING;
import static org.csploit.android.services.UpdateChecker.UPDATE_NOT_AVAILABLE;

@SuppressLint("NewApi")
public class MainActivity extends AppCompatActivity {
  private String UPDATE_MESSAGE;
  private static final int WIFI_CONNECTION_REQUEST = 1012;
  private boolean isWifiAvailable = false;
  private TargetAdapter mTargetAdapter = null;
  private NetworkRadarReceiver mRadarReceiver = new NetworkRadarReceiver();
  private UpdateReceiver mUpdateReceiver = new UpdateReceiver();
  private WipeReceiver mWipeReceiver = new WipeReceiver();
  private MsfRpcdServiceReceiver mMsfReceiver = new MsfRpcdServiceReceiver();
  private Menu mMenu = null;
  private TextView mUpdateStatus = null;
  private Toast mToast = null;
  private long mLastBackPressTime = 0;
  private ActionMode mActionMode = null;
  private ListView lv;
  private boolean isRootMissing = false;

  private void createUpdateStatusText() {
    if (mUpdateStatus != null) return;

    RelativeLayout layout = (RelativeLayout) findViewById(R.id.layout);

    mUpdateStatus = new TextView(this);

    LayoutParams params = new LayoutParams(LayoutParams.MATCH_PARENT,
            LayoutParams.MATCH_PARENT);

    mUpdateStatus.setGravity(Gravity.CENTER);
    mUpdateStatus.setLayoutParams(params);

    layout.addView(mUpdateStatus);
  }


  private void createUpdateLayout() {

    lv.setVisibility(View.GONE);
    findViewById(R.id.textView).setVisibility(View.GONE);

    createUpdateStatusText();

    mUpdateStatus
            .setText(UPDATE_MESSAGE.replace("#STATUS#", "..."));

    mUpdateReceiver.register(MainActivity.this);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
      invalidateOptionsMenu();
  }

  private void createOfflineLayout() {

    lv.setVisibility(View.GONE);
    findViewById(R.id.textView).setVisibility(View.GONE);

    createUpdateStatusText();

    mUpdateStatus.setText(getString(R.string.no_connectivity));

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
      invalidateOptionsMenu();
  }

  public void createOnlineLayout() {
    findViewById(R.id.textView).setVisibility(View.VISIBLE);
    lv.setVisibility(View.VISIBLE);

    if (mUpdateStatus != null)
      mUpdateStatus.setVisibility(View.GONE);

    if (mTargetAdapter != null) {
      mTargetAdapter.notifyDataSetChanged();
      return;
    }

    mTargetAdapter = new TargetAdapter();

    lv.setAdapter(mTargetAdapter);

    lv.setOnItemLongClickListener(new OnItemLongClickListener() {
      @Override
      public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        Target t = (Target) mTargetAdapter.getItem(position);
        if (t.getType() == Target.Type.NETWORK) {
          if (mActionMode == null)
            targetAliasPrompt(t);
          return true;
        }
        if (mActionMode == null) {
          mTargetAdapter.clearSelection();
          mActionMode = startSupportActionMode(mActionModeCallback);
        }
        mTargetAdapter.toggleSelection(position);
        return true;
      }
    });

    mRadarReceiver.register(MainActivity.this);
    mUpdateReceiver.register(MainActivity.this);
    mWipeReceiver.register(MainActivity.this);
    mMsfReceiver.register(MainActivity.this);

    System.setTargetListObserver(mTargetAdapter);

    StartRPCServer();

    // if called for the second time after wifi connection
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
      invalidateOptionsMenu();
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode,
                                  Intent intent) {
    if (requestCode == WIFI_CONNECTION_REQUEST && resultCode == RESULT_OK
            && intent.hasExtra(WifiScannerActivity.CONNECTED)) {
      System.reloadNetworkMapping();
      if(mTargetAdapter != null) {
        mTargetAdapter.notifyDataSetChanged();
      }
      try {
        onCreate(null);
      } catch (IllegalStateException e) {
        // already attached.  don't reattach.
      }
    }
  }

  private void createLayout() {
    boolean wifiAvailable = Network.isWifiConnected(this);
    boolean connectivityAvailable = wifiAvailable || Network.isConnectivityAvailable(this);
    boolean coreBeating = System.isCoreInitialized();

    if (coreBeating && wifiAvailable) {
      createOnlineLayout();
    } else if (connectivityAvailable) {
      createUpdateLayout();
    } else {
      createOfflineLayout();
    }
  }

  private void onInitializationError(final String message) {
    MainActivity.this.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        new FatalDialog(getString(R.string.initialization_error),
                message, message.contains(">"),
                MainActivity.this).show();
      }
    });
  }

  private boolean startCore() {
    isRootMissing = false;
    try {
      System.initCore();

      return true;
    } catch (System.SuException e) {
      onInitializationError(getString(R.string.only_4_root));
      isRootMissing = true;
    } catch (System.DaemonException e) {
      Logger.error(e.getMessage());
    }

    return false;
  }

  private void onCoreBeating() {
    if (Client.hadCrashed()) {
      Logger.warning("Client has previously crashed, building a crash report.");
      CrashReporter.notifyNativeLibraryCrash();
      onInitializationError(getString(R.string.JNI_crash_detected));
    }
  }

  private void onCoreUpdated() {
    if (startCore()) {
      onCoreBeating();
    } else if (isRootMissing) {
      return;
    }

    MainActivity.this.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        System.reloadNetworkMapping();
        createLayout();
        if (System.isInitialized())
          startNetworkRadar();
      }
    });
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
    Boolean isDark = themePrefs.getBoolean("isDark", false);
    boolean connectivityAvailable;

    if (isDark)
      setTheme(R.style.DarkTheme);
    else
      setTheme(R.style.AppTheme);
    super.onCreate(savedInstanceState);
    setContentView(R.layout.target_layout);

    lv = (ListView) findViewById(R.id.android_list);
    lv.setOnItemClickListener(new ListView.OnItemClickListener() {

      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id) {

        if (mActionMode != null) {
          ((TargetAdapter) lv.getAdapter()).toggleSelection(position);
          return;
        }

        Target target = (Target) mTargetAdapter.getItem(position);
        System.setCurrentTarget(target);

        new Thread(new Runnable() {
          @Override
          public void run() {

            startActivityForResult(new Intent(MainActivity.this,
                    ActionActivity.class), WIFI_CONNECTION_REQUEST);

            overridePendingTransition(R.anim.fadeout, R.anim.fadein);

          }
        }).start();

        Toast.makeText(MainActivity.this,
                getString(R.string.selected_) + System.getCurrentTarget(),
                Toast.LENGTH_SHORT).show();

      }
    });

    isWifiAvailable = Network.isWifiConnected(this);
    connectivityAvailable = isWifiAvailable || Network.isConnectivityAvailable(this);

    // make sure system object was correctly initialized during application
    // startup
    if (!System.isInitialized()) {
      // wifi available but system failed to initialize, this is a fatal
      // :(
      if (isWifiAvailable) {

        // retry
        try {
          System.init(MainActivity.this.getApplicationContext());

          System.registerPlugin(new RouterPwn());
          System.registerPlugin(new Traceroute());
          System.registerPlugin(new PortScanner());
          System.registerPlugin(new Inspector());
          System.registerPlugin(new ExploitFinder());
          System.registerPlugin(new LoginCracker());
          System.registerPlugin(new Sessions());
          System.registerPlugin(new MITM());
          System.registerPlugin(new PacketForger());
        } catch (Exception e) {
          if (!(e instanceof NoRouteToHostException))
            System.errorLogging(e);

          onInitializationError(System.getLastError());

          return;
        }
      }
    } else {
      System.reloadNetworkMapping();
    }

    boolean coreInstalled = System.isCoreInstalled();
    boolean coreBeating = System.isCoreInitialized();

    if (coreInstalled && !coreBeating) {
      coreBeating = startCore();
      if (coreBeating) {
        onCoreBeating();
      } else if (isRootMissing) {
        return;
      }
    }

    if (!connectivityAvailable) {
      if (!coreInstalled) {
        onInitializationError(getString(R.string.no_core_no_connectivity));
        return;
      } else if (!coreBeating) {
        onInitializationError(getString(R.string.heart_attack));
        return;
      }
    }

    if (!coreInstalled) {
      UPDATE_MESSAGE = getString(R.string.missing_core_update);
    } else if (!coreBeating) {
      UPDATE_MESSAGE = getString(R.string.heart_attack_update);
    } else if (!isWifiAvailable) {
      UPDATE_MESSAGE = getString(R.string.no_wifi_available);
    }

    if (connectivityAvailable)
      startUpdateChecker();

    if (coreBeating && isWifiAvailable)
      startNetworkRadar();

    createLayout();
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    MenuInflater inflater = getMenuInflater();
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

    Services.getNetworkRadar().buildMenuItem(item);

    item = menu.findItem(R.id.ss_msfrpcd);

    Services.getMsfRpcdService().buildMenuItem(item);

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
      mode.setTitle(i + " " + getString((i > 1 ? R.string.targets_selected : R.string.target_selected)));
      MenuItem item = menu.findItem(R.id.multi_action);
      if (item != null)
        item.setIcon((i > 1 ? android.R.drawable.ic_dialog_dialer : android.R.drawable.ic_menu_edit));
      return false;
    }

    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
      ArrayList<Plugin> commonPlugins = null;

      switch (item.getItemId()) {
        case R.id.multi_action:
          final int[] selected = mTargetAdapter.getSelectedPositions();
          if (selected.length > 1) {
            Target target = (Target) mTargetAdapter.getItem(selected[0]);
            commonPlugins = System.getPluginsForTarget(target);
            for (int i = 1; i < selected.length; i++) {
              target = (Target) mTargetAdapter.getItem(selected[i]);
              ArrayList<Plugin> targetPlugins = System.getPluginsForTarget(target);
              ArrayList<Plugin> removeThem = new ArrayList<Plugin>();
              for (Plugin p : commonPlugins) {
                if (!targetPlugins.contains(p))
                  removeThem.add(p);
              }
              for (Plugin p : removeThem) {
                commonPlugins.remove(p);
              }
            }
            if (commonPlugins.size() > 0) {
              final int[] actions = new int[commonPlugins.size()];
              for (int i = 0; i < actions.length; i++)
                actions[i] = commonPlugins.get(i).getName();

              (new MultipleChoiceDialog(R.string.choose_method, actions, MainActivity.this, new MultipleChoiceDialog.MultipleChoiceDialogListener() {
                @Override
                public void onChoice(int[] choices) {
                  Intent intent = new Intent(MainActivity.this, MultiAttackService.class);
                  int[] selectedActions = new int[choices.length];

                  for (int i = 0; i < selectedActions.length; i++)
                    selectedActions[i] = actions[choices[i]];

                  intent.putExtra(MultiAttackService.MULTI_TARGETS, selected);
                  intent.putExtra(MultiAttackService.MULTI_ACTIONS, selectedActions);

                  startService(intent);
                }
              })).show();
            } else {
              (new ErrorDialog(getString(R.string.error), "no common actions found", MainActivity.this)).show();
            }
          } else {
            targetAliasPrompt((Target) mTargetAdapter.getItem(selected[0]));
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
      new UpdateChecker(this).start();
    } else {
      MainActivity.this.sendBroadcast(new Intent(UPDATE_NOT_AVAILABLE));
    }
  }

  public void startNetworkRadar() {
    new Thread(new Runnable() {
      @Override
      public void run() {
        Services.getNetworkRadar().start();
      }
    }).start();
  }

  public void stopNetworkRadar() {
    new Thread(new Runnable() {
      @Override
      public void run() {
        Services.getNetworkRadar().stop();
      }
    }).start();
  }

  /**
   * start MSF RPC Daemon
   */
  public void StartRPCServer() {
    new Thread(new Runnable() {
      @Override
      public void run() {
        if(Services.getMsfRpcdService().isAvailable())
          Services.getMsfRpcdService().start();
      }
    }).start();
  }

  /**
   * stop MSF RPC Daemon
   */
  public void StopRPCServer() {
    new Thread(new Runnable() {
      @Override
      public void run() {
        Services.getMsfRpcdService().stop();
      }
    }).start();
  }

  @Override
  public boolean onOptionsItemSelected(final MenuItem item) {
    switch (item.getItemId()) {

      case R.id.add:
        new InputDialog(getString(R.string.add_custom_target),
                getString(R.string.enter_url), MainActivity.this,
                new InputDialogListener() {
                  @Override
                  public void onInputEntered(String input) {
                    final Target target = Target.getFromString(input);
                    if (target != null) {
                      ThreadHelper.getSharedExecutor().execute(new Runnable() {
                        @Override
                        public void run() {
                          System.addOrderedTarget(target);
                          mTargetAdapter.update(null, null);
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
        startNetworkRadar();
        return true;

      case R.id.wifi_scan:
        stopNetworkRadar();

        mRadarReceiver.unregister();
        mUpdateReceiver.unregister();

        startActivityForResult(new Intent(MainActivity.this,
                WifiScannerActivity.class), WIFI_CONNECTION_REQUEST);
        overridePendingTransition(R.anim.fadeout, R.anim.fadein);
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
        overridePendingTransition(R.anim.fadeout, R.anim.fadein);
        return true;

      case R.id.ss_monitor:
        new Thread(new Runnable() {
          @Override
          public void run() {
            Services.getNetworkRadar().onMenuClick(MainActivity.this, item);
          }
        }).start();
        return true;

      case R.id.ss_msfrpcd:
        new Thread(new Runnable() {
          @Override
          public void run() {
            Services.getMsfRpcdService().onMenuClick(MainActivity.this, item);
          }
        }).start();
        return true;

      case R.id.submit_issue:
        String uri = getString(R.string.github_new_issue_url);
        Intent browser = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
        startActivity(browser);
        // for fat-tire:
        //   String.format(getString(R.string.issue_message), getString(R.string.github_issues_url), getString(R.string.github_new_issue_url));
        return true;

      case R.id.about:
        new AboutDialog(this).show();
        return true;

      default:
        return super.onOptionsItemSelected(item);
    }
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
    stopNetworkRadar();
    StopRPCServer();

    mRadarReceiver.unregister();
    mUpdateReceiver.unregister();
    mWipeReceiver.unregister();
    mMsfReceiver.unregister();

    // make sure no zombie process is running before destroying the activity
    System.clean(true);

    super.onDestroy();
  }

  public class TargetAdapter extends BaseAdapter implements Runnable, Observer {

    private List<Target> list = System.getTargets();
    private boolean isDark = getSharedPreferences("THEME", 0).getBoolean("isDark", false);

    @Override
    public int getCount() {
      return list.size();
    }

    @Override
    public Object getItem(int position) {
      return list.get(position);
    }

    @Override
    public long getItemId(int position) {
      return R.layout.target_list_item;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
      View row = convertView;
      TargetHolder holder;

      if (row == null) {
        LayoutInflater inflater = (LayoutInflater) MainActivity.this
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        row = inflater.inflate(R.layout.target_list_item, parent, false);

        if (isDark)
          row.setBackgroundResource(R.drawable.card_background_dark);

        holder = new TargetHolder();
        holder.itemImage = (ImageView) (row != null ? row
                .findViewById(R.id.itemIcon) : null);
        holder.itemTitle = (TextView) (row != null ? row
                .findViewById(R.id.itemTitle) : null);
        holder.itemDescription = (TextView) (row != null ? row
                .findViewById(R.id.itemDescription) : null);
        holder.portCount = (TextView) (row != null ? row
                .findViewById(R.id.portCount) : null);
        holder.portCountLayout = (LinearLayout) (row != null ? row
                .findViewById(R.id.portCountLayout) : null);
        if (isDark)
            holder.portCountLayout.setBackgroundResource(R.drawable.rounded_square_grey);
        if (row != null)
          row.setTag(holder);
      } else
        holder = (TargetHolder) row.getTag();

      final Target target = list.get(position);

      if (target.hasAlias()){
        holder.itemTitle.setText(Html.fromHtml("<b>"
                + target.getAlias() + "</b> <small>( "
                + target.getDisplayAddress() + " )</small>"));
      } else {
        holder.itemTitle.setText(target.toString());
      }
      holder.itemTitle.setTextColor(ContextCompat.getColor(getApplicationContext(), (target.isConnected() ? R.color.app_color : R.color.gray_text)));

      holder.itemTitle.setTypeface(null, Typeface.NORMAL);
      holder.itemImage.setImageResource(target.getDrawableResourceId());
      holder.itemDescription.setText(target.getDescription());

      int openedPorts = target.getOpenPorts().size();

      holder.portCount.setText(String.format("%d", openedPorts));
      holder.portCountLayout.setVisibility(openedPorts < 1 ? View.GONE : View.VISIBLE);
      return row;
    }

    public void clearSelection() {
      for (Target t : list)
        t.setSelected(false);
      notifyDataSetChanged();
      if (mActionMode != null)
        mActionMode.finish();
    }

    public void toggleSelection(int position) {
      Target t = list.get(position);
      t.setSelected(!t.isSelected());
      notifyDataSetChanged();
      if (mActionMode != null) {
        if (getSelectedCount() > 0)
          mActionMode.invalidate();
        else
          mActionMode.finish();
      }
    }

    public int getSelectedCount() {
      int i = 0;
      for (Target t : list)
        if (t.isSelected())
          i++;
      return i;
    }

    public ArrayList<Target> getSelected() {
      ArrayList<Target> result = new ArrayList<Target>();
      for (Target t : list)
        if (t.isSelected())
          result.add(t);
      return result;
    }

    public int[] getSelectedPositions() {
      int[] res = new int[getSelectedCount()];
      int j = 0;

      for (int i = 0; i < list.size(); i++)
        if (list.get(i).isSelected())
          res[j++] = i;
      return res;
    }

    @Override
    public void update(Observable observable, Object data) {
      final Target target = (Target) data;

      if(target == null) {
        // update the whole list
        MainActivity.this.runOnUiThread(this);
        return;
      }

      // update only a row, if it's displayed
      MainActivity.this.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          if(lv == null)
            return;
          int start = lv.getFirstVisiblePosition();
          for(int i=start, j=lv.getLastVisiblePosition();i<=j;i++)
            if(target==list.get(i)){
              View view = lv.getChildAt(i-start);
              getView(i, view, lv);
              break;
            }
        }
      });

    }

    @Override
    public void run() {
      list = System.getTargets();
      notifyDataSetChanged();
    }

    class TargetHolder {
      ImageView itemImage;
      TextView itemTitle;
      TextView itemDescription;
      TextView portCount;
      LinearLayout portCountLayout;
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

      if (intent.getAction().equals(SettingsActivity.SETTINGS_WIPE_START)) {
        try {
          String path;

          if (intent.hasExtra(SettingsActivity.SETTINGS_WIPE_DIR)) {
            path = intent.getStringExtra(SettingsActivity.SETTINGS_WIPE_DIR);
          } else {
            path = System.getRubyPath() + "' '" + System.getMsfPath();
          }

          StopRPCServer();
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
            public void onEvent(Event e) {
            }
          });
        } catch (Exception e) {
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
      mFilter.addAction(UpdateService.ERROR);
      mFilter.addAction(UpdateService.DONE);
    }

    public IntentFilter getFilter() {
      return mFilter;
    }

    private void onUpdateAvailable(final Update update, final boolean mandatory) {
      MainActivity.this.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          new ConfirmDialog(getString(R.string.update_available),
                  update.prompt, MainActivity.this, new ConfirmDialogListener() {
            @Override
            public void onConfirm() {
              StopRPCServer();
              Intent i = new Intent(MainActivity.this, UpdateService.class);
              i.setAction(UpdateService.START);
              i.putExtra(UpdateService.UPDATE, update);

              startService(i);
            }

            @Override
            public void onCancel() {
              if (!mandatory) {
                return;
              }

              onInitializationError(getString(R.string.mandatory_update));
            }
          }
          ).show();
        }
      });
    }

    private void onUpdateAvailable(Update update) {
      onUpdateAvailable(update, (update instanceof CoreUpdate) && !System.isCoreInstalled());
    }

    private void onUpdateDone(Update update) {

      System.reloadTools();

      if((update instanceof MsfUpdate) || (update instanceof RubyUpdate)) {
        StartRPCServer();
      }

      if(update instanceof CoreUpdate) {
        onCoreUpdated();
      }

      // restart update checker after a successful update
      startUpdateChecker();
    }

    private void onUpdateError(final Update update, final int message) {

      if (update instanceof CoreUpdate) {
        onInitializationError(getString(message));
        return;
      }

      MainActivity.this.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          new ErrorDialog(getString(R.string.error),
                  getString(message), MainActivity.this).show();
        }
      });

      System.reloadTools();
    }

    @SuppressWarnings("ConstantConditions")
    @Override
    public void onReceive(Context context, Intent intent) {
      String action = intent.getAction();
      Update update = null;

      if(intent.hasExtra(UpdateService.UPDATE)) {
        update = (Update) intent.getSerializableExtra(UpdateService.UPDATE);
      }

      switch (action) {
        case UPDATE_CHECKING:
          if (mUpdateStatus != null)
            mUpdateStatus.setText(UPDATE_MESSAGE.replace(
                    "#STATUS#", getString(R.string.checking)));
          break;
        case UPDATE_NOT_AVAILABLE:
          if (mUpdateStatus != null)
            mUpdateStatus.setText(UPDATE_MESSAGE.replace(
                    "#STATUS#", getString(R.string.no_updates_available)));

          if (!System.isCoreInstalled()) {
            onInitializationError(getString(R.string.no_core_found));
          }
          break;
        case UPDATE_AVAILABLE:
          onUpdateAvailable(update);
          break;
        case UpdateService.DONE:
          onUpdateDone(update);
          break;
        case UpdateService.ERROR:
          int message = intent.getIntExtra(UpdateService.MESSAGE, R.string.error_occured);
          onUpdateError(update, message);
          break;
      }
    }
  }
}