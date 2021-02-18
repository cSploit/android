package org.csploit.android.gui;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;
import android.text.method.ScrollingMovementMethod;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.FatalDialog;
import org.csploit.android.net.metasploit.ShellSession;

/**
 * this Activity allow user to run commands on pwned shells
 */
public class Console extends ActionBarActivity {

  private EditText  mInput;
  private EditText  mOutput;
  private Button    runButton;
  private ShellSession   mSession;
  private ConsoleReceiver mReceiver;

  private class ConsoleReceiver extends ShellSession.RpcShellReceiver {

    @Override
    public void onNewLine(final String line) {
      Console.this.runOnUiThread( new Runnable() {
        @Override
        public void run() {
          mOutput.append(line + "\n");
        }
      });
    }

    @Override
    public void onEnd(int exitCode) {
      if(exitCode!=0)
        Toast.makeText(Console.this,"command returned "+exitCode,Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onRpcClosed() {
      new FatalDialog("Connection lost","RPC connection closed",Console.this).show();
    }

    @Override
    public void onTimedOut() {
      new FatalDialog("Command timed out","your submitted command generated an infinite loop or some connection error occurred.",Console.this).show();
    }
  }

  private View.OnClickListener clickListener = new View.OnClickListener() {
    @Override
    public void onClick(View v) {
      mSession.addCommand(mInput.getText().toString(), mReceiver);
    }
  };


  @Override
  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    setContentView(R.layout.console_layout);
    getSupportActionBar().setDisplayHomeAsUpEnabled(true);

    mInput = (EditText) findViewById(R.id.input);
    mOutput = (EditText) findViewById(R.id.output);
    runButton  = (Button)   findViewById(android.R.id.button1);

    // make TextView scrollable
    mOutput.setMovementMethod(ScrollingMovementMethod.getInstance());
    mOutput.setHorizontallyScrolling(true);

    // make TextView selectable ( require HONEYCOMB or newer )
    if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
      mOutput.setTextIsSelectable(true);

    mReceiver = new ConsoleReceiver();
    mSession = (ShellSession) System.getCurrentSession();

    runButton.setOnClickListener(clickListener);

    mInput.setOnEditorActionListener(new TextView.OnEditorActionListener() {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (actionId != EditorInfo.IME_ACTION_SEND)
          return false;
        clickListener.onClick(v);
        return true;
      }
    });

  }

  @Override
  public void onBackPressed() {
    super.onBackPressed();
    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
    System.setCurrentSession(null);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu){
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.console, menu);
    return super.onCreateOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    switch (item.getItemId()) {
      case R.id.clear:
        mOutput.setText("");
        return true;
      case android.R.id.copy:
        if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.HONEYCOMB) {
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
