/*
 * This file is part of the cSploit.
 *
 * Copyleft of Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 *
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android.gui.dialogs;

import android.app.Activity;

import org.csploit.android.core.Logger;

import java.util.Timer;
import java.util.TimerTask;

public class ProgressDialog extends android.app.ProgressDialog {

  private final Activity activity;

  private long minElapsedTime = 0;
  private long minShownTime = 1000;
  private long startTime = 0;
  private TimerTask task;

  public ProgressDialog(String title, String message, final Activity activity){
    super(activity);

    this.setTitle(title);
    this.setMessage(message);
    this.setCancelable(false);

    this.activity = activity;
  }

  public void setMinElapsedTime(long minElapsedTime) {
    this.minElapsedTime = minElapsedTime;
  }

  public void setMinShownTime(long minShownTime) {
    this.minShownTime = minShownTime;
  }

  @Override
  public void show() {
    startTime = System.currentTimeMillis();
    if(minElapsedTime == 0) {
      super.show();
      return;
    }

    synchronized (this) {
      Logger.debug("creating delayed start task");
      task = new TimerTask() {
        @Override
        public void run() {
          activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
              ProgressDialog.super.show();
            }
          });
        }
      };

      new Timer().schedule(task, minElapsedTime);
    }
  }

  @Override
  public void dismiss() {

    long dismissDelay = startTime + minElapsedTime + minShownTime - System.currentTimeMillis();

    if(minShownTime == 0 || dismissDelay <= 0) {
      synchronized (this) {
        if (task != null) {
          task.cancel();
        }
      }
      super.dismiss();
    } else {
      Logger.debug("creating delayed dismiss task");
      synchronized (this) {
        if(task != null) {
          task.cancel();
        }
        task = new TimerTask() {
          @Override
          public void run() {
            ProgressDialog.super.dismiss();
          }
        };
        new Timer().schedule(task, dismissDelay);
      }
    }
  }
}

