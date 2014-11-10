package it.evilsocket.dsploit.net.Databases;

import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;

/**
 * this class read data from a remote HTTP server.
 * reading from sockets cannot be interrupted,
 * so i created this for make us able to interrupt remote jobs.
 */
public class RemoteReader extends Thread {

  private String mBody = "";
  private final URL mUrl;
  private InputStream in;
  private final byte[] buffer = new byte[1024];
  private int mCount;

  public RemoteReader(String url) throws MalformedURLException {
    mUrl = new URL( url );
  }

  @Override
  public void interrupt() {
    if(in!=null) {
      try {
        in.close();
      } catch (IOException e) {
        // swallow
      }
    }
    // TODO: spawn a thread that read pending bytes ( high Recv-Q in netstat -an )
    super.interrupt();
  }

  public void run() {
    try {
      in = mUrl.openConnection().getInputStream();
      while((mCount = in.read(buffer)) > 0) {
        mBody+=(new String(buffer,0,mCount));
      }
    } catch (IOException e) {
      // swallow
    } catch ( IllegalStateException e) {
      // swallow (reading from a GZIP closed stream does not produce IOException, but IllegalStateException)
    } finally {
      if(in!=null)
        try {
          in.close();
        } catch (IOException e) {
          // swallow
        }
    }
  }

  public String getResponse() {
    return mBody;
  }
}
