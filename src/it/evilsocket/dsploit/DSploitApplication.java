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
package it.evilsocket.dsploit;

import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.plugins.ExploitFinder;
import it.evilsocket.dsploit.plugins.Inspector;
import it.evilsocket.dsploit.plugins.LoginCracker;
import it.evilsocket.dsploit.plugins.PacketForger;
import it.evilsocket.dsploit.plugins.PortScanner;
import it.evilsocket.dsploit.plugins.mitm.MITM;

import org.acra.*;
import org.acra.annotation.*;

import android.app.Application;

@ReportsCrashes
(
	formKey 				   = "dDNSaEdZRzVSV3RHaUpGSzlTdzJiSkE6MQ",
	mode 					   = ReportingInteractionMode.TOAST,
	forceCloseDialogAfterToast = false,
	resToastText 			   = R.string.crash_toast_text
) 
public class DSploitApplication extends Application 
{
	@Override
	public void onCreate() {
		ACRA.init( this );

		// initialize the system
		try
		{
			System.init( this );
			
	        System.registerPlugin( new PortScanner( ) );
	        System.registerPlugin( new Inspector( ) );
	        System.registerPlugin( new ExploitFinder( ) );
	        System.registerPlugin( new LoginCracker( ) );
	        System.registerPlugin( new MITM( ) );
	        System.registerPlugin( new PacketForger( ) );
		}
		catch( Exception e )
		{
			System.errorLogging( "DSPLOIT", e );
		}
		        
		super.onCreate();
	}
}
