package it.evilsocket.dsploit.gui.dialogs;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.widget.EditText;

public class InputDialog extends AlertDialog 
{
	private EditText mEditText = null;
	
	public interface InputDialogListener
	{
		public void onInputEntered( String input );
	}
	
	public InputDialog( String title, String message, Activity activity, InputDialogListener inputDialogListener ){
		super( activity );
		
		mEditText = new EditText( activity );
		
		this.setTitle( title );
		this.setMessage( message );
		this.setView( mEditText );
		
		final InputDialogListener listener = inputDialogListener;
		
		this.setButton( "Ok", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
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
