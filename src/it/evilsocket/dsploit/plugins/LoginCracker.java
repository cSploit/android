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
package it.evilsocket.dsploit.plugins;

import java.util.ArrayList;

import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.gui.dialogs.FinishDialog;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Target.Port;
import it.evilsocket.dsploit.system.Environment;
import it.evilsocket.dsploit.system.Plugin;

public class LoginCracker extends Plugin
{
	private static final String[] PROTOCOLS = new String[]
	{
		"ftp",
		"http-head",
		"http-get",
		"http-get-form",
		"http-post-form",
		"https-get-form",
		"https-post-form",
		"https-head",
		"https-get",
		"http-proxy",
		"http-proxy-ntlm",
		"icq",
		"imap",
		"imap-ntlm",
		"ldap2",
		"ldap3",
		"oracle-listener",
		"mssql",
		"mysql",
		"postgres",
		"pcanywhere",
		"nntp",
		"pcnfs",
		"pop3",
		"pop3-ntlm",
		"rexec",
		"rlogin",
		"rsh",
		"sapr3",
		"smb",
		"smbnt",
		"socks5",
		"ssh2",
		"telnet",
		"cisco",
		"cisco-enable",
		"vnc",
		"snmp",
		"cvs",
		"svn",
		"firebird",
		"afp",
		"ncp",
		"smtp-auth",
		"smtp-auth-ntlm",
		"teamspeak",
		"sip",
		"vmauthd"
	};
	
	private Spinner mPortSpinner	 = null;
	private Spinner mProtocolSpinner = null;
	
	public LoginCracker( ) {
		super
		( 
		    "Login Cracker", 
		    "A very fast network logon cracker which support many different services.", 
		    new Target.Type[]{ Target.Type.ENDPOINT, Target.Type.REMOTE }, 
		    R.layout.plugin_login_cracker,
		    R.drawable.action_login_48
	    );		
	}
	
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);   
        
        if( Environment.getTarget().hasOpenPorts() == false )
        	new FinishDialog( "Warning", "No open ports detected on current target, run the port scanner first.", this ).show();
        
        ArrayList<String> ports = new ArrayList<String>();
        
        for( Port port : Environment.getTarget().getOpenPorts() )
        	ports.add( Integer.toString( port.port ) );
        
        mPortSpinner     = ( Spinner )findViewById( R.id.portSpinner );
        mPortSpinner.setAdapter( new ArrayAdapter<String>( this, android.R.layout.simple_spinner_item, ports ) );
        mPortSpinner.setOnItemSelectedListener( new OnItemSelectedListener() {
        	public void onItemSelected( AdapterView<?> adapter, View view, int position, long id ) 
        	{
        		String port 	= ( String )adapter.getItemAtPosition( position ),
        			   protocol = Environment.getProtocolByPort( port );
        		
        		if( protocol != null )
        		{
        			for( int i = 0; i < PROTOCOLS.length; i++ )
        			{
        				if( PROTOCOLS[i].equals( protocol ) )
        				{
        					mProtocolSpinner.setSelection( i );
        					break;
        				}
        			}
        		}
        	}
        	
        	public void onNothingSelected(AdapterView<?> arg0) {}
		});
        
        mProtocolSpinner = ( Spinner )findViewById( R.id.protocolSpinner );
        mProtocolSpinner.setAdapter( new ArrayAdapter<String>( this, android.R.layout.simple_spinner_item, PROTOCOLS ) );
	}
}
