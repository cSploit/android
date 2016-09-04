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

import android.content.Context;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.MenuItem;

import org.csploit.android.R;
import org.csploit.android.net.Target;
import org.csploit.android.net.Target.Exploit;
import org.csploit.android.net.Target.Port;
import org.csploit.android.net.metasploit.RPCClient;

import java.util.Arrays;

public abstract class Plugin extends AppCompatActivity {
  public static final int NO_LAYOUT = -1;

  private int mNameStringId = -1;
  private int mDescriptionStringId = -1;
  private Target.Type[] mAllowedTargetTypes = null;
  private int mLayoutId = 0;
  private int mIconId = 0;
  protected Child mProcess = null;

  public Plugin(int nameStringId, int descStringId, Target.Type[] allowedTargetTypes, int layoutId, int iconResourceId){
    mNameStringId = nameStringId;
    mDescriptionStringId = descStringId;

    mAllowedTargetTypes = Arrays.copyOf(allowedTargetTypes, allowedTargetTypes.length);
    mLayoutId = layoutId;
    mIconId = iconResourceId;
  }

  public Plugin(int nameStringId, int descStringId, Target.Type[] allowedTargetTypes, int layoutId){
    this(nameStringId, descStringId, allowedTargetTypes, layoutId, R.drawable.action_plugin);
  }

  public int getName(){
    return mNameStringId;
  }

  public int getDescription(){
    return mDescriptionStringId;
  }

  public Target.Type[] getAllowedTargetTypes(){
    return Arrays.copyOf(mAllowedTargetTypes, mAllowedTargetTypes.length);
  }

  public int getIconResourceId(){
    return mIconId;
  }

  public boolean isAllowedTarget(Target target){
    for(Target.Type type : mAllowedTargetTypes)
      if(type == target.getType())
        return true;

    return false;
  }

  public boolean hasLayoutToShow(){
    return mLayoutId != -1;
  }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    public void onActionClick(Context context){

  }

  @Override
  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    setTitle(System.getCurrentTarget() + " > " + getString( mNameStringId ) );
    setContentView(mLayoutId);
    getSupportActionBar().setDisplayHomeAsUpEnabled(true);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    switch(item.getItemId()){
      case android.R.id.home:

        onBackPressed();

        return true;

      default:
        return super.onOptionsItemSelected(item);
    }
  }

  @Override
  public void onBackPressed(){
    super.onBackPressed();
    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
  }

  public void onRpcChange(RPCClient currentValue) { }
}
