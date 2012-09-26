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
package it.evilsocket.dsploit.core;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class UpdateService extends Service
{
	private static final String TAG = "UPDATESERVICE";
	
	public static final String UPDATE_AVAILABLE  = "UpdateService.action.UPDATE_AVAILABLE";
	public static final String AVAILABLE_VERSION = "UpdateService.data.AVAILABLE_VERSION";

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}
	
	@Override
	public void onCreate() { }
	
	@Override
    public int onStartCommand( Intent intent, int flags, int startId ) {
    	super.onStartCommand(intent, flags, startId);
    	
    	Log.d( TAG, "Service started." );
    	
    	if( System.getUpdateManager().isUpdateAvailable() )
    	{
    		Intent update = new Intent( UPDATE_AVAILABLE );
    		
    		update.putExtra( AVAILABLE_VERSION, System.getUpdateManager().getRemoteVersion() );

    		sendBroadcast( update );    
    	}
    	
    	Log.d( TAG, "Service stopped." );
    	
    	return START_NOT_STICKY;
	}
}
