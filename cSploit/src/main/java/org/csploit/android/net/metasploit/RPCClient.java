package org.csploit.android.net.metasploit;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.msgpack.MessagePack;
import org.msgpack.MessageTypeException;
import org.msgpack.packer.Packer;
import org.msgpack.type.*;
import org.msgpack.unpacker.Unpacker;
import org.msgpack.unpacker.Converter;

import org.csploit.android.core.System;
import org.csploit.android.core.Logger;

//TODO: add license and write down that we had taken part of this code from armitage

@SuppressWarnings("rawtypes")
public class RPCClient
{
  private URL u;
  private URLConnection huc;
  private String token;
  private static volatile MessagePack msgpack = null;
  private final Map callCache = new HashMap();
  private final Lock lock = new ReentrantLock();
  private final boolean mRemote;

  private static final Pattern 	GET_FILENAME 	= Pattern.compile("[^/]+$");
  private static final Pattern 	GET_FUNC 		= Pattern.compile("`([^']+)");

  public RPCClient(final String host, final String username, final String password, final int port, final boolean ssl) throws IOException, MSFException
  {
    u = new URL("http" + (ssl ? "s" : ""), host, port, "/api/");

    if(msgpack==null)
      msgpack = new MessagePack();

    login(username, password);

    mRemote = !(host.equals("127.0.0.1") || System.getNetwork().isInternal(host));
  }

  protected void writeCall(String methodName, Object[] args) throws IOException {
    huc = u.openConnection();
    huc.setDoOutput(true);
    huc.setDoInput(true);
    huc.setUseCaches(false);
    huc.setRequestProperty("Content-Type", "binary/message-pack");
    huc.setReadTimeout(0);
    OutputStream os = huc.getOutputStream();
    Packer pk = msgpack.createPacker(os);
    pk.writeArrayBegin(args.length+1);
    pk.write(methodName);
    for (Object arg : args)
      pk.write(arg);
    pk.writeArrayEnd();
    pk.close();
    os.close();
  }

  //IOExceptions for data connection, Runtime Exception for msgpack errors
  protected Value readResp() throws IOException, MsgpackException {
    InputStream is = huc.getInputStream();
    Unpacker unpk = null;
    try
    {
      unpk = msgpack.createUnpacker(is);
      return unpk.readValue();
    }
    catch ( IOException ioe) // RuntimeExceptions for msgpack stuff.
    {
      throw new MsgpackException(ioe);
    }
    finally
    {
      if(unpk!=null)
        unpk.close();
    }
  }


  @SuppressWarnings("unchecked")
  protected Map exec (String methname, Object[] params) throws IOException, MsgpackException, MSFException
  {
    Object response = null;
    try {
      synchronized(this) {
        lock.lock();
        writeCall(methname, params);
        response = readResp();
      }
    }
    finally
    {
      lock.unlock();
    }
    response = unMsg((Value)response);
    if (response instanceof Map) {
      return (Map)response;
    }
    else {
      Map temp = new HashMap();
      temp.put("response", response);
      return temp;
    }
  }

  private void login ( String username, String password) throws IOException, MSFException
  {
    try
    {
			/* login to msf server */
      Map results = exec("auth.login",new Object[]{ username, password });

			/* save the temp token (lasts for 5 minutes of inactivity) */
      token = results.get("token").toString();

			/* generate a non-expiring token and use that */
      results = exec("auth.token_generate", new Object[]{ token });
      token = results.get("token").toString();
    }
    catch ( MsgpackException me)
    {
      throw new IOException(me);
    }
  }

  /** Caches certain calls and checks cache for re-executing them.
   * If not cached or not cacheable, calls exec. */
  @SuppressWarnings("unchecked")
  private Object cacheExecute(String methodName, Object[] params) throws IOException, MSFException, MsgpackException {
    if (methodName.equals("module.info") || methodName.equals("module.options") || methodName.equals("module.compatible_payloads") || methodName.equals("core.version")) {
      StringBuilder keysb = new StringBuilder(methodName);

      for(Object o : params)
        keysb.append(o.toString());

      String key = keysb.toString();
      Object result = callCache.get(key);

      if(result != null)
        return result;

      result = exec(methodName, params);
      callCache.put(key, result);
      return result;
    }
    return exec(methodName, params);
  }

  public Object call(String method) throws IOException, MSFException
  {
    try
    {
      Object[] tmp = new Object[] { token };
      return cacheExecute(method, tmp);
    }
    catch ( MsgpackException me)
    {
      throw new IOException(me);
    }
  }

  @SuppressWarnings("unchecked")
  public Object call(String method, Object... args) throws IOException, MSFException
  {
    try
    {
      Object[] local_array = new Object[args.length+1];
      local_array[0] = token;
      java.lang.System.arraycopy(args, 0, local_array, 1, args.length);
      return cacheExecute(method, local_array);
    }
    catch ( MsgpackException me)
    {
      throw new IOException(me);
    }
  }

  @SuppressWarnings("unchecked")
  public void updateSessions() {
    Map<Integer,Map<String,Object>> openSessions;
    Integer[] ids;

    try {
      openSessions = (Map<Integer, Map<String, Object>>) call("session.list");
      ids = openSessions.keySet().toArray(new Integer[openSessions.size()]);

      for(Integer id : ids) {
        try {
          String type = (String)openSessions.get(id).get("type");
          Session s;
          if("shell".equals(type)) {
            s = new ShellSession(id,openSessions.get(id));
          } else if("meterpreter".equals(type)) {
            //TODO
            s = new Session(id,openSessions.get(id));
          } else {
            s = new Session(id,openSessions.get(id));
          }
          org.csploit.android.core.System.getCurrentTarget().addSession(s);
        } catch ( UnknownHostException e) {
          Logger.info(e.getMessage());
        }
      }
    } catch (IOException e) {
      e.printStackTrace();
    } catch (MSFException e) {
      e.printStackTrace();
    }
  }

  public boolean isRemote() {
    return mRemote;
  }

  public boolean isConnected() {
    try {
      exec("core.version", new Object[]{});
      return true;
    } catch (MSFException | MsgpackException | IOException e) {
      return false;
    }
  }

  // Errors from server
  public static class MSFException extends Exception {
    public MSFException(String message) {
      super(message);
    }
  }

  //Errors from msgpack
  private static class MsgpackException extends Exception {
    public MsgpackException(Exception ex) {
      super(ex);
    }
  }

  /**
   * Decodes a response recursively from MessagePackObject to a normal Java object
   * @param src MessagePack response
   * @return decoded object
   */
  @SuppressWarnings("unchecked")
  private Object unMsg(Value src) throws MsgpackException, MSFException
  {
    Object out = src;
    Iterator i;
    try
    {
      Converter conv = new Converter(msgpack,src);
      switch(src.getType())
      {
        case MAP:
          Map hout = new HashMap(conv.readMapBegin());
          i = conv.iterator();
          try
          {
            while(i.hasNext())
              hout.put(unMsg((Value)i.next()), unMsg((Value)i.next()));
          }
          catch ( MessageTypeException mte)
          {
            //FIXME: https://github.com/muga/msgpack-java-0.7/issues/2
          }
          finally
          {
            conv.readMapEnd(false);
          }
          out = hout;
          // special snippet for msfrpcd exceptions handling
          if(hout.containsKey("error") && hout.containsKey("error_class"))
          {
            MSFException msfex = new MSFException((String)hout.get("error_message"));
            ArrayList<StackTraceElement> trace = new ArrayList<StackTraceElement>();
            StackTraceElement[] trace_a;
            String file,filename,func;
            int line;
            Matcher matcher;
            for(String str : (ArrayList<String>)hout.get("error_backtrace"))
            {
              try
              {
                StringTokenizer strTok = new StringTokenizer(str, ":");
                if(strTok.countTokens() != 3)
                  throw new RuntimeException("more then 2 ':'");
                file = strTok.nextToken();
                if((matcher = GET_FILENAME.matcher(file)) == null || !matcher.find())
                  throw new RuntimeException("cannot find filename");
                filename = matcher.group();
                line = Integer.parseInt(strTok.nextToken());
                func = strTok.nextToken();
                if((matcher = GET_FUNC.matcher(func)) == null || !matcher.find())
                  throw new RuntimeException("cannot find method name");
                trace.add(new StackTraceElement(filename, matcher.group(1), file, line));
              }
              catch ( Exception e)
              {
                Logger.warning("cannot parse \""+str+"\" as stack trace");
                System.errorLogging(e);
              }
            }
            trace_a = new StackTraceElement[trace.size()];
            trace_a = trace.toArray(trace_a);
            msfex.setStackTrace(trace_a);
            throw msfex;
          }
          break;
        case ARRAY:
          out = new ArrayList(conv.readArrayBegin());
          i = conv.iterator();
          try
          {
            while(i.hasNext())
              ((ArrayList)out).add(unMsg((Value)i.next()));
          }
          catch ( MessageTypeException mte)
          {
            //FIXME: https://github.com/muga/msgpack-java-0.7/issues/2
          }
          finally
          {
            conv.readArrayEnd(false);
          }
          break;
        case BOOLEAN:
          out = conv.readBoolean();
          break;
        case FLOAT:
          out = conv.readFloat();
          break;
        case INTEGER:
          out = conv.readInt();
          break;
        case NIL:
          conv.readNil();
          out = null;
          break;
        case RAW:
          out = conv.readString();
      }
    }
    catch ( IOException ioe)
    {
      throw new MsgpackException(ioe);
    }
    catch ( MessageTypeException mte)
    {
      throw new MsgpackException(mte);
    }
    return out;
  }

}