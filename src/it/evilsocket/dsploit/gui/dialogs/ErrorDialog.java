package it.evilsocket.dsploit.gui.dialogs;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;

public class ErrorDialog extends AlertDialog 
{
	public ErrorDialog( String title, String message, final Activity activity ){
		super( activity );
		
		this.setTitle( title );
		this.setMessage( message );
		this.setCancelable( false );
		this.setButton( "Ok", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            	dialog.dismiss();
            }
        });			
	}
}
