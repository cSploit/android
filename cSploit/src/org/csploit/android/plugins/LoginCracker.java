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

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.csploit.android.MainActivity;
import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Client;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.events.Login;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.FinishDialog;
import org.csploit.android.gui.dialogs.InputDialog;
import org.csploit.android.gui.dialogs.InputDialog.InputDialogListener;
import org.csploit.android.net.Target;
import org.csploit.android.net.Target.Port;
import org.csploit.android.tools.Hydra;
import org.tukaani.xz.check.Check;

import java.text.Collator;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

public class LoginCracker extends Plugin {
    private static final int SELECT_USER_WORDLIST = 1012;
    private static final int SELECT_PASS_WORDLIST = 1013;
    private static final String[] PROTOCOLS = new String[]{"ftp",
            "http-get", "https-get", "icq", "imap", "imap-ntlm", "ldap", "oracle-listener",
            "mssql", "mysql", "pcanywhere", "nntp", "pcnfs", "pop3", "pop3s",
            "pop3-ntlm", "rexec", "rlogin", "rsh", "smb", "smbnt", "socks5",
            "ssh", "telnet", "cisco", "cisco-enable", "vnc", "smtp", "smtps", "snmp", "cvs",
            "smtp-auth", "smtp-auth-ntlm", "teamspeak", "sip", "vmauthd"};
    private static final String[] CHARSETS = new String[]{"a-z", "A-Z",
            "a-z0-9", "A-Z0-9", "a-zA-Z0-9", "a-zA-Z0-9 + custom"};
    private static final String[] CHARSETS_MAPPING = new String[]{"a", "A",
            "a1", "A1", "aA1", null};
    private static final String[] LENGTHS = new String[]{"1", "2", "3", "4",
            "5", "6"};
    private static String[] USERNAMES = new String[]{"admin", "saroot",
            "root", "administrator", "Administrator", "Admin", "admin",
            "system", "webadmin", "daemon ", "bin ", "sys ", "adm ", "lp ",
            "uucp ", "nuucp ", "smmsp ", "listen ", "gdm ", "sshd ",
            "webservd ", "oracle", "httpd", "nginx", "-- ADD --"};
    private static String[] STOP = new String[]{"Globally", "Per host", "Don't stop"};
    private Spinner mPortSpinner = null;
    private Spinner mProtocolSpinner = null;
    private ProtocolAdapter mProtocolAdapter = null;
    private Spinner mCharsetSpinner = null;
    private Spinner mUserSpinner = null;
    private Spinner mMinSpinner = null;
    private Spinner mMaxSpinner = null;
    private ToggleButton mStartButton = null;
    private TextView mStatusText = null;
    private ProgressBar mActivity = null;
    private ProgressBar mProgressBar = null;
    private Intent mWordlistPicker = null;
    private String mUserWordlist = null;
    private String mPassWordlist = null;
    private boolean mRunning = false;
    private boolean mAccountFound = false;
    private Receiver mAttemptsReceiver = null;
    private String mCustomCharset = null;
    private CheckBox mStopCheck = null;
    private CheckBox mLoopCheck = null;
    private EditText mThreads = null;
    private EditText mOpt = null;
    private ListView mFound = null;
    private ArrayAdapter mAdapter = null;

    public LoginCracker() {
        super(R.string.login_cracker, R.string.login_cracker_desc,

                new Target.Type[]{Target.Type.ENDPOINT, Target.Type.REMOTE},
                R.layout.plugin_login_cracker, R.drawable.action_login);
        mAttemptsReceiver = new Receiver();
    }

    private void setStoppedState() {
        if (mProcess != null) {
            mProcess.kill();
            mProcess = null;
        }
        mRunning = false;

        LoginCracker.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.setVisibility(View.INVISIBLE);
                mProgressBar.setProgress(0);
                mStartButton.setChecked(false);
                if (!mAccountFound) {
                    mStatusText.setTextColor(Color.DKGRAY);
                    mStatusText.setText(getString(R.string.stopped_dots));
                }
            }
        });
    }

    private void setStartedState() {
        int min = Integer.parseInt((String) mMinSpinner.getSelectedItem());
        int max = Integer.parseInt((String) mMaxSpinner.getSelectedItem());
        if (min > max)
            max = min;
        mAccountFound = false;
        mActivity.setVisibility(View.VISIBLE);
        mStatusText.setTextColor(Color.DKGRAY);
        mStatusText.setText(getString(R.string.starting_dots));

        try {
            mProcess =
                    System.getTools().hydra
                            .crack(System.getCurrentTarget(),
                                    Integer.parseInt((String) mPortSpinner
                                            .getSelectedItem()),
                                    (String) mProtocolSpinner.getSelectedItem(),
                                    mCustomCharset == null ? CHARSETS_MAPPING[mCharsetSpinner
                                            .getSelectedItemPosition()] : mCustomCharset,
                                    min, max, (String) mUserSpinner.getSelectedItem(),
                                    mUserWordlist, mPassWordlist, mStopCheck.isChecked(),
                                    mLoopCheck.isChecked(), Integer.valueOf(mThreads.getText().toString()), mOpt.getText().toString(), mAttemptsReceiver);

            mRunning = true;

        } catch (ChildManager.ChildNotStartedException e) {
            System.errorLogging(e);
            Toast.makeText(LoginCracker.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
        }
    }

    public void onCreate(Bundle savedInstanceState) {
        SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
        Boolean isDark = themePrefs.getBoolean("isDark", false);
        if (isDark)
            setTheme(R.style.DarkTheme);
        else
            setTheme(R.style.AppTheme);
        super.onCreate(savedInstanceState);

        if (!System.getCurrentTarget().hasOpenPorts())
            new FinishDialog(getString(R.string.warning),
                    getString(R.string.no_open_ports), this).show();

        final ArrayList<String> ports = new ArrayList<String>();

        for (Port port : System.getCurrentTarget().getOpenPorts())
            ports.add(Integer.toString(port.getNumber()));

        mProtocolAdapter = new ProtocolAdapter();

        mPortSpinner = (Spinner) findViewById(R.id.portSpinner);
        mPortSpinner.setAdapter(new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, ports));
        mPortSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> adapterView, View view,
                                       int position, long id) {
            }
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });

        mProtocolSpinner = (Spinner) findViewById(R.id.protocolSpinner);
        mProtocolSpinner.setAdapter(new ProtocolAdapter());
        mProtocolSpinner
                .setOnItemSelectedListener(new OnItemSelectedListener() {
                    public void onItemSelected(AdapterView<?> adapterView,
                                               View view, int position, long id) {
                        int portIndex = mProtocolAdapter.getPortIndexByProtocol(position);
                        if (portIndex != -1)
                            mPortSpinner.setSelection(portIndex);
                    }

                    public void onNothingSelected(AdapterView<?> adapterView) {
                    }
                });

        mCharsetSpinner = (Spinner) findViewById(R.id.charsetSpinner);
        mCharsetSpinner.setAdapter(new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, CHARSETS));
        mCharsetSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> adapter, View view,
                                       int position, long id) {
                if (CHARSETS_MAPPING[position] == null) {
                    new InputDialog(getString(R.string.custom_charset),
                            getString(R.string.enter_chars_wanted),
                            LoginCracker.this, new InputDialogListener() {
                        @Override
                        public void onInputEntered(String input) {
                            input = input.trim();
                            if (!input.isEmpty())
                                mCustomCharset = "aA1" + input;

                            else {
                                mCustomCharset = null;
                                mCharsetSpinner.setSelection(0);
                            }
                        }
                    }).show();
                } else
                    mCustomCharset = null;
            }

            public void onNothingSelected(AdapterView<?> arg0) {
            }
        });

        mUserSpinner = (Spinner) findViewById(R.id.userSpinner);
        mUserSpinner.setAdapter(new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, USERNAMES));
        mUserSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> adapter, View view,
                                       int position, long id) {
                String user = (String) adapter.getItemAtPosition(position);
                if (user != null && user.equals("-- ADD --")) {
                    new InputDialog(getString(R.string.add_username),
                            getString(R.string.enter_username),
                            LoginCracker.this, new InputDialogListener() {
                        @Override
                        public void onInputEntered(String input) {
                            USERNAMES = Arrays.copyOf(USERNAMES,
                                    USERNAMES.length + 1);
                            USERNAMES[USERNAMES.length - 1] = "-- ADD --";
                            USERNAMES[USERNAMES.length - 2] = input;

                            mUserSpinner
                                    .setAdapter(new ArrayAdapter<String>(
                                            LoginCracker.this,
                                            android.R.layout.simple_spinner_item,
                                            USERNAMES));
                            mUserSpinner
                                    .setSelection(USERNAMES.length - 2);
                        }
                    }).show();
                }
            }

            public void onNothingSelected(AdapterView<?> arg0) {
            }
        });

        mMaxSpinner = (Spinner) findViewById(R.id.maxSpinner);
        mMaxSpinner.setAdapter(new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, LENGTHS));
        mMinSpinner = (Spinner) findViewById(R.id.minSpinner);
        mMinSpinner.setAdapter(new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, LENGTHS));

        mStartButton = (ToggleButton) findViewById(R.id.startButton);
        mStatusText = (TextView) findViewById(R.id.statusText);
        mProgressBar = (ProgressBar) findViewById(R.id.progressBar);
        mActivity = (ProgressBar) findViewById(R.id.activity);

        mStopCheck = (CheckBox) findViewById(R.id.stopCheck);
        mLoopCheck = (CheckBox) findViewById(R.id.loopCheck);
        mThreads = (EditText) findViewById(R.id.tasksEdit);
        mFound = (ListView) findViewById(R.id.foundList);
        mAdapter = new ArrayAdapter(this, R.layout.found_layout);
        mFound.setAdapter(mAdapter);

        mProgressBar.setMax(100);

        mStartButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mRunning) {
                    setStoppedState();
                } else {
                    setStartedState();
                }
            }
        });

        mWordlistPicker = new Intent();
        mWordlistPicker.addCategory(Intent.CATEGORY_OPENABLE);
        mWordlistPicker.setType("text/*");
        mWordlistPicker.setAction(Intent.ACTION_GET_CONTENT);
        mWordlistPicker.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.login_cracker, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        int itemId = item.getItemId();

        switch (itemId) {
            case R.id.user_wordlist:

                startActivityForResult(Intent.createChooser(mWordlistPicker,
                        getString(R.string.select_wordlist)), SELECT_USER_WORDLIST);

                return true;

            case R.id.pass_wordlist:

                startActivityForResult(Intent.createChooser(mWordlistPicker,
                        getString(R.string.select_wordlist)), SELECT_PASS_WORDLIST);

                return true;

            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @SuppressWarnings("ConstantConditions")
    @Override
    protected void onActivityResult(int request, int result, Intent intent) {
        super.onActivityResult(request, result, intent);

        if (request == SELECT_USER_WORDLIST && result == RESULT_OK) {
            try {
                String fileName = null;

                if (intent != null && intent.getData() != null)
                    fileName = intent.getData().getPath();

                if (fileName == null) {
                    new ErrorDialog(getString(R.string.error),
                            getString(R.string.error_filepath),
                            LoginCracker.this).show();
                } else {
                    mUserWordlist = fileName;
                }
            } catch (Exception e) {
                System.errorLogging(e);
            }
        } else if (request == SELECT_PASS_WORDLIST && result == RESULT_OK) {
            try {
                String fileName = null;

                if (intent != null && intent.getData() != null)
                    fileName = intent.getData().getPath();

                if (fileName == null) {
                    new ErrorDialog(getString(R.string.error),
                            getString(R.string.error_filepath),
                            LoginCracker.this).show();
                } else {
                    mPassWordlist = fileName;
                }
            } catch (Exception e) {
                System.errorLogging(e);
            }
        }
    }

    @Override
    public void onBackPressed() {
        setStoppedState();
        super.onBackPressed();
    }

    public class ProtocolAdapter extends BaseAdapter implements SpinnerAdapter {
        private List<Port> mOpenPorts = null;
        private ArrayList<String> mProtocols = null;

        public ProtocolAdapter() {
            mOpenPorts = System.getCurrentTarget().getOpenPorts();
            mProtocols = new ArrayList<String>(Arrays.asList(PROTOCOLS));

            // first sort alphabetically
            Collections.sort(mProtocols, Collator.getInstance());

            // remove open ports protocols from the list
            List<String> openProtocols = new ArrayList<String>();
            Iterator<String> iterator = mProtocols.iterator();

            while (iterator.hasNext()) {
                String sProtocol = iterator.next();

                if (hasProtocolOpenPort(sProtocol)) {
                    iterator.remove();
                    openProtocols.add(sProtocol);
                }
            }

            Collections.sort(openProtocols, Collator.getInstance());

            // add open ports protocol on top of the list
            mProtocols.addAll(0, openProtocols);
        }

        private boolean hasProtocolOpenPort(String sProtocol) {
            for (Port port : mOpenPorts) {
                String protocol = System.getProtocolByPort("" + port.getNumber());
                if (protocol != null
                        && (protocol.equals(sProtocol) || sProtocol
                        .toLowerCase().contains(protocol))) {
                    return true;
                }
            }

            return false;
        }

        public int getPortIndexByProtocol(int position) {
            String protocol = mProtocols.get(position);

            for (int i = 0; i < mOpenPorts.size(); i++) {
                String portProtocol = System.getProtocolByPort(mOpenPorts.get(i).getNumber());
                if (portProtocol != null) {
                    if (portProtocol.equals(protocol) || protocol.toLowerCase().contains(portProtocol))
                        return i;
                }
            }

            return -1;
        }

        public int getCount() {
            return mProtocols.size();
        }

        public Object getItem(int position) {
            return mProtocols.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            LayoutInflater inflater = (LayoutInflater) LoginCracker.this
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);

            View spinView = inflater.inflate(
                    android.R.layout.simple_spinner_item, null);
            TextView textView = (TextView) (spinView != null ? spinView
                    .findViewById(android.R.id.text1) : null);

            String sProtocol = mProtocols.get(position);

            if (textView != null)
                textView.setText(sProtocol);

            if (hasProtocolOpenPort(sProtocol)) {
                if (textView != null) {
                    textView.setTextColor(Color.GREEN);
                    textView.setTypeface(null, Typeface.BOLD);
                }
            }

            return spinView;
        }
    }

    private class Receiver extends Hydra.AttemptsReceiver {

        @Override
        public void onAttemptStatus(final int rate, final int progress, final int left) {
            int total = progress + left;
            final int percentage = (int) ((progress / total) * 100);

            LoginCracker.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mStatusText.setTextColor(Color.DKGRAY);
                    mProgressBar.setProgress(percentage);
                }
            });
        }

        @Override
        public void onAccountFound(final String login, final String password) {
            //TODO CRACK SEVERAL ACCOUNTS -f -F
            final String creds = login + ":" + password;
            LoginCracker.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mAdapter.add(creds);
                    mAdapter.notifyDataSetChanged();
                }
            });
        }

        @Override
        public void onWarning(String message) {
            final String error = message;

            LoginCracker.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mStatusText.setTextColor(Color.YELLOW);
                    mStatusText.setText(error);
                }
            });
        }

        @Override
        public void onError(String message) {
            final String error = message;

            LoginCracker.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    setStoppedState();
                    mStatusText.setTextColor(Color.RED);
                    mStatusText.setText(error);
                }
            });
        }
        @Override
        public void onStart(String command) {
            super.onStart(command);
            LoginCracker.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mRunning = true;
                }
            });
        }

        @Override
        public void onEnd(int code) {
            if (mRunning) {
                LoginCracker.this.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        setStoppedState();
                    }
                });
            }
            super.onEnd(code);
        }
    }
}
