package org.csploit.android.gui.fragments;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import org.csploit.android.R;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.gui.adapters.PluginListAdapter;
import org.csploit.android.net.Target;

import java.util.ArrayList;

public class PluginList extends BaseFragment {

  private final static String TARGET_ID = "target_id";

  private ArrayList<Plugin> mAvailable = null;
  private ListView theList;
  private OnFragmentInteractionListener listener = null;

  public static PluginList newInstance(Target target) {
    PluginList fragment = new PluginList();
    setup(fragment, target);
    return fragment;
  }

  public static void setup(PluginList fragment, Target target) {
    Bundle args = new Bundle();
    args.putString(TARGET_ID, target.getIdentifier());
    fragment.setArguments(args);
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    if(getArguments() != null) {
      String id = getArguments().getString(TARGET_ID);
      Target target = System.getTargetById(id);
      mAvailable = System.getPluginsForTarget(target);
    }
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setHasOptionsMenu(true);
    return inflater.inflate(R.layout.actions_layout, container, false);
  }

  @Override
  public void onViewCreated(View v, Bundle savedInstanceState) {
    super.onViewCreated(v, savedInstanceState);

    theList = (ListView) v.findViewById(R.id.android_list);

    PluginListAdapter adapter = new PluginListAdapter(getContext(), mAvailable);

    theList.setAdapter(adapter);
    theList.setOnItemClickListener(new ListView.OnItemClickListener() {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        if (listener != null) {
          Plugin plugin = mAvailable.get(position);
          listener.onPluginChosen(plugin);
        }
      }
    });
  }

  @Override
  public void onAttach(Context context) {
    super.onAttach(context);
    if (context instanceof OnFragmentInteractionListener) {
      listener = (OnFragmentInteractionListener) context;
    } else {
      throw new RuntimeException(context.toString()
              + " must implement OnFragmentInteractionListener");
    }
  }

  @Override
  public void onDetach() {
    super.onDetach();
    listener = null;
  }

  public interface OnFragmentInteractionListener {
    void onPluginChosen(Plugin plugin);
  }
}