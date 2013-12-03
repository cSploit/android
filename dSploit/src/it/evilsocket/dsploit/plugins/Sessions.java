/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *             Massimo Dragano aka tux_mind <massimo.dragano@gmail.com>
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
package it.evilsocket.dsploit.plugins;

import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.TimeoutException;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.Console;
import it.evilsocket.dsploit.gui.dialogs.ChoiceDialog;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.FinishDialog;
import it.evilsocket.dsploit.gui.dialogs.ListChoiceDialog;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.metasploit.RPCClient;
import it.evilsocket.dsploit.net.metasploit.Session;
import it.evilsocket.dsploit.net.metasploit.ShellSession;

public class Sessions extends Plugin {
	private ListView mListView = null;
  private ArrayList<Session> mResults;
  private ArrayAdapter<Session> mAdapter = null;
  private Sessions              UIThread = null;

	public Sessions() {
		super(R.string.sessions, R.string.sessions_desc,

		new Target.Type[] { Target.Type.ENDPOINT, Target.Type.REMOTE },
				R.layout.plugin_sessions_layout, R.drawable.action_session);
	}

  private AdapterView.OnItemLongClickListener longClickListener = new AdapterView.OnItemLongClickListener() {
    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
      final Session s = mAdapter.getItem(position);
      final ArrayList<Integer> availableChoices = new ArrayList<Integer>();
      availableChoices.add(R.string.show_full_description);
      if(s.haveShell())
        availableChoices.add(R.string.open_shell);
      if(s.isMeterpreter()) {
        if(System.getCurrentTarget().getDeviceOS().toLowerCase().contains("windows"))
          availableChoices.add(R.string.clear_event_log);
      }
      availableChoices.add(R.string.delete);

      new ListChoiceDialog(R.string.choose_an_option,availableChoices.toArray(new Integer[availableChoices.size()]),Sessions.this, new ChoiceDialog.ChoiceDialogListener() {
        @Override
        public void onChoice(int choice) {
          switch (availableChoices.get(choice)) {
            case R.string.open_shell:
              System.setCurrentSession(s);
              startActivity(new Intent(Sessions.this, Console.class));
              overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
              break;
            case R.string.show_full_description:
              String message = s.getDescription();
              if(s.getInfo().length()>0)
                message+= "\n\nInfo:\n"+s.getInfo();
              new ErrorDialog(s.getName(),message,Sessions.this).show();
              break;
            case R.string.clear_event_log:
              boolean succeeded = false;
              int exitCode = -1;
              try {
                exitCode = ((ShellSession)s).runCommand("clearev");
                succeeded = true;
              } catch (InterruptedException e) {
                System.errorLogging(e);
              } catch (IOException e) {
                System.errorLogging(e);
              } catch (TimeoutException e) {
                System.errorLogging(e);
              } catch (RPCClient.MSFException e) {
                System.errorLogging(e);
              }
              if(!succeeded)
                Toast.makeText(Sessions.this,"command failed",Toast.LENGTH_SHORT).show();
              else if(exitCode != 0)
                Toast.makeText(Sessions.this,"command returned "+exitCode,Toast.LENGTH_LONG).show();
              break;
            case R.string.delete:
              s.stopSession();
              System.getCurrentTarget().getSessions().remove(s);
              mAdapter.notifyDataSetChanged();
              break;
          }
        }
      }).show();
      return true;
    }
  };

  private AdapterView.OnItemClickListener clickListener = new AdapterView.OnItemClickListener() {
    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
      Session s = mAdapter.getItem(position);
      if(s.haveShell()) {
        System.setCurrentSession(s);
        startActivity(new Intent(Sessions.this,Console.class));
        overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
      } else {
        longClickListener.onItemLongClick(parent, view, position, id);
      }
    }
  };

	@Override
	public void onCreate(Bundle savedInstanceState) {
		SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
		if (isDark)
			setTheme(R.style.Sherlock___Theme);
		else
			setTheme(R.style.AppTheme);
		super.onCreate(savedInstanceState);

    UIThread = this;

    if(System.getMsfRpc()==null) {
      new FinishDialog(getString(R.string.error),"MSF RPC not connected",Sessions.this).show();
      return;
		}

    System.getMsfRpc().updateSessions();

    mResults = System.getCurrentTarget().getSessions();

    if(mResults.isEmpty()) {
      new FinishDialog(getString(R.string.warning),getString(R.string.no_opened_sessions),Sessions.this).show();
      return;
    }

		mListView = (ListView) findViewById(android.R.id.list);
    mAdapter  = new ArrayAdapter<Session>(this, android.R.layout.simple_list_item_1, mResults);

		mListView.setAdapter(mAdapter);

    mListView.setOnItemClickListener(clickListener);

    mListView.setOnItemLongClickListener(longClickListener);
	}

	@Override
  public void onRpcChange(RPCClient currentValue) {
    if(UIThread==null)
      return;
    if(this!=UIThread)
      UIThread.onRpcChange(currentValue);
    else if(currentValue == null)
      new FinishDialog(getString(R.string.error),getString(R.string.msfrpc_disconnected),Sessions.this).show();
	}
}