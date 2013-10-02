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

import java.util.ArrayList;
import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.dialogs.FinishDialog;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Target.Exploit;

public class Sessions extends Plugin
{
  //private final static String  TAG = "SESSIONS";

  private ListView 			mListView		   = null;
  private ArrayList<Exploit> results = new ArrayList<Target.Exploit>();
  private ArrayAdapter<Exploit> mAdapter = null;

  public Sessions() {
    super
    (
      R.string.sessions,
      R.string.sessions_desc,

      new Target.Type[]{ Target.Type.ENDPOINT, Target.Type.REMOTE },
      R.layout.plugin_sessions_layout,
      R.drawable.action_session
    );
  }


  private Thread mThread = new Thread( new Runnable() {
    @Override
    public void run()
    {
      //TODO: find and add sessions to mAdapted
    }
  });

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    int nEx,i;

    i=nEx=0;
    nEx = System.getCurrentTarget().getExploits().size();
    for(i=0;i<nEx;i++)
    {
      if(System.getCurrentExploits().get(i).started)
        break;
    }

    if( System.getCurrentTarget().hasOpenPorts() == false )
      new FinishDialog( "Warning", "No open ports detected on current target, run the service inspector first.", this ).show();
    else if( nEx == 0)
      new FinishDialog( "Warning", "No exploits found on this target, run the ExploitFinder first.", this ).show();
    else if(i>=nEx)
      new FinishDialog( "Warning", "No exploit has been started.", this ).show();
    mListView		   = ( ListView )findViewById( android.R.id.list );
    mAdapter		   = new ArrayAdapter<Exploit>(this, android.R.layout.simple_list_item_1, results);

    mAdapter.clear();

    mListView.setAdapter( mAdapter );

    mThread.start();

     /*TODO: open shell....how? maybe we can use android-terminal-emulator

      mListView.setOnChildClickListener( new OnChildClickListener(){
			@Override
			public boolean onChildClick( ExpandableListView parent, View v, int groupPosition, int childPosition, long id ) {
				Vulnerability cve = ( Vulnerability )mAdapter.getChild(groupPosition, childPosition);

				if( cve != null )
				{
					String uri     = "http://web.nvd.nist.gov/view/vuln/detail?vulnId=" + cve.getIdentifier();
					Intent browser = new Intent( Intent.ACTION_VIEW, Uri.parse( uri ) );

					startActivity( browser );
				}

				return true;
			}}
        );

		for( int i = 0; i < mAdapter.getGroupCount(); i++ )
		{
			mListView.expandGroup( i );
		}

		*/
  }

  @Override
  public void onBackPressed() {
    super.onBackPressed();
    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
    mThread.interrupt();
    mThread.stop();
  }
}