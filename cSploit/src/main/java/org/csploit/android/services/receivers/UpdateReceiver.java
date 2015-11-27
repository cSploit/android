package org.csploit.android.services.receivers;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.support.annotation.StringRes;

import org.csploit.android.R;
import org.csploit.android.core.*;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.FinishDialog;
import org.csploit.android.services.Services;
import org.csploit.android.services.UpdateInstaller;
import org.csploit.android.update.CoreUpdate;
import org.csploit.android.update.MsfUpdate;
import org.csploit.android.update.RubyUpdate;
import org.csploit.android.update.Update;

import static org.csploit.android.services.UpdateChecker.UPDATE_AVAILABLE;
import static org.csploit.android.services.UpdateChecker.UPDATE_CHECKING;
import static org.csploit.android.services.UpdateChecker.UPDATE_NOT_AVAILABLE;
import static org.csploit.android.services.UpdateInstaller.DONE;
import static org.csploit.android.services.UpdateInstaller.ERROR;
import static org.csploit.android.services.UpdateInstaller.MESSAGE;
import static org.csploit.android.services.UpdateInstaller.UPDATE;

/**
 * receive update messages from services and acts accordingly
 */
public class UpdateReceiver extends ManagedReceiver {
  private IntentFilter filter = null;
  private final Activity activity;
  private final CoreUpdateListener listener;

  public UpdateReceiver(Activity activity) {
    if(!(activity instanceof CoreUpdateListener)) {
      throw new RuntimeException(activity + " must implement CoreUpdateListener");
    }

    this.activity = activity;
    listener = (CoreUpdateListener) activity;

    filter = new IntentFilter();

    filter.addAction(UPDATE_CHECKING);
    filter.addAction(UPDATE_AVAILABLE);
    filter.addAction(UPDATE_NOT_AVAILABLE);
    filter.addAction(ERROR);
    filter.addAction(DONE);
  }

  public IntentFilter getFilter() {
    return filter;
  }

  private String getString(@StringRes int stringId) {
    return activity.getString(stringId);
  }

  private boolean isUpdateMandatory(Update update) {
    return update instanceof CoreUpdate && !System.isCoreInstalled();
  }

  private void onUpdateAvailable(final Update update) {
    new ConfirmDialog(getString(R.string.update_available),
            update.prompt, activity, new ConfirmDialog.ConfirmDialogListener() {
      @Override
      public void onConfirm() {
        Intent i = new Intent(activity, UpdateInstaller.class);
        i.setAction(UpdateInstaller.START);
        i.putExtra(UPDATE, update);

        activity.startService(i);
      }

      @Override
      public void onCancel() {
        if (isUpdateMandatory(update)) {
          onFatal(getString(R.string.mandatory_update));
        }
      }
    }).show();
  }

  private void onUpdateDone(Update update) {
    System.reloadTools();

    if ((update instanceof MsfUpdate) || (update instanceof RubyUpdate)) {
      Services.getMsfRpcdService().stop();
      Services.getMsfRpcdService().start();
    }

    if (update instanceof CoreUpdate) {
      System.onCoreInstalled();
      listener.onCoreUpdated();
    }

    // restart update checker after a successful update
    Services.getUpdateService().start();
  }

  private void onUpdateError(final Update update, final int message) {
    if (update instanceof CoreUpdate && !System.isCoreInstalled()) {
      onFatal(getString(message));
      return;
    }

    new ErrorDialog(getString(R.string.error), getString(message), activity).show();

    System.reloadTools();
  }

  private void onFatal(String message) {
    new FinishDialog(getString(R.string.initialization_error), message, activity).show();
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    String action = intent.getAction();
    Update update = null;

    if (intent.hasExtra(UPDATE)) {
      update = (Update) intent.getSerializableExtra(UPDATE);
    }

    switch (action) {
      case UPDATE_NOT_AVAILABLE:
        if (!System.isCoreInstalled()) {
          onFatal(getString(R.string.no_core_found));
        }
        break;
      case UPDATE_AVAILABLE:
        onUpdateAvailable(update);
        break;
      case DONE:
        onUpdateDone(update);
        break;
      case ERROR:
        int message = intent.getIntExtra(MESSAGE, R.string.error_occured);
        onUpdateError(update, message);
        break;
    }
  }

  public interface CoreUpdateListener {
    void onCoreUpdated();
  }
}
