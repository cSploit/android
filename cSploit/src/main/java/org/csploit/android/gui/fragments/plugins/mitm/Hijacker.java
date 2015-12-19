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
package org.csploit.android.gui.fragments.plugins.mitm;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Toast;
import android.widget.ToggleButton;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.gui.adapters.mitm.SessionListAdapter;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.InputDialog;
import org.csploit.android.gui.dialogs.InputDialog.InputDialogListener;
import org.csploit.android.gui.dialogs.SpinnerDialog;
import org.csploit.android.gui.dialogs.SpinnerDialog.SpinnerDialogListener;
import org.csploit.android.gui.fragments.BaseFragment;
import org.csploit.android.helpers.ThreadHelper;
import org.csploit.android.plugins.mitm.SpoofSession;
import org.csploit.android.plugins.mitm.SpoofSession.OnSessionReadyListener;
import org.csploit.android.plugins.mitm.hijacker.Session;

import java.io.IOException;
import java.util.ArrayList;

public class Hijacker extends BaseFragment
  implements org.csploit.android.plugins.mitm.hijacker.Hijacker.HijackerListener {
  private ToggleButton mHijackToggleButton = null;
  private ProgressBar mHijackProgress = null;
  private SessionListAdapter mAdapter = null;

  private org.csploit.android.plugins.mitm.hijacker.Hijacker hijacker;

  private OnFragmentInteractionListener listener;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.plugin_mitm_hijacker, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    mHijackToggleButton = (ToggleButton) view.findViewById(R.id.hijackToggleButton);
    mHijackProgress = (ProgressBar) view.findViewById(R.id.hijackActivity);
    ListView mListView = (ListView) view.findViewById(R.id.listView);

    //TODO: restore hijacker from bundle
    hijacker = new org.csploit.android.plugins.mitm.hijacker.Hijacker();
    mAdapter = new SessionListAdapter( getActivity(), R.layout.plugin_mitm_hijacker_list_item, hijacker);

    mListView.setAdapter(mAdapter);
    mListView.setOnItemClickListener(new OnItemClickListener() {
      @Override
      public void onItemClick(AdapterView<?> parent, View view,
                              int position, long id) {
        final Session session = hijacker.getSessions().get(position);
        if (session != null) {
          new ConfirmDialog(getString(R.string.hijack_session),
                  hijacker.isRunning() ? getString(R.string.start_hijacking)
                          : getString(R.string.start_hijacking2),
                  getActivity(), new ConfirmDialogListener() {
            @Override
            public void onConfirm() {
              if(listener == null)
                return;

              if(hijacker.isRunning())
                hijacker.stop();

              listener.onHijackerSessionSelected(session);
            }

            @Override
            public void onCancel() {
            }
          }).show();
        }
      }
    });

    mListView.setOnItemLongClickListener(new OnItemLongClickListener() {
      @Override
      public boolean onItemLongClick(AdapterView<?> parent, View view,
                                     int position, long id) {
        final Session session = hijacker.getSessions().get(position);
        if (session != null) {
          new InputDialog(getString(R.string.save_session),
                  getString(R.string.set_session_filename), session
                  .getFileName(), true, false, getActivity(),
                  new InputDialogListener() {
                    @Override
                    public void onInputEntered(String name) {
                      if (!name.isEmpty()) {

                        try {
                          String filename = session
                                  .save(name);

                          Toast.makeText(
                                  getContext(),
                                  getString(R.string.session_saved_to)
                                          + filename + " .",
                                  Toast.LENGTH_SHORT).show();
                        } catch (IOException e) {
                          new ErrorDialog(
                                  getString(R.string.error),
                                  e.toString(), getActivity())
                                  .show();
                        }
                      } else
                        new ErrorDialog(
                                getString(R.string.error),
                                getString(R.string.invalid_session),
                                getActivity()).show();
                    }
                  }).show();
        }

        return true;
      }
    });

    mHijackToggleButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        ThreadHelper.getSharedExecutor().execute(new Runnable() {
          @Override
          public void run() {
            toggleState();
          }
        });
      }
    });
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
    inflater.inflate(R.menu.hijacker, menu);
    super.onCreateOptionsMenu(menu, inflater);
  }

  private void toggleState() {
    if(hijacker.isRunning()) {
      hijacker.stop();
    } else {
      hijacker.start();
    }
  }

  @Override
  public void onSessionsChanged() { }

  @Override
  public void onStateChanged() {
    if(hijacker.isRunning()) {
      getActivity().runOnUiThread(new Runnable() {
        @Override
        public void run() {
          mHijackToggleButton.setText(R.string.stop);
          mHijackProgress.setVisibility(View.VISIBLE);
          mHijackToggleButton.setChecked(true);
        }
      });
    } else {
      getActivity().runOnUiThread(new Runnable() {
        @Override
        public void run() {
          mHijackToggleButton.setText(R.string.start);
          mHijackProgress.setVisibility(View.INVISIBLE);
          mHijackToggleButton.setChecked(false);
        }
      });
    }
  }

  @Override
  public void onError(final String message) {
    getActivity().runOnUiThread(new Runnable() {
      @Override
      public void run() {
        if (!getActivity().isFinishing()) {
          new ErrorDialog(getString(R.string.error), message,
                  getActivity()).show();
          hijacker.stop();
        }
      }
    });
  }

  @Override
  public void onError(@StringRes int stringId) {
    onError(getString(stringId));
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    int itemId = item.getItemId();

    switch (itemId) {

      case R.id.load:

        final ArrayList<String> sessions = System
                .getAvailableHijackerSessionFiles();

        if (sessions != null && sessions.size() > 0) {
          new SpinnerDialog(getString(R.string.select_session),
                  getString(R.string.select_session_file),
                  sessions.toArray(new String[sessions.size()]),
                  getActivity(), new SpinnerDialogListener() {
            @Override
            public void onItemSelected(int index) {
              String filename = sessions.get(index);

              try {
                Session session = Session.load(filename);

                if (session != null) {
                  hijacker.addSession(session);
                }
              } catch (Exception e) {
                e.printStackTrace();
                new ErrorDialog("Error", e.getMessage(),
                        getActivity()).show();
              }
            }
          }).show();
        } else
          new ErrorDialog(getString(R.string.error),
                  getString(R.string.no_session_found), getActivity())
                  .show();

        return true;

      default:
        return super.onOptionsItemSelected(item);
    }
  }

  @Override
  public void onAttach(Context context) {

    if(context instanceof OnFragmentInteractionListener) {
      listener = (OnFragmentInteractionListener) context;
    } else {
      throw new RuntimeException(context + " must implement OnFragmentInteractionListener");
    }

    super.onAttach(context);
  }

  @Override
  public void onDetach() {
    hijacker.stop();
    listener = null;
    super.onDetach();
  }

  public interface OnFragmentInteractionListener {
    void onHijackerSessionSelected(Session session);
  }
}
