package org.csploit.android.gui.fragments;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import org.csploit.android.R;
import org.csploit.android.gui.adapters.TargetListAdapter;
import org.csploit.android.net.Target;
import org.csploit.android.services.Services;
import org.csploit.android.services.receivers.NetworkRadarReceiver;

/**
 * A list fragment representing a list of Items. This fragment
 * also supports tablet devices by allowing list items to be given an
 * 'activated' state upon selection. This helps indicate which item is
 * currently being viewed in a {@link TargetDetail}.
 * <p>
 * Activities containing this fragment MUST implement the {@link OnFragmentInteraction}
 * interface.
 */
public class TargetList extends BaseListFragment {

  /**
   * The serialization (saved instance state) Bundle key representing the
   * activated item position. Only used on tablets.
   */
  private static final String STATE_ACTIVATED_TARGET = "activated_target";

  /**
   * The fragment's current callback object, which is notified of list item
   * clicks.
   */
  private OnFragmentInteraction listener = null;

  /**
   * The current activated item position. Only used on tablets.
   */
  private int mActivatedPosition = ListView.INVALID_POSITION;

  private NetworkRadarReceiver radarReceiver;

  /**
   * Mandatory empty constructor for the fragment manager to instantiate the
   * fragment (e.g. upon screen orientation changes).
   */
  public TargetList() { }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    radarReceiver = new NetworkRadarReceiver();

    radarReceiver.register(getActivity());

    TargetListAdapter adapter = new TargetListAdapter(getActivity());
    setListAdapter(adapter);
  }

  @Override
  public void onDestroy() {
    if(radarReceiver != null) {
      radarReceiver.unregister();
    }
    radarReceiver = null;
    super.onDestroy();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setHasOptionsMenu(true);
    return inflater.inflate(R.layout.target_list, container, false);
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
    inflater.inflate(R.menu.main, menu);
    super.onCreateOptionsMenu(menu, inflater);
  }

  @Override
  public void onPrepareOptionsMenu(Menu menu) {

    MenuItem item = menu.findItem(R.id.ss_monitor);

    Services.getNetworkRadar().buildMenuItem(item);

    super.onPrepareOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    if(item.getItemId() == R.id.ss_monitor) {
      Services.getNetworkRadar().onMenuClick(getActivity(), item);
      return true;
    }

    return super.onOptionsItemSelected(item);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    TargetListAdapter adapter = (TargetListAdapter) getListAdapter();
    ListView lv = getListView();

    adapter.setListView(lv);

    if (savedInstanceState != null
            && savedInstanceState.containsKey(STATE_ACTIVATED_TARGET)) {
      String id = savedInstanceState.getString(STATE_ACTIVATED_TARGET);
      int pos = adapter.indexOf(id);
      setActivatedPosition(pos >= 0 ? pos : ListView.INVALID_POSITION);
    }
  }

  @Override
  public void onAttach(Context context) {
    super.onAttach(context);

    // Activities containing this fragment must implement its callbacks.
    if (!(context instanceof OnFragmentInteraction)) {
      throw new RuntimeException(context + " must implement fragment's callbacks.");
    }

    listener = (OnFragmentInteraction) context;
  }

  @Override
  public void onDetach() {
    super.onDetach();

    listener = null;
  }

  @Override
  public void onListItemClick(ListView listView, View view, int position, long id) {
    super.onListItemClick(listView, view, position, id);

    // Notify the active callbacks interface (the activity, if the
    // fragment is attached to one) that an item has been selected.
    if(listener != null) {
      Target target = (Target) getListAdapter().getItem(position);
      listener.onTargetSelected(target);
    }
  }

  @Override
  public void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);
    if (mActivatedPosition != ListView.INVALID_POSITION) {
      // Serialize and persist the activated item position.
      Target target = (Target) getListAdapter().getItem(mActivatedPosition);
      outState.putString(STATE_ACTIVATED_TARGET, target.getIdentifier());
    }
  }

  /**
   * Turns on activate-on-click mode. When this mode is on, list items will be
   * given the 'activated' state when touched.
   */
  public void setActivateOnItemClick(boolean activateOnItemClick) {
    // When setting CHOICE_MODE_SINGLE, ListView will automatically
    // give items the 'activated' state when touched.
    getListView().setChoiceMode(activateOnItemClick
            ? ListView.CHOICE_MODE_SINGLE
            : ListView.CHOICE_MODE_NONE);
  }

  private void setActivatedPosition(int position) {
    if (position == ListView.INVALID_POSITION) {
      getListView().setItemChecked(mActivatedPosition, false);
    } else {
      getListView().setItemChecked(position, true);
    }

    mActivatedPosition = position;
  }

  /**
   * A callback interface that all activities containing this fragment must
   * implement. This mechanism allows activities to be notified of item
   * selections.
   */
  public interface OnFragmentInteraction {
    /**
     * Callback for when an item has been selected.
     */
    void onTargetSelected(Target target);
    /**
     * TODO: long click + action mode
     */
    //void onTargetLongClicked(String id);
  }
}
