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
package org.csploit.android.plugins;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import org.csploit.android.R;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.net.Target;

public class RouterPwn extends Plugin{
  public RouterPwn(){
    super(
      R.string.router_pwn,
      R.string.router_pwn_desc,

      new Target.Type[]{Target.Type.ENDPOINT},
      Plugin.NO_LAYOUT,
      R.drawable.action_routerpwn
    );
  }

  @Override
  public boolean isAllowedTarget(Target target){
    return target.isRouter();
  }


  @Override
  public void onBackPressed() {
    super.onBackPressed();
    overridePendingTransition(R.anim.fadeout, R.anim.fadein);
  }

  @Override
  public void onActionClick(Context context){
    try{
      String uri = "http://routerpwn.com/";
      Intent browser = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));

      context.startActivity(browser);
    }
    catch(ActivityNotFoundException e){
      System.errorLogging(e);
      new ErrorDialog(getString(R.string.error), getString(R.string.no_activities_for_url), (Activity) context).show();
    }
  }
}
