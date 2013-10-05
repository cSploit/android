package it.evilsocket.dsploit.core;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.DialogInterface;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

import it.evilsocket.dsploit.R;

public class DownloadManager
{
  public interface StatusReceiver
  {
    public void onStart();
    public void onError(Exception e);
    public void onFinished( File file );
    public void onCanceled();
  }

  private Activity mParent = null;
  private ProgressDialog mProgressDialog = null;

  public DownloadManager( Activity parent, int progressTitleId, int progressMessageId ){
    mParent = parent;
    mProgressDialog = new ProgressDialog( mParent );

    mProgressDialog.setTitle(parent.getString(progressTitleId));
    mProgressDialog.setMessage( parent.getString(progressMessageId) );
    mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
    mProgressDialog.setMax(100);
    mProgressDialog.setCancelable(false);

    mProgressDialog.setButton(DialogInterface.BUTTON_NEGATIVE, mParent.getString(R.string.cancel), new DialogInterface.OnClickListener(){
      @Override
      public void onClick(DialogInterface dialog, int which){
        dialog.dismiss();
      }
    });
  }

  private String formatSize(int size){
    if(size < 1024)
      return size + " B";

    else if(size < (1024 * 1024))
      return (size / 1024) + " KB";

    else if(size < (1024 * 1024 * 1024))
      return (size / (1024 * 1024)) + " MB";

    else
      return (size / (1024 * 1024 * 1024)) + " GB";
  }

  private String formatSpeed(int speed){
    if(speed < 1024)
      return speed + " B/s";

    else if(speed < (1024 * 1024))
      return (speed / 1024) + " KB/s";

    else if(speed < (1024 * 1024 * 1024))
      return (speed / (1024 * 1024)) + " MB/s";

    else
      return (speed / (1024 * 1024 * 1024)) + " GB/s";
  }

  public void download( final String remote, final String fileName, final StatusReceiver receiver ){
    mProgressDialog.show();

    new Thread(new Runnable(){
      @Override
      public void run(){
        try
        {
          receiver.onStart();

          HttpURLConnection.setFollowRedirects(true);
          URL url = new URL(remote);
          HttpURLConnection connection = (HttpURLConnection) url.openConnection();
          byte[] buffer = new byte[1024];
          int read;

          connection.connect();

          //noinspection ResultOfMethodCallIgnored
          File file = new File(fileName);
          if(file.exists())
            //noinspection ResultOfMethodCallIgnored
            file.delete();

          FileOutputStream writer = new FileOutputStream(file);
          InputStream reader = connection.getInputStream();

          int total = connection.getContentLength(),
            downloaded = 0,
            sampled = 0;
          long time = java.lang.System.currentTimeMillis();
          double speed = 0.0,
            deltat;

          while( mProgressDialog.isShowing() && (read = reader.read(buffer)) != -1 )
          {
            writer.write(buffer, 0, read);

            downloaded += read;

            deltat = (java.lang.System.currentTimeMillis() - time) / 1000.0;

            if(deltat > 1.0){
              speed = (downloaded - sampled) / deltat;
              time = java.lang.System.currentTimeMillis();
              sampled = downloaded;
            }

            // update the progress ui
            final int fdown = downloaded,
              ftot = total;
            final double fspeed = speed;

            mParent.runOnUiThread(new Runnable(){
              @Override
              public void run(){
                mProgressDialog.setMessage("[ " + formatSpeed((int) fspeed) + " ] " + formatSize(fdown) + " / " + formatSize(ftot) + " ...");
                mProgressDialog.setProgress((100 * fdown) / ftot);
              }
            });

          }

          writer.close();
          reader.close();

          if( mProgressDialog.isShowing() ){
            receiver.onFinished( file );
          }
          else {
            receiver.onCanceled();
          }
        }
        catch(Exception e){
          System.errorLogging(e);
          receiver.onError(e);
        }

        if( mProgressDialog.isShowing() )
          mProgressDialog.dismiss();
      }
    }).start();
  }
}
