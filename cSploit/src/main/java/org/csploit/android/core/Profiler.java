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

// a little class to profile network latency ^_^
public class Profiler{
  private static volatile Profiler mInstance = null;

  public static Profiler instance(){
    if(mInstance == null)
      mInstance = new Profiler();

    return mInstance;
  }

  private volatile boolean mEnabled = false;
  private volatile long mTick = 0;
  private volatile String mProfiling = null;

  public Profiler(){
    mEnabled = System.getSettings().getBoolean("PREF_ENABLE_PROFILER", false);
  }

  private String format(long delta){
    if(delta < 1000)
      return delta + " ms";

    else if(delta < 60000)
      return (delta / 1000.0) + " s";

    else
      return (delta / 60000.0) + " m";
  }

  public void emit(){
    if(mEnabled && mTick > 0 && mProfiling != null){
      long delta = java.lang.System.currentTimeMillis() - mTick;

      Logger.debug("[" + mProfiling + "] " + format(delta));

      mProfiling = null;
      mTick = 0;
    }
  }

  public void profile(String label){
    emit();
    if(mEnabled){
      mTick = java.lang.System.currentTimeMillis();
      mProfiling = label;
    }
  }
}
