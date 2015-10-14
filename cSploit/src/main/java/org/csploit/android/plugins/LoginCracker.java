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
import android.os.Build;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.content.ContextCompat;
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
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.TextView;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.FinishDialog;
import org.csploit.android.gui.dialogs.InputDialog;
import org.csploit.android.gui.dialogs.InputDialog.InputDialogListener;
import org.csploit.android.net.Target;
import org.csploit.android.net.Target.Port;
import org.csploit.android.tools.Hydra;

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
          "http-head", "icq", "imap", "imap-ntlm", "ldap", "oracle-listener",
          "mssql", "mysql", "pcanywhere", "nntp", "pcnfs", "pop3",
          "pop3-ntlm", "rexec", "rlogin", "rsh", "smb", "smbnt", "socks5",
          "ssh", "telnet", "cisco", "cisco-enable", "vnc", "snmp", "cvs",
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
  private Spinner mPortSpinner = null;
  private Spinner mProtocolSpinner = null;
  private ProtocolAdapter mProtocolAdapter = null;
  private Spinner mCharsetSpinner = null;
  private Spinner mUserSpinner = null;
  private Spinner mMinSpinner = null;
  private Spinner mMaxSpinner = null;
  private FloatingActionButton mStartButton = null;
  private TextView mStatusText = null;
  private ProgressBar mActivity = null;
  private ProgressBar mProgressBar = null;
  private Intent mWordlistPicker = null;
  private String mUserWordlist = null;
  private String mPassWordlist = null;
  private boolean mRunning = false;
  private boolean mAccountFound = false;
  private AttemptReceiver mReceiver = null;
  private String mCustomCharset = null;

  public LoginCracker() {
    super(R.string.login_cracker, R.string.login_cracker_desc,

            new Target.Type[]{Target.Type.ENDPOINT, Target.Type.REMOTE},
            R.layout.plugin_login_cracker, R.drawable.action_login);
  }

  private void setStoppedState(final String user, final String pass) {
    if (mProcess != null) {
      mProcess.kill();
      mProcess = null;
    }
    mRunning = false;
    mAccountFound = true;

    LoginCracker.this.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        mActivity.setVisibility(View.INVISIBLE);
        mProgressBar.setProgress(0);
        mStartButton.setImageDrawable(ContextCompat.getDrawable(getBaseContext(), R.drawable.ic_play_arrow_24dp));
        mStatusText.setTextColor(Color.GREEN);
        mStatusText.setText("USERNAME = " + user + " - PASSWORD = "
                + pass);
      }
    });
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
        mStartButton.setImageDrawable(ContextCompat.getDrawable(getBaseContext(), R.drawable.ic_play_arrow_24dp));
        if (!mAccountFound) {
          mStatusText.setTextColor(Color.DKGRAY);
          mStatusText.setText(getString(R.string.stopped_dots));
        }
      }
    });
  }

  private void setStartedState() {
    int min = Integer.parseInt((String) mMinSpinner.getSelectedItem()), max = Integer
            .parseInt((String) mMaxSpinner.getSelectedItem());

    if (min > max)
      max = min + 1;

    mAccountFound = false;

    try {
      mStartButton.setImageDrawable(ContextCompat.getDrawable(getBaseContext(), R.drawable.ic_stop_24dp));
      mProcess =
              System.getTools().hydra
                      .crack(System.getCurrentTarget(),
                              Integer.parseInt((String) mPortSpinner
                                      .getSelectedItem()),
                              (String) mProtocolSpinner.getSelectedItem(),
                              mCustomCharset == null ? CHARSETS_MAPPING[mCharsetSpinner
                                      .getSelectedItemPosition()] : mCustomCharset,
                              min, max, (String) mUserSpinner.getSelectedItem(),
                              mUserWordlist, mPassWordlist, mReceiver);

      mActivity.setVisibility(View.VISIBLE);
      mStatusText.setTextColor(Color.DKGRAY);
      mStatusText.setText(getString(R.string.starting_dots));
      mRunning = true;

    } catch (ChildManager.ChildNotStartedException e) {
      setStoppedState();
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
      public void onItemSelected(AdapterView<?> adapter, View view,
                                 int position, long id) {
        String port = (String) adapter.getItemAtPosition(position);
        int protocolIndex = mProtocolAdapter.getIndexByPort(port);

        if (protocolIndex != -1)
          mProtocolSpinner.setSelection(protocolIndex);
      }

      public void onNothingSelected(AdapterView<?> arg0) {
      }
    });

    mProtocolSpinner = (Spinner) findViewById(R.id.protocolSpinner);
    mProtocolSpinner.setAdapter(new ProtocolAdapter());
    mProtocolSpinner
            .setOnItemSelectedListener(new OnItemSelectedListener() {
              public void onItemSelected(AdapterView<?> adapter,
                                         View view, int position, long id) {
                int portIndex = mProtocolAdapter
                        .getPortIndexByProtocol(position);
                if (portIndex != -1)
                  mPortSpinner.setSelection(portIndex);
              }

              public void onNothingSelected(AdapterView<?> arg0) {
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

    mStartButton = (FloatingActionButton) findViewById(R.id.startFAB);
    mStatusText = (TextView) findViewById(R.id.statusText);
    mProgressBar = (ProgressBar) findViewById(R.id.progressBar);
    mActivity = (ProgressBar) findViewById(R.id.activity);

    mProgressBar.setMax(100);

    mReceiver = new AttemptReceiver();

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

    if(Build.VERSION.SDK_INT >= 11)
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
    overridePendingTransition(R.anim.fadeout, R.anim.fadein);
  }

  public class ProtocolAdapter extends BaseAdapter implements SpinnerAdapter {
    private List<Port> mOpenPorts = null;
    private ArrayList<String> mProtocols = null;

    private class Holder {
      public TextView textView;
      public String protocol;
    }

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

    public int getIndexByPort(String port) {
      String portProtocol = System.getProtocolByPort(port), protocol;

      if (portProtocol != null) {
        for (int i = 0; i < mProtocols.size(); i++) {
          protocol = mProtocols.get(i);
          if (protocol.equals(portProtocol)
                  || protocol.toLowerCase().contains(portProtocol)) {
            return i;
          }
        }
      }

      return -1;
    }

    public int getPortIndexByProtocol(int position) {
      String protocol = mProtocols.get(position), portProtocol;

      for (int i = 0; i < mOpenPorts.size(); i++) {
        portProtocol = System.getProtocolByPort(""
                + mOpenPorts.get(i).getNumber());
        if (portProtocol != null) {
          if (portProtocol.equals(protocol)
                  || protocol.toLowerCase().contains(portProtocol))
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
      View spinView = convertView;
      Holder holder;

      if(spinView == null) {
        LayoutInflater inflater = (LayoutInflater) LoginCracker.this
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        holder = new Holder();

        spinView = inflater.inflate(
                android.R.layout.simple_spinner_item, parent, false);

        holder.textView = (TextView) (spinView != null ? spinView
                .findViewById(android.R.id.text1) : null);

        holder.protocol = mProtocols.get(position);

        if (holder.textView != null)
          holder.textView.setText(holder.protocol);

        if(spinView != null)
          spinView.setTag(holder);

      } else {
        holder = (Holder) spinView.getTag();
      }

      if (hasProtocolOpenPort(holder.protocol) && holder.textView != null) {
        holder.textView.setTextColor(Color.GREEN);
        holder.textView.setTypeface(null, Typeface.BOLD);
      }

      return spinView;
    }
  }

  private class AttemptReceiver extends Hydra.AttemptReceiver {
    private long mStarted = 0;
    private long mEndTime = 0;
    private long mLastAttempt = 0;
    private long mLastLeft = 0;
    private long mNow = 0;

    private String formatTimeLeft(long millis) {
      int seconds = (int) (millis / 1000) % 60, minutes = (int) ((millis / 60000) % 60), hours = (int) ((millis / 3600000) % 24);
      String time = "";

      if (hours > 0)
        time += hours + "h";

      if (minutes > 0)
        time += " " + minutes + "m";

      if (seconds > 0)
        time += " " + seconds + "s";

      return time.trim();
    }

    private void reset() {
      mLastAttempt = 0;
      mStarted = 0;
      mEndTime = 0;
      mLastLeft = 0;
      mNow = 0;
    }

    @Override
    public void onStart(String command) {
      reset();

      mStarted = java.lang.System.currentTimeMillis();

      super.onStart(command);
    }

    @Override
    public void onAttemptStatus(final long progress, final long total) {
      String status = "";

      mNow = java.lang.System.currentTimeMillis();

      if (mLastAttempt == 0)
        mLastAttempt = mNow;

      else {
        long timeElapsed = (mNow - mStarted);
        double timeNeeded = (timeElapsed / progress) * total;

        mEndTime = (mStarted + (long) timeNeeded);
        mLastAttempt = mNow;
        mLastLeft = mEndTime - mNow;

        status += "\t[ " + formatTimeLeft(mLastLeft) + " "
                + getString(R.string.left) + " ]";
      }

      final String text = status;
      final int percentage = (int) ((progress / total) * 100);

      LoginCracker.this.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          mStatusText.setTextColor(Color.DKGRAY);
          mStatusText.setText(text);
          mProgressBar.setProgress(percentage);
        }
      });
    }

    @Override
    public void onAccountFound(final String login, final String password) {
      reset();

      LoginCracker.this.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          setStoppedState(login, password);
        }
      });
    }

    @Override
    public void onWarning(String message) {
      reset();

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
      reset();

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
    public void onEnd(int code) {
      if (mRunning) {
        reset();

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
