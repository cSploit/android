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
package org.csploit.android.core;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

public abstract class ManagedReceiver extends BroadcastReceiver{
  private boolean mRegistered = false;
  private Context mContext = null;

  @Override
  public void onReceive(Context context, Intent intent){
  }

  public void unregister(){
    if(mRegistered && mContext != null){
      mContext.unregisterReceiver(this);
      mRegistered = false;
      mContext = null;
    }
  }

  public void register(Context context){
    if(mRegistered)
      unregister();

    context.registerReceiver(this, getFilter());
    mRegistered = true;
    mContext = context;
  }

  public abstract IntentFilter getFilter();

  protected void finalize() throws Throwable{
    unregister();
    super.finalize();
  }
}
