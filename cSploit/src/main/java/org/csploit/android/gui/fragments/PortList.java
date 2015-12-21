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
import org.csploit.android.core.System;
import org.csploit.android.gui.adapters.PortListAdapter;
import org.csploit.android.net.Target;

/**
 * Show a list of ports detected on a target
 */
public class PortList extends BaseListFragment {

  private final static String TARGET_ID = "target_id";

  private OnFragmentInteractionListener listener = null;
  private Target target;

  public static PortList newInstance(Target target) {
    PortList fragment = new PortList();
    setup(fragment, target);
    return fragment;
  }

  public static void setup(PortList fragment, Target target) {
    Bundle args = new Bundle();
    args.putString(TARGET_ID, target.getIdentifier());
    fragment.setArguments(args);
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    if(getArguments() != null) {
      String id = getArguments().getString(TARGET_ID);
      target = System.getTargetById(id);
    }
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setHasOptionsMenu(true);
    return inflater.inflate(R.layout.port_list, container, false);
  }

  @Override
  public void onViewCreated(View v, Bundle savedInstanceState) {
    super.onViewCreated(v, savedInstanceState);

    ListView lv = getListView();

    final PortListAdapter adapter = new PortListAdapter(getActivity(), target);

    lv.setAdapter(adapter);

    adapter.setListView(lv);

    lv.setOnItemClickListener(new ListView.OnItemClickListener() {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        if (listener != null) {
          Target.Port port = (Target.Port) adapter.getItem(position);
          listener.onPortChosen(port);
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
    void onPortChosen(Target.Port port);
  }

}
