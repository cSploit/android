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

import java.util.ArrayList;

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
		public void onInputEntered( ArrayList<String> from, ArrayList<String> to );
	}
	
	public CustomFilterDialog( String title, Activity activity, final CustomFilterDialogListener listener ){
		super( activity );
		
		final View view = LayoutInflater.from( activity ).inflate( R.layout.plugin_mitm_filter_dialog, null );
		
		this.setTitle( title );
		this.setView( view );

		this.setButton( "Ok", new DialogInterface.OnClickListener() {
            public void onClick( DialogInterface dialog, int id ) {
				String f0 = ( ( EditText )view.findViewById( R.id.fromText0 ) ).getText().toString().trim(),
					   f1 = ( ( EditText )view.findViewById( R.id.fromText1 ) ).getText().toString().trim(),
					   f2 = ( ( EditText )view.findViewById( R.id.fromText2 ) ).getText().toString().trim(),
					   f3 = ( ( EditText )view.findViewById( R.id.fromText3 ) ).getText().toString().trim(),
					   f4 = ( ( EditText )view.findViewById( R.id.fromText4 ) ).getText().toString().trim(),
					   t0 = ( ( EditText )view.findViewById( R.id.toText0 ) ).getText().toString().trim(),
					   t1 = ( ( EditText )view.findViewById( R.id.toText1 ) ).getText().toString().trim(),
					   t2 = ( ( EditText )view.findViewById( R.id.toText2 ) ).getText().toString().trim(),
					   t3 = ( ( EditText )view.findViewById( R.id.toText3 ) ).getText().toString().trim(),
					   t4 = ( ( EditText )view.findViewById( R.id.toText4 ) ).getText().toString().trim();
				
				ArrayList<String> from = new ArrayList<String>(),
								  to   = new ArrayList<String>();
				
				if( f0.isEmpty() == false && t0.isEmpty() == false )
				{
					from.add( f0 );
					to.add( t0 );
				}
				
				if( f1.isEmpty() == false && t1.isEmpty() == false )
				{
					from.add( f1 );
					to.add( t1 );
				}

				if( f2.isEmpty() == false && t2.isEmpty() == false )
				{
					from.add( f2 );
					to.add( t2 );
				}
				
				if( f3.isEmpty() == false && t3.isEmpty() == false )
				{
					from.add( f3 );
					to.add( t3 );
				}
				
				if( f4.isEmpty() == false && t4.isEmpty() == false )
				{
					from.add( f4 );
					to.add( t4 );
				}
				
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
