package org.csploit.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.FinishDialog;
import org.csploit.android.net.Target;

import java.util.ArrayList;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;

public class ActionFragment extends Fragment {

    private ArrayList<Plugin> mAvailable = null;

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        return inflater.inflate(R.layout.actions_layout, container, false);
    }

    @Override
    public void onViewCreated(View v, Bundle savedInstanceState) {
        SharedPreferences themePrefs = getActivity().getSharedPreferences("THEME", 0);
        Boolean isDark = themePrefs.getBoolean("isDark", false);
        if (isDark) {
            getActivity().setTheme(R.style.DarkTheme);
            v.setBackgroundColor(ContextCompat.getColor(getActivity(), R.color.background_window_dark));
        } else {
            getActivity().setTheme(R.style.AppTheme);
            v.setBackgroundColor(ContextCompat.getColor(getActivity(), R.color.background_window));
        }
        Target mTarget = System.getCurrentTarget();

        if (mTarget != null) {
            getActivity().setTitle("cSploit > " + mTarget);
            ((AppCompatActivity) getActivity()).getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            ListView theList = getActivity().findViewById(R.id.android_list);
            mAvailable = System.getPluginsForTarget();
            ActionsAdapter mActionsAdapter = new ActionsAdapter();
            theList.setAdapter(mActionsAdapter);
            theList.setOnItemClickListener((parent, view, position, id) -> {

                if (System.checkNetworking(getActivity())) {
                    Plugin plugin = mAvailable.get(position);
                    System.setCurrentPlugin(plugin);

                    if (plugin.hasLayoutToShow()) {
                        Toast.makeText(getActivity(), getString(R.string.selected) + getString(plugin.getName()), Toast.LENGTH_SHORT).show();

                        startActivity(new Intent(
                                getActivity(),
                                plugin.getClass()
                        ));
                        getActivity().overridePendingTransition(R.anim.fadeout, R.anim.fadein);
                    } else
                        plugin.onActionClick(getActivity().getApplicationContext());
                }
            });
        } else {
            new FinishDialog(getString(R.string.warning), getString(R.string.something_went_wrong), getActivity()).show();
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:

                getActivity().onBackPressed();

                return true;

            default:
                return super.onOptionsItemSelected(item);
        }
    }

    public void onBackPressed() {
        getActivity().finish();
        getActivity().overridePendingTransition(R.anim.fadeout, R.anim.fadein);
    }

    public class ActionsAdapter extends ArrayAdapter<Plugin> {
        public ActionsAdapter() {
            super(getActivity(), R.layout.actions_list_item, mAvailable);
        }

        @SuppressLint("NewApi")
        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View row = convertView;
            ActionHolder holder;

            if (row == null) {
                LayoutInflater inflater = (LayoutInflater) getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                row = inflater.inflate(R.layout.actions_list_item, parent, false);
                if (getActivity().getSharedPreferences("THEME", 0).getBoolean("isDark", false))
                    row.setBackgroundResource(R.drawable.card_background_dark);
                holder = new ActionHolder();

                holder.icon = (ImageView) (row != null ? row.findViewById(R.id.actionIcon) : null);
                holder.name = (TextView) (row != null ? row.findViewById(R.id.actionName) : null);
                holder.description = (TextView) (row != null ? row.findViewById(R.id.actionDescription) : null);
                if (row != null) row.setTag(holder);

            } else holder = (ActionHolder) row.getTag();

            Plugin action = mAvailable.get(position);

            holder.icon.setImageResource(action.getIconResourceId());
            holder.name.setText(getString(action.getName()));
            holder.description.setText(getString(action.getDescription()));

            return row;
        }

        public class ActionHolder {
            ImageView icon;
            TextView name;
            TextView description;
        }
    }
}