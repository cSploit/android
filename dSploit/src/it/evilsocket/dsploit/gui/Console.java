package it.evilsocket.dsploit.gui;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.net.metasploit.*;

import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;

/**
 * this Activity allow user to run commands on pwned shells
 */
public class Console extends SherlockActivity {

  private EditText  mInput;
  private EditText  mOutput;
  private Button    runButton;
  private ShellSession   mSession;
  private ConsoleReceiver mReceiver;

  private class ConsoleReceiver implements Shell.OutputReceiver {
    @Override
    public void onStart(String command) { }

    @Override
    public void onNewLine(String line) {
      final String _line = line;
      Console.this.runOnUiThread( new Runnable() {
        @Override
        public void run() {
          mOutput.append(_line + "\n");
        }
      });
    }

    @Override
    public void onEnd(int exitCode) {
      if(exitCode!=0)
        Toast.makeText(Console.this,"command returned "+exitCode,Toast.LENGTH_SHORT).show();
    }
  }

  private View.OnClickListener clickListener = new View.OnClickListener() {
    @Override
    public void onClick(View v) {
      mSession.addCommand(mInput.getText().toString());
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
    if(Build.VERSION.SDK_INT >= 11)
      mOutput.setTextIsSelectable(true);

    mReceiver = new ConsoleReceiver();
    mSession = (ShellSession) System.getCurrentSession();
    mSession.setReceiver(mReceiver);
    mSession.wakeUp();

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
    mSession.sleep();
    System.setCurrentSession(null);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu){
    MenuInflater inflater = getSupportMenuInflater();
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
