package org.csploit.android.gui.fragments;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.csploit.android.R;
import org.csploit.android.core.ManagedReceiver;
import org.csploit.android.helpers.NetworkHelper;
import org.csploit.android.services.Services;
import org.csploit.android.services.UpdateChecker;

import java.util.HashMap;
import java.util.Map;

/**
 * A fragment to show that the core is missing
 */
public class Heartless extends BaseFragment {

  private TextView textView;
  private String baseText;
  private UpdateReceiver receiver;

  @Override
  public void onDetach() {
    super.onDetach();
    if(receiver != null) {
      receiver.unregister();
    }
    receiver = null;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container,
                           Bundle savedInstanceState) {
    return inflater.inflate(R.layout.heartless, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    textView = (TextView) view.findViewById(android.R.id.text1);


    if(!NetworkHelper.isConnectivityAvailable(getContext())) {
      textView.setText(getString(R.string.no_core_no_connectivity));
      return;
    }

    baseText = getString(R.string.missing_core_update);

    setStatus("");

    receiver = new UpdateReceiver();

    receiver.register(getContext());

    Services.getUpdateService().start();
  }

  private void setStatus(@StringRes int status) {
    setStatus(getString(status));
  }

  private void setStatus(String s) {
    textView.setText(baseText.replace("#STATUS#", s));
  }

  private class UpdateReceiver extends ManagedReceiver {

    private final Map<String, Integer> action2text = new HashMap<String, Integer>() {{
      put(UpdateChecker.UPDATE_CHECKING, R.string.checking);
      put(UpdateChecker.UPDATE_AVAILABLE, R.string.update_available);
      put(UpdateChecker.UPDATE_NOT_AVAILABLE, R.string.no_updates_available);
    }};

    private final IntentFilter filter;

    public UpdateReceiver() {
      filter = new IntentFilter();
      for(String action : action2text.keySet()) {
        filter.addAction(action);
      }
    }

    @Override
    public IntentFilter getFilter() {
      return filter;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
      String action = intent.getAction();
      Integer stringRes = action2text.get(action);
      setStatus(stringRes);
    }
  }
}
