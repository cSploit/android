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

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputType;
import android.widget.EditText;

public class WifiCrackDialog extends AlertDialog 
{
	private EditText mEditText = null;
	
	public interface WifiCrackDialogListener
	{
		public void onManualConnect( String key );
		public void onCrack( );
	}

	public WifiCrackDialog( String title, String message, Activity activity, WifiCrackDialogListener wifiCrackDialogListener ){
		super( activity );
		
		mEditText = new EditText( activity );
		mEditText.setEnabled( true );
		mEditText.setInputType( InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD );
				
		this.setTitle( title );
		this.setMessage( message );
		this.setView( mEditText );
		
		final WifiCrackDialogListener listener = wifiCrackDialogListener;
		
		this.setButton( "Connect", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            	if( listener != null )
            		listener.onManualConnect( mEditText.getText().toString() );
            }
        });			
		
		this.setButton2( "Crack", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            	if( listener != null )
            		listener.onCrack( );
            }
        });	
				
		this.setButton3( "Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            	dialog.dismiss();
            }
        });		
	}
}