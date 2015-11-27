package org.csploit.android.gui.fragments;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.csploit.android.R;
import org.csploit.android.core.Client;
import org.csploit.android.core.CrashReporter;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.FinishDialog;
import org.csploit.android.helpers.ThreadHelper;
import org.csploit.android.plugins.ExploitFinder;
import org.csploit.android.plugins.Inspector;
import org.csploit.android.plugins.LoginCracker;
import org.csploit.android.plugins.PacketForger;
import org.csploit.android.plugins.PortScanner;
import org.csploit.android.plugins.RouterPwn;
import org.csploit.android.plugins.Sessions;
import org.csploit.android.plugins.Traceroute;
import org.csploit.android.plugins.mitm.MITM;
import org.csploit.android.services.Services;

import java.net.NoRouteToHostException;

/**
 * Manage the initialization process
 */
public class Init extends BaseFragment implements Runnable {

  private OnFragmentInteractionListener listener;

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
    super.onDetach();
    listener = null;
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.init, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    ThreadHelper.getSharedExecutor().execute(this);
  }

  @Override
  public void run() {

    String errMsg = null;
    boolean isFatal = true;

    try {
      System.init();
      System.initCore();

      if (Client.hadCrashed()) {
        Logger.warning("Client has previously crashed, building a crash report.");
        CrashReporter.notifyNativeLibraryCrash();
        errMsg = getString(R.string.JNI_crash_detected);
      }

      // start services
      Services.getUpdateService().start();
      Services.getNetworkRadar().start();
      Services.getMsfRpcdService().start();

      // register plugins
      System.registerPlugin(new RouterPwn());
      System.registerPlugin(new Traceroute());
      System.registerPlugin(new PortScanner());
      System.registerPlugin(new Inspector());
      System.registerPlugin(new ExploitFinder());
      System.registerPlugin(new LoginCracker());
      System.registerPlugin(new Sessions());
      System.registerPlugin(new MITM());
      System.registerPlugin(new PacketForger());

      isFatal = false;
    } catch (System.SuException e) {
      errMsg = getString(R.string.only_4_root);
    } catch (System.DaemonException e) {
      errMsg = e.getMessage();
    } catch (UnsatisfiedLinkError e) {
      errMsg = "hi developer, you missed to build JNI stuff, thanks for playing with me :)";
    } catch (Exception e) {
      isFatal = !(e instanceof NoRouteToHostException);
      errMsg = System.getLastError();
    }

    if(!isFatal && listener != null) {
      getActivity().runOnUiThread(new Runnable() {
        @Override
        public void run() {
          listener.onInitDone();
        }
      });
    }

    if(errMsg == null)
      return;

    final boolean fFatal = isFatal;
    final String fMsg = errMsg;

    getActivity().runOnUiThread(new Runnable() {
      @Override
      public void run() {
        if(fFatal) {
          new FinishDialog(getString(R.string.initialization_error), fMsg, getActivity()).show();
        } else {
          new ErrorDialog(getString(R.string.initialization_error), fMsg, getActivity()).show();
        }
      }
    });
  }

  public interface OnFragmentInteractionListener {
    void onInitDone();
  }
}
