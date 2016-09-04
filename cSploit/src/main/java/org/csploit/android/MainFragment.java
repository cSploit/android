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
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.view.ActionMode;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
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
import org.csploit.android.gui.dialogs.ChoiceDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.FatalDialog;
import org.csploit.android.gui.dialogs.InputDialog;
import org.csploit.android.gui.dialogs.InputDialog.InputDialogListener;
import org.csploit.android.gui.dialogs.ListChoiceDialog;
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
import java.util.Timer;
import java.util.TimerTask;

import static org.csploit.android.services.UpdateChecker.UPDATE_AVAILABLE;
import static org.csploit.android.services.UpdateChecker.UPDATE_CHECKING;
import static org.csploit.android.services.UpdateChecker.UPDATE_NOT_AVAILABLE;

@SuppressLint("NewApi")
public class MainFragment extends Fragment {
    private String EMPTY_LIST_MESSAGE = "";
    private static final int WIFI_CONNECTION_REQUEST = 1012;
    private boolean isAnyNetInterfaceAvailable = false;
    private TargetAdapter mTargetAdapter = null;
    private NetworkRadarReceiver mRadarReceiver = new NetworkRadarReceiver();
    private UpdateReceiver mUpdateReceiver = new UpdateReceiver();
    private WipeReceiver mWipeReceiver = new WipeReceiver();
    private MsfRpcdServiceReceiver mMsfReceiver = new MsfRpcdServiceReceiver();
    private ConnectivityReceiver mConnectivityReceiver = new ConnectivityReceiver();
    private Menu mMenu = null;
    private TextView mEmptyTextView = null;
    private Toast mToast = null;
    private TextView mTextView = null;
    private long mLastBackPressTime = 0;
    private ActionMode mActionMode = null;
    private ListView lv;
    private boolean isRootMissing = false;
    private String[] mIfaces = null;
    private boolean mIsCoreInstalled = false;
    private boolean mIsDaemonBeating = false;
    private boolean mIsConnectivityAvailable = false;
    private boolean mIsUpdateDownloading = false;
    private boolean mHaveAnyWifiInterface = true; // TODO: check is device have a wifi interface
    private boolean mOfflineMode = false;

    @Override
    public void onActivityResult(int requestCode, int resultCode,
                                 Intent intent) {
        if (requestCode == WIFI_CONNECTION_REQUEST && resultCode == AppCompatActivity.RESULT_OK
                && intent.hasExtra(WifiScannerFragment.CONNECTED)) {
            init();
        }
    }

    private void onInitializationError(final String message) {
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                new FatalDialog(getString(R.string.initialization_error),
                        message, message.contains(">"),
                        getActivity()).show();
            }
        });
    }

    private void onCoreUpdated() {
        System.onCoreInstalled();
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                init();
                startAllServices();
                notifyMenuChanged();
            }
        });
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        return inflater.inflate(R.layout.target_layout, container, false);
    }

    @Override
    public void onViewCreated(View v, Bundle savedInstanceState) {
        SharedPreferences themePrefs = getActivity().getSharedPreferences("THEME", 0);
        Boolean isDark = themePrefs.getBoolean("isDark", false);
        if (isDark) {
            getActivity().setTheme(R.style.DarkTheme);
            v.setBackgroundColor(ContextCompat.getColor(getActivity(), R.color.background_window_dark));
        } else {
            getActivity().setTheme(R.style.AppTheme);
            v.setBackgroundColor(ContextCompat.getColor(getActivity(), R.color.background_window));
        }
        mEmptyTextView = (TextView) v.findViewById(R.id.emptyTextView);
        lv = (ListView) v.findViewById(R.id.android_list);
        mTextView = (TextView) v.findViewById(R.id.textView);

        lv.setOnItemClickListener(new ListView.OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {

                if (mActionMode != null) {
                    mTargetAdapter.toggleSelection(position);
                    return;
                }

                Target target = (Target) mTargetAdapter.getItem(position);
                System.setCurrentTarget(target);

                ThreadHelper.getSharedExecutor().execute(new Runnable() {
                    @Override
                    public void run() {

                        startActivityForResult(new Intent(getActivity(),
                                ActionActivity.class), WIFI_CONNECTION_REQUEST);

                        getActivity().overridePendingTransition(R.anim.fadeout, R.anim.fadein);
                    }
                });

                Toast.makeText(getActivity(),
                        getString(R.string.selected_) + System.getCurrentTarget(),
                        Toast.LENGTH_SHORT).show();

            }
        });
        lv.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
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
                    mActionMode = ((AppCompatActivity) getActivity()).startSupportActionMode(mActionModeCallback);
                }
                mTargetAdapter.toggleSelection(position);
                return true;
            }
        });
        mTargetAdapter = new TargetAdapter();

        lv.setEmptyView(v.findViewById(android.R.id.empty));
        lv.setAdapter(mTargetAdapter);

        System.setTargetListObserver(mTargetAdapter);

        mRadarReceiver.register(getActivity());
        mUpdateReceiver.register(getActivity());
        mWipeReceiver.register(getActivity());
        mMsfReceiver.register(getActivity());
        mConnectivityReceiver.register(getActivity());

        init();
        startAllServices();
    }

    private void startAllServices() {
        startNetworkRadar();
        startUpdateChecker();
        startRPCServer();
    }

    private void notifyMenuChanged() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
            getActivity().invalidateOptionsMenu();
        else
            configureMenu();
    }

    /**
     * Performs the firsts actions when the app starts.
     * called also when the user connects to a wifi from the app, and when the core is updated.
     */
    public void init() {
        loadInterfaces();
        isAnyNetInterfaceAvailable = (mIfaces.length > 0);
        mIsConnectivityAvailable = isConnectivityAvailable();

        mIsCoreInstalled = System.isCoreInstalled();
        mIsDaemonBeating = System.isCoreInitialized();

        // check minimum requirements for system initialization

        if (!mIsCoreInstalled) {
            EMPTY_LIST_MESSAGE = mIsConnectivityAvailable ?
                    getString(R.string.missing_core_update) :
                    getString(R.string.no_connectivity);
            return;
        } else if (!mIsDaemonBeating) {
            try {
                System.initCore();
                mIsDaemonBeating = true;

                if (Client.hadCrashed()) {
                    Logger.warning("Client has previously crashed, building a crash report.");
                    CrashReporter.notifyNativeLibraryCrash();
                    onInitializationError(getString(R.string.JNI_crash_detected));
                    return;
                }
            } catch (UnsatisfiedLinkError e) {
                onInitializationError("hi developer, you missed to build JNI stuff, thanks for playing with me :)");
                return;
            } catch (System.SuException e) {
                onInitializationError(getString(R.string.only_4_root));
                return;
            } catch (System.DaemonException e) {
                Logger.error(e.getMessage());
            }

            if (!mIsDaemonBeating) {
                if (mIsConnectivityAvailable) {
                    EMPTY_LIST_MESSAGE = getString(R.string.heart_attack_update);
                } else {
                    onInitializationError(getString(R.string.heart_attack));
                }
                return;
            }
        }

        // if all is initialized, configure the network
        initSystem();
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        inflater.inflate(R.menu.main, menu);

        mMenu = menu;
        configureMenu();
        super.onCreateOptionsMenu(menu, inflater);
        getActivity().onCreateOptionsMenu(menu);
    }

    private boolean isConnectivityAvailable() {
        return Network.isConnectivityAvailable(getActivity()) || Network.isWifiConnected(getActivity());
    }

    public void configureMenu() {
        if (mMenu == null)
            return;
        mMenu.findItem(R.id.add).setVisible(isAnyNetInterfaceAvailable);
        mMenu.findItem(R.id.scan).setVisible(mHaveAnyWifiInterface);
        mMenu.findItem(R.id.wifi_ifaces).setEnabled(canChangeInterface());
        mMenu.findItem(R.id.new_session).setEnabled(isAnyNetInterfaceAvailable);
        mMenu.findItem(R.id.save_session).setEnabled(isAnyNetInterfaceAvailable);
        mMenu.findItem(R.id.restore_session).setEnabled(isAnyNetInterfaceAvailable);
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);
        MenuItem item = menu.findItem(R.id.ss_monitor);

        Services.getNetworkRadar().buildMenuItem(item);

        item = menu.findItem(R.id.ss_msfrpcd);

        Services.getMsfRpcdService().buildMenuItem(item);

        mMenu = menu;
        getActivity().onPrepareOptionsMenu(menu);
    }

    private boolean initSystem() {
        // retry
        try {
            System.init(getActivity().getApplicationContext());
        } catch (Exception e) {
            boolean isFatal = !(e instanceof NoRouteToHostException);

            if (isFatal) {
                System.errorLogging(e);
                onInitializationError(System.getLastError());
            }

            return !isFatal;
        }

        registerPlugins();
        return true;
    }

    private void registerPlugins() {
        if (!System.getPlugins().isEmpty())
            return;

        System.registerPlugin(new RouterPwn());
        System.registerPlugin(new Traceroute());
        System.registerPlugin(new PortScanner());
        System.registerPlugin(new Inspector());
        System.registerPlugin(new ExploitFinder());
        System.registerPlugin(new LoginCracker());
        System.registerPlugin(new Sessions());
        System.registerPlugin(new MITM());
        System.registerPlugin(new PacketForger());
    }

    private void loadInterfaces() {
        boolean menuChanged;
        List<String> interfaces = Network.getAvailableInterfaces();
        int size = interfaces.size();

        if (mIfaces != null) {
            menuChanged = mIfaces.length != size;
            menuChanged &= mIfaces.length <= 1 || size <= 1;
        } else {
            menuChanged = true;
        }

        mIfaces = new String[size];
        interfaces.toArray(mIfaces);
        isAnyNetInterfaceAvailable = mIfaces.length > 0;

        if (menuChanged) {
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    notifyMenuChanged();
                }
            });
        }
    }

    private boolean canChangeInterface() {
        return mIfaces.length > 1 || (mOfflineMode && isAnyNetInterfaceAvailable);
    }

    private boolean haveInterface(String ifname) {
        for (String s : mIfaces) {
            if (s.equals(ifname))
                return true;
        }
        return false;
    }

    private void onNetworkInterfaceChanged() {
        String toastMessage = null;

        stopNetworkRadar();

        if (!System.reloadNetworkMapping()) {
            String ifname = System.getIfname();

            ifname = ifname == null ? getString(R.string.any_interface) : ifname;

            toastMessage = String.format(getString(R.string.error_initializing_interface), ifname);
        } else {
            startNetworkRadar();
            registerPlugins();
        }

        final String msg = toastMessage;

        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (msg != null) {
                    Toast.makeText(getActivity(), msg, Toast.LENGTH_LONG).show();
                }
                notifyMenuChanged();
            }
        });
    }

    private void onConnectionLost() {
        if (mOfflineMode)
            return;

        mOfflineMode = true;

        stopNetworkRadar();
        System.markNetworkAsDisconnected();

        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                new ConfirmDialog(getString(R.string.connection_lost),
                        getString(R.string.connection_lost_prompt), getActivity(),
                        new ConfirmDialogListener() {
                            @Override
                            public void onConfirm() {
                                mOfflineMode = false;
                                System.setIfname(null);
                                onNetworkInterfaceChanged();
                            }

                            @Override
                            public void onCancel() {
                            }
                        }).show();
            }
        });
    }

    private void onConnectionResumed() {
        if (!mOfflineMode)
            return;
        mOfflineMode = false;
        System.markInitialNetworkTargetsAsConnected();
        startNetworkRadar();
    }

    /**
     * Displays a dialog for choose a network interface
     *
     * @param forceDialog forces to show the dialog even if there's only one interface
     */
    private void displayNetworkInterfaces(final boolean forceDialog) {
        // reload the interfaces list if we've invoked the dialog from the menu
        loadInterfaces();
        boolean autoload = !forceDialog && mIfaces.length == 1;

        if (autoload) {
            System.setIfname(mIfaces[0]);
            onNetworkInterfaceChanged();
        } else if (isAnyNetInterfaceAvailable) {
            String title = getString(R.string.iface_dialog_title);

            new ListChoiceDialog(title, mIfaces, getActivity(), new ChoiceDialog.ChoiceDialogListener() {
                @Override
                public void onChoice(int index) {
                    System.setIfname(mIfaces[index]);
                    onNetworkInterfaceChanged();
                }
            }).show();
        } else {
            new ErrorDialog(getString(android.R.string.dialog_alert_title),
                    getString(R.string.iface_error_no_available), getActivity()).show();
        }
    }

    //FIXME: This method is never called. Is this a bug?
    private void displayNetworkInterfaces() {
        displayNetworkInterfaces(false);
    }

    private void targetAliasPrompt(final Target target) {

        new InputDialog(getString(R.string.target_alias),
                getString(R.string.set_alias),
                target.hasAlias() ? target.getAlias() : "", true,
                false, getActivity(), new InputDialogListener() {
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

                            (new MultipleChoiceDialog(R.string.choose_method, actions, getActivity(), new MultipleChoiceDialog.MultipleChoiceDialogListener() {
                                @Override
                                public void onChoice(int[] choices) {
                                    Intent intent = new Intent(getActivity(), MultiAttackService.class);
                                    int[] selectedActions = new int[choices.length];

                                    for (int i = 0; i < selectedActions.length; i++)
                                        selectedActions[i] = actions[choices[i]];

                                    intent.putExtra(MultiAttackService.MULTI_TARGETS, selected);
                                    intent.putExtra(MultiAttackService.MULTI_ACTIONS, selectedActions);

                                    getActivity().startService(intent);
                                }
                            })).show();
                        } else {
                            (new ErrorDialog(getString(R.string.error), "no common actions found", getActivity())).show();
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
        if (!isConnectivityAvailable() || mIsUpdateDownloading)
            return;
        if (System.getSettings().getBoolean("PREF_CHECK_UPDATES", true)) {
            new UpdateChecker(getActivity()).start();
            mIsUpdateDownloading = true;
        } else {
            getActivity().sendBroadcast(new Intent(UPDATE_NOT_AVAILABLE));
            mIsUpdateDownloading = false;
        }
    }

    public void startNetworkRadar() {
        if (!isAnyNetInterfaceAvailable || !mIsDaemonBeating) {
            return;
        }
        ThreadHelper.getSharedExecutor().execute(new Runnable() {
            @Override
            public void run() {
                Services.getNetworkRadar().start();
            }
        });
    }

    public void stopNetworkRadar() {
        ThreadHelper.getSharedExecutor().execute(new Runnable() {
            @Override
            public void run() {
                Services.getNetworkRadar().stop();
            }
        });
    }

    /**
     * start MSF RPC Daemon
     */
    public void startRPCServer() {
        ThreadHelper.getSharedExecutor().execute(new Runnable() {
            @Override
            public void run() {
                if (Services.getMsfRpcdService().isAvailable())
                    Services.getMsfRpcdService().start();
            }
        });
    }

    /**
     * stop MSF RPC Daemon
     */
    public void StopRPCServer() {
        ThreadHelper.getSharedExecutor().execute(new Runnable() {
            @Override
            public void run() {
                Services.getMsfRpcdService().stop();
            }
        });
    }

    @Override
    public boolean onOptionsItemSelected(final MenuItem item) {
        switch (item.getItemId()) {

            case R.id.add:
                new InputDialog(getString(R.string.add_custom_target),
                        getString(R.string.enter_url), getActivity(),
                        new InputDialogListener() {
                            @Override
                            public void onInputEntered(String input) {
                                final Target target = Target.getFromString(input);
                                if (target != null) {
                                    ThreadHelper.getSharedExecutor().execute(new Runnable() {
                                        @Override
                                        public void run() {
                                            System.addOrderedTarget(target);
                                        }
                                    });
                                } else
                                    new ErrorDialog(getString(R.string.error),
                                            getString(R.string.invalid_target),
                                            getActivity()).show();
                            }
                        }).show();
                return true;

            case R.id.scan:
                startNetworkRadar();
                return true;

            case R.id.wifi_ifaces:
                displayNetworkInterfaces(true);
                return true;

            case R.id.wifi_scan:
                stopNetworkRadar();

                mRadarReceiver.unregister();
                mUpdateReceiver.unregister();

                startActivityForResult(new Intent(getActivity(),
                        WifiScannerActivity.class), WIFI_CONNECTION_REQUEST);
                getActivity().overridePendingTransition(R.anim.fadeout, R.anim.fadein);
                return true;

            case R.id.new_session:
                new ConfirmDialog(getString(R.string.warning),
                        getString(R.string.warning_new_session), getActivity(),
                        new ConfirmDialogListener() {
                            @Override
                            public void onConfirm() {
                                try {
                                    System.reset();

                                    Toast.makeText(
                                            getActivity(),
                                            getString(R.string.new_session_started),
                                            Toast.LENGTH_SHORT).show();
                                } catch (Exception e) {
                                    new FatalDialog(getString(R.string.error), e
                                            .toString(), getActivity()).show();
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
                        System.getSessionName(), true, false, getActivity(),
                        new InputDialogListener() {
                            @Override
                            public void onInputEntered(String input) {
                                String name = input.trim().replace("/", "")
                                        .replace("..", "");

                                if (!name.isEmpty()) {
                                    try {
                                        String filename = System.saveSession(name);

                                        Toast.makeText(
                                                getActivity(),
                                                getString(R.string.session_saved_to)
                                                        + filename + " .",
                                                Toast.LENGTH_SHORT).show();
                                    } catch (IOException e) {
                                        new ErrorDialog(getString(R.string.error),
                                                e.toString(), getActivity())
                                                .show();
                                    }
                                } else
                                    new ErrorDialog(getString(R.string.error),
                                            getString(R.string.invalid_session),
                                            getActivity()).show();
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
                            getActivity(), new SpinnerDialogListener() {
                        @Override
                        public void onItemSelected(int index) {
                            String session = sessions.get(index);

                            try {
                                System.loadSession(session);
                            } catch (Exception e) {
                                e.printStackTrace();
                                new ErrorDialog(getString(R.string.error),
                                        e.getMessage(), getActivity())
                                        .show();
                            }
                        }
                    }).show();
                } else
                    new ErrorDialog(getString(R.string.error),
                            getString(R.string.no_session_found), getActivity())
                            .show();
                return true;

            case R.id.settings:
                startActivity(new Intent(getActivity(), SettingsActivity.class));
                getActivity().overridePendingTransition(R.anim.fadeout, R.anim.fadein);
                return true;

            case R.id.ss_monitor:
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        Services.getNetworkRadar().onMenuClick(getActivity(), item);
                    }
                }).start();
                return true;

            case R.id.ss_msfrpcd:
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        Services.getMsfRpcdService().onMenuClick(getActivity(), item);
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
                new AboutDialog(getActivity()).show();
                return true;

            default:
                return super.onOptionsItemSelected(item);
        }
    }

    public void onBackPressed() {
        if (mLastBackPressTime < java.lang.System.currentTimeMillis() - 4000) {
            mToast = Toast.makeText(getActivity(), getString(R.string.press_back),
                    Toast.LENGTH_SHORT);
            mToast.show();
            mLastBackPressTime = java.lang.System.currentTimeMillis();
        } else {
            if (mToast != null)
                mToast.cancel();

            new ConfirmDialog(getString(R.string.exit),
                    getString(R.string.close_confirm), getActivity(),
                    new ConfirmDialogListener() {
                        @Override
                        public void onConfirm() {
                            getActivity().finish();
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
        mConnectivityReceiver.unregister();

        // make sure no zombie process is running before destroying the activity
        System.clean(true);

        super.onDestroy();
    }

    public class TargetAdapter extends BaseAdapter implements Runnable, Observer {

        private List<Target> list = System.getTargets();
        private boolean isDark = getActivity().getSharedPreferences("THEME", 0).getBoolean("isDark", false);

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
                LayoutInflater inflater = (LayoutInflater) getActivity()
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

            if (target.hasAlias()) {
                holder.itemTitle.setText(Html.fromHtml("<b>"
                        + target.getAlias() + "</b> <small>( "
                        + target.getDisplayAddress() + " )</small>"));
            } else {
                holder.itemTitle.setText(target.toString());
            }
            holder.itemTitle.setTextColor(ContextCompat.getColor(getActivity().getApplicationContext(), (target.isConnected() ? R.color.app_color : R.color.gray_text)));

            holder.itemTitle.setTypeface(null, Typeface.NORMAL);
            holder.itemImage.setImageResource(target.getDrawableResourceId());
            holder.itemDescription.setText(target.getDescription());

            int openedPorts = target.getOpenPorts().size();

            holder.portCount.setText(String.format("%d", openedPorts));
            holder.portCountLayout.setVisibility(openedPorts < 1 ? View.GONE : View.VISIBLE);
            return row;
        }

        public void clearSelection() {
            synchronized (this) {
                for (Target t : list)
                    t.setSelected(false);
            }
            notifyDataSetChanged();
            if (mActionMode != null)
                mActionMode.finish();
        }

        public void toggleSelection(int position) {
            synchronized (this) {
                Target t = list.get(position);
                t.setSelected(!t.isSelected());
            }
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
            synchronized (this) {
                for (Target t : list)
                    if (t.isSelected())
                        i++;
            }
            return i;
        }

        public ArrayList<Target> getSelected() {
            ArrayList<Target> result = new ArrayList<Target>();
            synchronized (this) {
                for (Target t : list)
                    if (t.isSelected())
                        result.add(t);
            }
            return result;
        }

        public int[] getSelectedPositions() {
            int[] res;
            int j = 0;

            synchronized (this) {
                res = new int[getSelectedCount()];
                for (int i = 0; i < list.size(); i++)
                    if (list.get(i).isSelected())
                        res[j++] = i;
            }
            return res;
        }

        @Override
        public void update(Observable observable, Object data) {
            final Target target = (Target) data;

            if (target == null) {
                // update the whole list
                getActivity().runOnUiThread(this);
                return;
            }

            // update only a row, if it's displayed
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (lv == null)
                        return;
                    synchronized (this) {
                        int start = lv.getFirstVisiblePosition();
                        int end = Math.min(lv.getLastVisiblePosition(), list.size());
                        for (int i = start; i <= end; i++)
                            if (target == list.get(i)) {
                                View view = lv.getChildAt(i - start);
                                getView(i, view, lv);
                                break;
                            }
                    }
                }
            });

        }

        @Override
        public void run() {
            synchronized (this) {
                list = System.getTargets();
            }
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

            mFilter.addAction(SettingsFragment.SETTINGS_WIPE_START);
        }

        public IntentFilter getFilter() {
            return mFilter;
        }

        @Override
        public void onReceive(Context context, Intent intent) {

            if (intent.getAction().equals(SettingsFragment.SETTINGS_WIPE_START)) {
                try {
                    String path;

                    if (intent.hasExtra(SettingsFragment.SETTINGS_WIPE_DIR)) {
                        path = intent.getStringExtra(SettingsFragment.SETTINGS_WIPE_DIR);
                    } else {
                        path = System.getRubyPath() + "' '" + System.getMsfPath();
                    }

                    StopRPCServer();
                    System.getTools().raw.async("rm -rf '" + path + "'", new Child.EventReceiver() {
                        @Override
                        public void onEnd(int exitCode) {
                            getActivity().sendBroadcast(new Intent(SettingsFragment.SETTINGS_WIPE_DONE));
                        }

                        @Override
                        public void onDeath(int signal) {
                            getActivity().sendBroadcast(new Intent(SettingsFragment.SETTINGS_WIPE_DONE));
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
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    new ConfirmDialog(getString(R.string.update_available),
                            update.prompt, getActivity(), new ConfirmDialogListener() {
                        @Override
                        public void onConfirm() {
                            StopRPCServer();
                            Intent i = new Intent(getActivity(), UpdateService.class);
                            i.setAction(UpdateService.START);
                            i.putExtra(UpdateService.UPDATE, update);

                            getActivity().startService(i);
                            mIsUpdateDownloading = true;
                        }

                        @Override
                        public void onCancel() {
                            mIsUpdateDownloading = false;
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

            mIsUpdateDownloading = false;

            System.reloadTools();

            if ((update instanceof MsfUpdate) || (update instanceof RubyUpdate)) {
                startRPCServer();
            }

            if (update instanceof CoreUpdate) {
                onCoreUpdated();
            }

            // restart update checker after a successful update
            startUpdateChecker();
        }

        private void onUpdateError(final Update update, final int message) {

            mIsUpdateDownloading = false;

            if (update instanceof CoreUpdate) {
                onInitializationError(getString(message));
                return;
            }

            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    new ErrorDialog(getString(R.string.error),
                            getString(message), getActivity()).show();
                }
            });

            System.reloadTools();
        }

        @SuppressWarnings("ConstantConditions")
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Update update = null;

            if (intent.hasExtra(UpdateService.UPDATE)) {
                update = (Update) intent.getSerializableExtra(UpdateService.UPDATE);
            }

            switch (action) {
                case UPDATE_CHECKING:
                    if (mEmptyTextView != null)
                        mEmptyTextView.setText(EMPTY_LIST_MESSAGE.replace(
                                "#STATUS#", getString(R.string.checking)));
                    break;
                case UPDATE_NOT_AVAILABLE:
                    if (mEmptyTextView != null)
                        mEmptyTextView.setText(EMPTY_LIST_MESSAGE.replace(
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

    private class ConnectivityReceiver extends ManagedReceiver {
        private static final int CHECK_DELAY = 2000;

        private final IntentFilter mFilter;
        private TimerTask mTask = null;

        public ConnectivityReceiver() {
            mFilter = new IntentFilter();
            mFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        }

        @Override
        public IntentFilter getFilter() {
            return mFilter;
        }

        private String ifacesToString() {
            StringBuilder sb = new StringBuilder();
            for (String iface : mIfaces) {
                if (sb.length() > 0) {
                    sb.append(", ");
                }
                sb.append(iface);
            }
            return sb.toString();
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            synchronized (getActivity()) {
                if (mTask != null) {
                    mTask.cancel();
                }
                mTask = new TimerTask() {
                    @Override
                    public void run() {
                        check();
                    }
                };
                new Timer().schedule(mTask, CHECK_DELAY);
            }
        }

        @Override
        public void unregister() {
            super.unregister();
            synchronized (getActivity()) {
                if (mTask != null) {
                    mTask.cancel();
                }
            }
        }

        private void check() {
            synchronized (getActivity()) {
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        loadInterfaces();

                        String current = System.getIfname();

                        Logger.debug(String.format("current='%s', ifaces=[%s], haveInterface=%s, isAnyNetInterfaceAvailable=%s",
                                current != null ? current : "(null)",
                                ifacesToString(), haveInterface(current), isAnyNetInterfaceAvailable));

                        if (haveInterface(current)) {
                            onConnectionResumed();
                        } else if (current != null) {
                            onConnectionLost();
                        } else if (isAnyNetInterfaceAvailable) {
                            onNetworkInterfaceChanged();
                        }

                    }
                });

                mTask = null;
            }
        }
    }
}