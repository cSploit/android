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
package it.evilsocket.dsploit.tools;

import android.content.Context;

import java.io.File;
import java.io.IOException;

import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.core.System;

public class Tool {
    private static final String TAG = "Tool";

    protected File mFile = null;
    protected String mName = null;
    protected String mDirName = null;
    protected String mFileName = null;
    protected String mLibPath = null;
    protected Context mAppContext = null;

    @SuppressWarnings("ConstantConditions")
    public Tool(String name, Context context) {
        mAppContext = context;
        mLibPath = mAppContext.getFilesDir().getAbsolutePath() + "/tools/libs";
        mFileName = mAppContext.getFilesDir().getAbsolutePath() + "/tools/" + name;
        mFile = new File(mFileName);
        mName = mFile.getName();
        mDirName = mFile.getParent();
    }

    public Tool(String name) {
        mName = name;
    }

    public void run(String args, OutputReceiver receiver) throws IOException, InterruptedException {
        String cmd = null;

        if (mAppContext != null)
            cmd = "cd " + mDirName + " && ./" + mName + " " + args;
        else
            cmd = mName + " " + args;

        Shell.exec(cmd, receiver);
    }

    public void run(String args) throws IOException, InterruptedException {
        run(args, null);
    }

    public Thread async(String args, OutputReceiver receiver) {
        String cmd = null;

        if (mAppContext != null)
            cmd = "cd " + mDirName + " && ./" + mName + " " + args;
        else
            cmd = mName + " " + args;

        return Shell.async(cmd, receiver);
    }

    public Thread asyncStatic(String args, OutputReceiver receiver) {
        String cmd = null;

        if (mAppContext != null)
            cmd = "cd " + mDirName + " && ./" + mName + " " + args;
        else
            cmd = mName + " " + args;

        return Shell.async(cmd, receiver, false);
    }

    public boolean kill() {
        return kill("9");
    }

    public boolean kill(String signal) {
        try {
            Shell.exec("killall -" + signal + " " + mName);

            return true;
        } catch (Exception e) {
            System.errorLogging(TAG, e);
        }

        return false;
    }
}
