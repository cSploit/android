package org.csploit.android.gui;

import android.content.Context;
import android.os.Bundle;
import android.text.Html;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.material.floatingactionbutton.FloatingActionButton;

import org.csploit.android.R;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.FatalDialog;
import org.csploit.android.net.metasploit.ShellSession;

import androidx.appcompat.app.AppCompatActivity;

/**
 * this Activity allow user to run commands on pwned shells
 */
public class Console extends AppCompatActivity {

    private EditText mInput;
    private TextView mOutput;
    private ScrollView mScrollView;
    private ShellSession mSession;
    private ConsoleReceiver mReceiver;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.console_layout);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mInput = findViewById(R.id.input);
        mOutput = findViewById(R.id.output);
        mScrollView = findViewById(R.id.outputscroll);
        FloatingActionButton runButton = findViewById(R.id.myFAB);

        // make TextView scrollable
        // mOutput.setMovementMethod(ScrollingMovementMethod.getInstance());
        // mOutput.setHorizontallyScrolling(true);

        // make TextView selectable ( require HONEYCOMB or newer )
        mOutput.setTextIsSelectable(true);

        mReceiver = new ConsoleReceiver();
        mSession = (ShellSession) System.getCurrentSession();
        mOutput.append(Html.fromHtml("<font color=\"green\">Enter Command:&gt;</font>\n\n"));
        mInput.setImeActionLabel(getResources().getString(R.string.run), KeyEvent.KEYCODE_ENTER);
        runButton.setOnClickListener(v -> {
            mOutput.append(Html.fromHtml("<font color=\"red\">" + mInput.getText().toString() + "</font>\n"));
            mSession.addCommand(mInput.getText().toString(), mReceiver);
        });
        mInput.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_NEXT) {
                mOutput.append(Html.fromHtml("<font color=\"red\">" + mInput.getText().toString() + "</font>\n"));
                mSession.addCommand(mInput.getText().toString(), mReceiver);
                return true;
            }
            return false;
        });
        mInput.setOnKeyListener((v, keyCode, event) -> {
            if (event.getAction() == KeyEvent.ACTION_DOWN
                    && event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
                mOutput.append(Html.fromHtml("<font color=\"red\">" + mInput.getText().toString() + "</font>\n"));
                mSession.addCommand(mInput.getText().toString(), mReceiver);
                return true;
            }
            return false;
        });
        mInput.requestFocus();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.clear:
                mOutput.setText("");
                return true;
            case android.R.id.copy:
                android.content.ClipboardManager clipboard = (android.content.ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                android.content.ClipData clip = android.content.ClipData.newPlainText("Copied Text", mOutput.getText().toString());
                clipboard.setPrimaryClip(clip);
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
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

    private class ConsoleReceiver extends ShellSession.RpcShellReceiver {

        @Override
        public void onNewLine(final String line) {
            Console.this.runOnUiThread(() -> mOutput.append("\n" + line));
        }

        @Override
        public void onEnd(int exitCode) {
            if (exitCode != 0)
                Toast.makeText(Console.this, "command returned " + exitCode, Toast.LENGTH_SHORT).show();
            try {
                Thread.sleep(200);
                Console.this.runOnUiThread(() -> {
                    mOutput.append(Html.fromHtml("\n<font color=\"green\">Enter Command:&gt;</font>  "));
                    mScrollView.fullScroll(ScrollView.FOCUS_DOWN);
                    mScrollView.scrollBy(0, -mInput.getLineHeight());
                    mInput.setText("");
                    mInput.requestFocus();
                });
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onRpcClosed() {
            new FatalDialog("Connection lost", "RPC connection closed", Console.this).show();
        }

        @Override
        public void onTimedOut() {
            new FatalDialog("Command timed out",
                    "your submitted command generated an infinite loop or some connection error occurred.",
                    Console.this).show();
        }
    }
}
