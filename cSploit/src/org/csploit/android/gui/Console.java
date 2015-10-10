package org.csploit.android.gui;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.text.Html;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.FatalDialog;
import org.csploit.android.net.metasploit.ShellSession;

/**
 * this Activity allow user to run commands on pwned shells
 */
public class Console extends AppCompatActivity {

    private EditText mInput;
    private TextView mOutput;
    private ScrollView mScrollView;
    private FloatingActionButton runButton;
    private ShellSession mSession;
    private ConsoleReceiver mReceiver;

    private class ConsoleReceiver extends ShellSession.RpcShellReceiver {

        @Override
        public void onNewLine(final String line) {
            Console.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mOutput.append("\n" + line);
                }
            });
        }

        @Override
        public void onEnd(int exitCode) {
            if (exitCode != 0)
                Toast.makeText(Console.this, "command returned " + exitCode, Toast.LENGTH_SHORT).show();
            try {
                Thread.sleep(200);
                Console.this.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mOutput.append(Html.fromHtml("\n<font color=\"green\">Enter Command:&gt;</font>  "));
                        mScrollView.fullScroll(ScrollView.FOCUS_DOWN);
                        mScrollView.scrollBy(0, -mInput.getLineHeight());
                        mInput.setText("");
                        mInput.requestFocus();
                    }
                });            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onRpcClosed() {
            new FatalDialog("Connection lost", "RPC connection closed", Console.this).show();
        }

        @Override
        public void onTimedOut() {
            new FatalDialog("Command timed out", "your submitted command generated an infinite loop or some connection error occurred.", Console.this).show();
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.console_layout);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mInput = (EditText) findViewById(R.id.input);
        mOutput = (TextView) findViewById(R.id.output);
        mScrollView = (ScrollView) findViewById(R.id.outputscroll);
        runButton = (FloatingActionButton) findViewById(R.id.myFAB);

        // make TextView scrollable
       // mOutput.setMovementMethod(ScrollingMovementMethod.getInstance());
        // mOutput.setHorizontallyScrolling(true);

        // make TextView selectable ( require HONEYCOMB or newer )
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
            mOutput.setTextIsSelectable(true);

        mReceiver = new ConsoleReceiver();
        mSession = (ShellSession) System.getCurrentSession();
        mOutput.append(Html.fromHtml("<font color=\"green\">Enter Command:&gt;</font>\n\n"));
        mInput.setImeActionLabel(getResources().getString(R.string.run), KeyEvent.KEYCODE_ENTER);
        runButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mOutput.append(Html.fromHtml("<font color=\"red\">" + mInput.getText().toString() + "</font>\n"));
                mSession.addCommand(mInput.getText().toString(), mReceiver);
            }
        });
        mInput.setOnEditorActionListener(new EditText.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_NEXT) {
                    mOutput.append(Html.fromHtml("<font color=\"red\">" + mInput.getText().toString() + "</font>\n"));
                    mSession.addCommand(mInput.getText().toString(), mReceiver);
                    return true;
                }
                return false;
            }
        });
        mInput.setOnKeyListener(new EditText.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (event.getAction() == KeyEvent.ACTION_DOWN
                        && event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
                    mOutput.append(Html.fromHtml("<font color=\"red\">" + mInput.getText().toString() + "</font>\n"));
                    mSession.addCommand(mInput.getText().toString(), mReceiver);
                    return true;
                }
                return false;
            }
        });
        mInput.requestFocus();
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
        overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
        System.setCurrentSession(null);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.console, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.clear:
                mOutput.setText("");
                return true;
            case android.R.id.copy:
                if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.HONEYCOMB) {
                    android.text.ClipboardManager clipboard = (android.text.ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                    clipboard.setText(mOutput.getText().toString());
                } else {
                    android.content.ClipboardManager clipboard = (android.content.ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                    android.content.ClipData clip = android.content.ClipData.newPlainText("Copied Text", mOutput.getText().toString());
                    clipboard.setPrimaryClip(clip);
                }
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }
}
