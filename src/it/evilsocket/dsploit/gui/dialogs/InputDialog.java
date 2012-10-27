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

public class InputDialog extends AlertDialog 
{
	private EditText mEditText = null;
	
	public interface InputDialogListener
	{
		public void onInputEntered( String input );
	}
	
	public InputDialog( String title, String message, Activity activity, InputDialogListener inputDialogListener ){
		this( title, message, null, true, false, activity, inputDialogListener );
	}
	
	public InputDialog( String title, String message, String text, boolean editable, boolean password, Activity activity, InputDialogListener inputDialogListener ){
		super( activity );
		
		mEditText = new EditText( activity );
		
		if( text != null )
			mEditText.setText(text);
		
		mEditText.setEnabled( editable );
		
		if( password )
			mEditText.setInputType( InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD );
				
		this.setTitle( title );
		this.setMessage( message );
		this.setView( mEditText );
		
		final InputDialogListener listener = inputDialogListener;
		
		this.setButton( "Ok", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            	if( listener != null )
            		listener.onInputEntered( mEditText.getText().toString() );
            }
        });			
		
		this.setButton2( "Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            	dialog.dismiss();
            }
        });	
	}
}
