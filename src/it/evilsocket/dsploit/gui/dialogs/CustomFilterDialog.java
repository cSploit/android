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
package it.evilsocket.dsploit.gui.dialogs;

import it.evilsocket.dsploit.R;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

public class CustomFilterDialog extends AlertDialog
{
	public interface CustomFilterDialogListener
	{
		public void onInputEntered( String from, String to );
	}
	
	public CustomFilterDialog( String title, Activity activity, final CustomFilterDialogListener listener ){
		super( activity );
		
		final View view = LayoutInflater.from( activity ).inflate( R.layout.plugin_mitm_filter_dialog, null );
		
		this.setTitle( title );
		this.setView( view );

		this.setButton( "Ok", new DialogInterface.OnClickListener() {
            public void onClick( DialogInterface dialog, int id ) {
            	String from = ( ( EditText )view.findViewById( R.id.fromText ) ).getText().toString();
				String to   = ( ( EditText )view.findViewById( R.id.toText ) ).getText().toString();
				
				listener.onInputEntered( from, to );
            }
        });			
		
		this.setButton2( "Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            	dialog.dismiss();
            }
        });	
	}
}
