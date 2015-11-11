
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
package org.csploit.android;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import org.csploit.android.gui.dialogs.AboutDialog;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {

    MainFragment mMainFragment;
    ActionBarDrawerToggle mDrawerToggle;
    DrawerLayout mDrawerLayout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
        if (themePrefs.getBoolean("isDark", false))
            setTheme(R.style.DarkTheme);
        else
            setTheme(R.style.AppTheme);
        setContentView(R.layout.main);
        if (findViewById(R.id.mainframe) != null) {
            if (savedInstanceState != null) {
                return;
            }
            mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
            ListView mListView = (ListView) findViewById(R.id.drawer_listview);
            String[] mString = getResources().getStringArray(R.array.sidebar_item_array);
            TypedArray mTypedArray = getResources().obtainTypedArray(R.array.sidebar_icon_array);
            ArrayList<DrawerItem> mArrayList = new ArrayList<>();
            // load up the drawer with options from the array
            for (int x = 0; x < mString.length; x++) {
                mArrayList.add(new DrawerItem(mTypedArray.getResourceId(x, -1), mString[x]));
            }
            mTypedArray.recycle();
            mListView.setAdapter(new SideBarArrayAdapter(this,
                    R.layout.main_drawer_item, mArrayList));
            mListView.setOnItemClickListener(new DrawerItemClickListener());
            mDrawerToggle = new ActionBarDrawerToggle(this,
                    mDrawerLayout, R.string.drawer_was_opened, R.string.drawer_was_closed);
            mDrawerLayout.setDrawerListener(mDrawerToggle);
            mDrawerToggle.syncState();
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setHomeButtonEnabled(true);

            mMainFragment = new MainFragment();
            getSupportFragmentManager().beginTransaction()
                    .add(R.id.mainframe, mMainFragment).commit();
        }
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        if (mDrawerToggle.onOptionsItemSelected(item)) {
            return true;
        }
        return true;
    }

    public void onBackPressed() {
        mMainFragment.onBackPressed();
    }

    public void launchSettings() {
        startActivity(new Intent(this, SettingsActivity.class));
        overridePendingTransition(R.anim.fadeout, R.anim.fadein);
    }

    public void launchAbout() {
        new AboutDialog(this).show();
    }

    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);
        // Sync the toggle state after onRestoreInstanceState has occurred.
        mDrawerToggle.syncState();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        mDrawerToggle.onConfigurationChanged(newConfig);
    }

    public class DrawerItem {
        public int icon;
        public String name;

        public DrawerItem(int icon, String name) {
            this.icon = icon;
            this.name = name;
        }
    }

    private class DrawerItemClickListener implements ListView.OnItemClickListener {
        @Override
        public void onItemClick(AdapterView parent, View view, int position, long id) {
            mDrawerLayout.closeDrawers();
            switch (position) {
                case 0: //about
                    launchAbout();
                    break;
                case 1:
                    launchSettings();
                    break;
                case 2:
                    String uri = getString(R.string.github_new_issue_url);
                    Intent browser = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
                    startActivity(browser);
                    break;
            }
        }
    }

    public class SideBarArrayAdapter extends ArrayAdapter<DrawerItem> {

        private final Context context;
        private final int layoutResourceId;
        private ArrayList<DrawerItem> data = null;

        public SideBarArrayAdapter(Context context, int layoutResourceId, ArrayList<DrawerItem> data) {
            super(context, layoutResourceId, data);
            this.context = context;
            this.layoutResourceId = layoutResourceId;
            this.data = data;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            LayoutInflater inflater = ((Activity) context).getLayoutInflater();
            View v = inflater.inflate(layoutResourceId, parent, false);

            ImageView imageView = (ImageView) v.findViewById(R.id.drawer_item_icon);
            TextView textView = (TextView) v.findViewById(R.id.drawer_item_title);

            DrawerItem item = data.get(position);

            imageView.setImageResource(item.icon);
            textView.setText(item.name);

            return v;
        }
    }


}