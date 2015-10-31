package org.csploit.msf.impl;

import org.apache.commons.io.IOUtils;
import org.csploit.msf.api.MsfException;
import org.msgpack.MessagePack;
import org.msgpack.packer.Packer;
import org.msgpack.template.AbstractTemplate;
import org.msgpack.template.Template;
import org.msgpack.template.Templates;
import org.msgpack.type.Value;
import org.msgpack.type.ValueType;
import org.msgpack.unpacker.Unpacker;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A Msf Msgpack simple client
 */
@SuppressWarnings("rawtypes, unchecked")
class MsgpackClient {
  private URL u;
  private String token;
  private static MessagePack msgpack = null;

  private static final Pattern RUBY_STACKTRACE_PATTERN = Pattern.compile("^(from )?(.*/([^/.]+)(\\.[^:]+)?):([0-9]+):in `(.*)'");
  private static final Template<Map<String, String>> mapStringStringTemplate = Templates.tMap(Templates.TString, Templates.TString);
  private static final Template<Map<Integer, String>> mapIntegerStringTemplate = Templates.tMap(Templates.TInteger, Templates.TString);
  private static final Map<String, Template> methodTemplatesMap = new HashMap<String, Template>() {{
    put("auth.login", mapStringStringTemplate);
    put("auth.token_generate", mapStringStringTemplate);
    put("module.exploits", SingleTemplate.getModulesInstance());
    put("module.post", SingleTemplate.getModulesInstance());
    put("module.payloads", SingleTemplate.getModulesInstance());
    put("module.auxiliary", SingleTemplate.getModulesInstance());
    put("module.encoders", SingleTemplate.getModulesInstance());
    put("module.nops", SingleTemplate.getModulesInstance());
    put("module.info", ModuleInfoTemplate.getInstance());
    put("module.options", Templates.tMap(Templates.TString, OptionTemplate.getInstance()));
    put("job.list", mapIntegerStringTemplate);
    put("job.info", JobInfoTemplate.getInstance());
    put("session.list", Templates.tMap(Templates.TInteger, SessionInfoTemplate.getInstance()));
    put("session.compatible_modules", SingleTemplate.getModulesInstance());
    put("session.stop", SingleTemplate.getBoolInstance());
    put("session.shell_read", SingleTemplate.getReadInstance());
    put("session.shell_write", SingleTemplate.getReadInstance());
    put("session.shell_upgrade", SingleTemplate.getBoolInstance());
    put("session.meterpreter_read", SingleTemplate.getReadInstance());
    put("session.meterpreter_write", SingleTemplate.getBoolInstance());
    put("session.meterpreter_detach", SingleTemplate.getBoolInstance());
    put("session.meterpreter_session_kill", SingleTemplate.getBoolInstance());
    put("session.meterpreter_tabs", SingleTemplate.getTabsInstance());
  }};
  private static final Map<String, Template> parametricTemplatesMap = new HashMap<String, Template>() {{
    put("module.execute(payload)", SingleTemplate.getPayloadGenerateInstance());
    put("module.execute", SingleTemplate.getJobIdInstance());
  }};

  public MsgpackClient(final String host, final String username, final String password, final int port, final boolean ssl) throws IOException, MsfException {
    u = new URL("http" + (ssl ? "s" : ""), host, port, "/api/");

    if (msgpack == null)
      msgpack = new MessagePack();

    login(username, password);
  }

  /**
   * execute a remote command and get a response
   * @param methodName the method to call
   * @param params the parameters to send
   * @return the response
   * @throws IOException if some IO error occurs
   * @throws MsfException if the server reply with an exception
   */
  protected Object exec(String methodName, Object[] params) throws IOException, MsfException {
    Packer pk = null;
    URLConnection huc;
    OutputStream os = null;
    InputStream is = null;
    byte[] response = null;

    try {
      huc = u.openConnection();
      huc.setDoOutput(true);
      huc.setDoInput(true);
      huc.setUseCaches(false);
      huc.setRequestProperty("Content-Type", "binary/message-pack");
      huc.setReadTimeout(0);
      os = huc.getOutputStream();
      pk = msgpack.createPacker(os);
      pk.writeArrayBegin(params.length + 1);
      pk.write(methodName);
      for (Object arg : params)
        pk.write(arg);
      pk.writeArrayEnd();
      pk.close();
      os.close();
      is = huc.getInputStream();
      response = IOUtils.toByteArray(is);
    } finally {
      IOUtils.closeQuietly(os);
      IOUtils.closeQuietly(pk);
      IOUtils.closeQuietly(is);
    }

    Object firstParam = params.length > 1 ? params[1] : null;

    return parseResponse(methodName, firstParam, response);
  }

  private Object parseResponse(String methodName, Object firstParam, byte[] response)
          throws IOException, MsfException {
    ByteArrayInputStream bis = new ByteArrayInputStream(response);
    Unpacker unpacker = msgpack.createUnpacker(bis);

    // attempt to read an MsfException
    try {
      MsfException exception = unpacker.read(ServerExceptionTemplate.getInstance());
      throw new MsfException("failed to execute command: " + methodName, exception);
    } catch (IllegalArgumentException e) {
      bis.reset();
      unpacker = msgpack.createUnpacker(bis);
    }

    Template t = null;
    String withArg = null;

    if (methodTemplatesMap.containsKey(methodName)) {
      t = methodTemplatesMap.get(methodName);
    } else if(firstParam != null) {
      withArg = String.format("%s(%s)", methodName, firstParam.toString());
      if(parametricTemplatesMap.containsKey(withArg)) {
        t = parametricTemplatesMap.get(withArg);
      } else if(parametricTemplatesMap.containsKey(methodName)) {
        t = parametricTemplatesMap.get(methodName);
      }
    }

    if(t == null) {
      throw new MsfException("unknown command: " + (withArg != null ? withArg : methodName));
    }

    return unpacker.read(t);
  }

  private void login(String username, String password) throws IOException, MsfException {
    /* login to msf server */
    Map<String, String> response = (Map<String, String>)
            exec("auth.login", new Object[]{username, password});

    /* save the temp token (lasts for 5 minutes of inactivity) */
    token = response.get("token");

    /* generate a non-expiring token and use that */
    response = (Map<String, String>) exec("auth.token_generate", new Object[]{token});
    token = response.get("token");
  }

  private Object call(String method) throws IOException, MsfException {
    Object[] tmp = new Object[]{token};
    return exec(method, tmp);
  }

  private Object call(String method, Object... args) throws IOException, MsfException {
    Object[] local_array = new Object[args.length + 1];
    local_array[0] = token;
    java.lang.System.arraycopy(args, 0, local_array, 1, args.length);
    return exec(method, local_array);
  }

  public boolean isConnected() {
    try {
      call("core.version");
      return true;
    } catch (MsfException | IOException e) {
      return false;
    }
  }

  public String[] getExploits() throws IOException, MsfException {
    return (String[]) call("module.exploits");
  }

  public String[] getPayloads() throws IOException, MsfException {
    return (String[]) call("module.payloads");
  }

  public String[] getPosts() throws IOException, MsfException {
    return (String[]) call("module.post");
  }

  public String[] getEncoders() throws IOException, MsfException {
    return (String[]) call("module.encoders");
  }

  public String[] getNops() throws IOException, MsfException {
    return (String[]) call("module.nops");
  }

  public String[] getAuxiliaries() throws IOException, MsfException {
    return (String[]) call("module.auxiliary");
  }

  public ModuleInfo getModuleInfo(String type, String name) throws IOException, MsfException {
    return (ModuleInfo) call("module.info", type, name);
  }

  public JobInfo getJobInfo(int id) throws IOException, MsfException {
    return (JobInfo) call("job.info", id);
  }

  public Map<Integer, String> getJobs() throws IOException, MsfException {
    return (Map<Integer, String>) call("job.list");
  }

  public Map<Integer, SessionInfo> getSessions() throws IOException, MsfException {
    return (Map<Integer, SessionInfo>) call("session.list");
  }

  public String[] getSessionCompatibleModules(int sessionId) throws IOException, MsfException {
    return (String[]) call("session.compatible_modules", sessionId);
  }

  public void stopSession(int sessionId) throws IOException, MsfException {
    call("session.stop", sessionId);
  }

  public String readFromShellSession(int sessionId) throws IOException, MsfException {
    return (String) call("session.shell_read", sessionId);
  }

  public int writeToShellSession(int sessionId, String data) throws IOException, MsfException {
    return (Integer) call("session.shell_write", sessionId, data);
  }

  public void upgradeShellSession(int sessionId, String lhost, int lport) throws IOException, MsfException {
    call("session.upgrade", sessionId, lhost, lport);
  }

  public String readFromMeterpreterSession(int sessionId) throws IOException, MsfException {
    return (String) call("session.meterpreter_read", sessionId);
  }

  public boolean writeToMeterpreterSession(int sessionId, String data) throws IOException, MsfException {
    return (Boolean) call("session.meterpreter_write", sessionId, data);
  }

  public boolean detachMeterpreterSession(int sessionId) throws IOException, MsfException {
    return (Boolean) call("session.meterpreter_detach", sessionId);
  }

  public boolean killMeterpreterSession(int sessionId) throws IOException, MsfException {
    return (Boolean) call("session.meterpreter_session_kill", sessionId);
  }

  public String[] getMeterpreterTabs(int sessionId, String base) throws IOException, MsfException {
    return (String[]) call("session.meterpreter_tabs", sessionId, base);
  }

  public Map<String, OptionInfo> getModuleOptions(String type, String name) throws IOException, MsfException {
    return (Map<String, OptionInfo>) call("module.options", type, name);
  }

  public int execute(String type, String name, Map options) throws IOException, MsfException {
    return (Integer) call("module.execute", type, name, options);
  }

  public byte[] generatePayload(String type, String name, Map options) throws IOException, MsfException {
    return (byte[]) call("module.execute", type, name, options);
  }

  @Override
  public String toString() {
    return String.format("%s: { Url: %s }", this.getClass().getSimpleName(), u);
  }

  public static class ModuleInfo {
    public String name, description, license, filepath;
    public int rank;
    public String[][] references;
    public String[] authors;
    public Map<Integer, String> targets;
    public int defaultTarget;
    public Map<Integer, String> actions;
    public int defaultAction;
  }

  public static class JobInfo {
    public int id;
    public String name;
    public int startTime;
  }

  public static class SessionInfo {
    public String type,
                  tunnelLocal,
                  tunnelPeer,
                  viaExploit,
                  viaPayload,
                  description,
                  info,
                  workspace,
                  sessionHost,
                  targetHost,
                  username,
                  uuid,
                  exploitUuid,
                  platform;
    public int sessionPort;
    public String[] routes;
  }

  public static class OptionInfo {
    String type, description;
    boolean required, advanced, evasion;
    Object defaultValue;
    String[] enums;
  }

  public static class ServerException extends MsfException {
    public ServerException(String causeClass, String message) {
      super(causeClass + ": " + message);
    }
  }

  private static class ServerExceptionTemplate extends AbstractTemplate<ServerException> {

    private static final ServerExceptionTemplate instance = new ServerExceptionTemplate();

    private ServerExceptionTemplate() {
      super();
    }

    public static ServerExceptionTemplate getInstance() {
      return instance;
    }

    @Override
    public void write(Packer pk, ServerException v, boolean required) throws IOException {
      throw new UnsupportedOperationException();
    }

    @Override
    public ServerException read(Unpacker u, ServerException to, boolean required) throws IOException {
      if(!required && u.trySkipNil()) {
        return null;
      }

      boolean error = false;
      String klass = null;
      String message = null;
      String[] backtrace = null;

      if(u.getNextType() != ValueType.MAP) {
        throw new IllegalArgumentException();
      }

      int size = u.readMapBegin();

      for(int i = 0; i < size && (!error || message == null || backtrace == null); i++) {
        String key = u.read(Templates.TString);
        switch (key) {
          case "error_string":
          case "error_message":
            message = u.readString();
            break;
          case "error_backtrace":
            backtrace = u.read(String[].class);
            break;
          case "error_class":
            klass = u.readString();
            break;
          case "error":
            error = u.readBoolean();
            break;
          default:
            u.skip();
        }
      }

      u.readMapEnd();

      if(!error) {
        throw new IllegalArgumentException();
      }

      if(to == null) {
        // this will not overwrite the previous message...
        to = new ServerException(
                klass == null ? "Unknown" : klass,
                message);
      }

      if(backtrace != null) {
        fillStackTrace(backtrace, to);
      }

      return to;
    }

    private void fillStackTrace(String[] backtrace, Exception to) {
      Matcher matcher;
      String file, filename, func;
      int line;
      ArrayList<StackTraceElement> trace = new ArrayList<>();

      for (String str : backtrace) {
        matcher = RUBY_STACKTRACE_PATTERN.matcher(str);
        if(!matcher.find()) {
          continue;
        }

        file = matcher.group(2);
        filename = matcher.group(3);
        line = Integer.parseInt(matcher.group(5));
        func = matcher.group(6);

        trace.add(new StackTraceElement(filename, func, file, line));
      }

      StackTraceElement[] traceElements = new StackTraceElement[trace.size()];
      trace.toArray(traceElements);
      to.setStackTrace(traceElements);
    }
  }

  private static class ModuleInfoTemplate extends AbstractTemplate<ModuleInfo> {

    private static final ModuleInfoTemplate instance = new ModuleInfoTemplate();

    private ModuleInfoTemplate() {
      super();
    }

    public static ModuleInfoTemplate getInstance() {
      return instance;
    }

    @Override
    public void write(Packer pk, ModuleInfo v, boolean required) throws IOException {
      throw new UnsupportedOperationException();
    }

    @Override
    public ModuleInfo read(Unpacker u, ModuleInfo to, boolean required) throws IOException {
      if(!required && u.trySkipNil()) {
        return null;
      }

      if(to == null) {
        to = new ModuleInfo();
      }

      int size = u.readMapBegin();

      to.defaultTarget = to.defaultAction = -1;

      for(int i = 0; i < size; i++ ) {
        String key = u.read(Templates.TString);
        switch (key) {
          case "name":
            to.name = u.readString();
            break;
          case "description":
            to.description = u.readString();
            break;
          case "license":
            to.license = u.readString();
            break;
          case "rank":
            to.rank = u.readInt();
            break;
          case "references":
            to.references = u.read(String[][].class);
            break;
          case "authors":
            to.authors = u.read(String[].class);
            break;
          case "targets":
            to.targets = u.read(mapIntegerStringTemplate);
            break;
          case "default_target":
            to.defaultTarget = u.readInt();
            break;
          case "actions":
            to.actions = u.read(mapIntegerStringTemplate);
            break;
          case "default_action":
            to.defaultAction = u.readInt();
            break;
          case "filepath":
            to.filepath = u.readString();
            break;
          default:
            u.skip();
        }
      }

      u.readMapEnd();

      if(to.references == null) {
        to.references = new String[0][2];
      }

      if(to.authors == null) {
        to.authors = new String[0];
      }

      return to;
    }
  }

  private static class JobInfoTemplate extends AbstractTemplate<JobInfo> {

    private static final JobInfoTemplate instance = new JobInfoTemplate();

    private JobInfoTemplate() {
      super();
    }

    public static JobInfoTemplate getInstance() {
      return instance;
    }

    @Override
    public void write(Packer pk, JobInfo v, boolean required) throws IOException {
      throw new UnsupportedOperationException();
    }

    @Override
    public JobInfo read(Unpacker u, JobInfo to, boolean required) throws IOException {
      if(!required && u.trySkipNil()) {
        return null;
      }
      int size = u.readMapBegin();

      if(to == null) {
        to = new JobInfo();
      }

      for(int i = 0; i < size; i++) {
        String key = u.readString();
        switch (key) {
          case "jid":
            to.id = u.readInt();
            break;
          case "name":
            to.name = u.readString();
            break;
          case "start_time":
            to.startTime = u.readInt();
            break;
          default:
            u.skip();
        }
      }

      u.readMapEnd();

      return to;
    }
  }

  private static class SessionInfoTemplate extends AbstractTemplate<SessionInfo> {

    private static final SessionInfoTemplate instance = new SessionInfoTemplate();

    private SessionInfoTemplate() {
      super();
    }

    public static SessionInfoTemplate getInstance() {
      return instance;
    }

    @Override
    public void write(Packer pk, SessionInfo v, boolean required) throws IOException {
      throw new UnsupportedOperationException();
    }

    @Override
    public SessionInfo read(Unpacker u, SessionInfo to, boolean required) throws IOException {
      if(!required && u.trySkipNil()) {
        return null;
      }

      if(to == null) {
        to = new SessionInfo();
      }

      int size = u.readMapBegin();

      for(int i = 0; i < size; i++) {
        String key = u.readString();
        switch (key) {
          case "type":
            to.type = u.readString();
            break;
          case "tunnel_local":
            to.tunnelLocal = u.readString();
            break;
          case "tunnel_peer":
            to.tunnelPeer = u.readString();
            break;
          case "via_exploit":
            to.viaExploit = u.readString();
            break;
          case "via_payload":
            to.viaPayload = u.readString();
            break;
          case "desc":
            to.description = u.readString();
            break;
          case "info":
            to.info = u.readString();
            break;
          case "workspace":
            to.workspace = u.readString();
            break;
          case "session_host":
            to.sessionHost = u.readString();
            break;
          case "session_port":
            to.sessionPort = u.readInt();
            break;
          case "target_host":
            to.targetHost = u.readString();
            break;
          case "username":
            to.username = u.readString();
            break;
          case "uuid":
            to.uuid = u.readString();
            break;
          case "exploit_uuid":
            to.exploitUuid = u.readString();
            break;
          case "routes":
            to.routes = u.readString().split(",");
            break;
          case "platform":
            to.platform = u.readString();
            break;
          default:
            u.skip();
        }
      }

      u.readMapEnd();

      return to;
    }
  }

  private static class OptionTemplate extends AbstractTemplate<OptionInfo> {

    private static final OptionTemplate instance = new OptionTemplate();

    private OptionTemplate() {
      super();
    }

    public static OptionTemplate getInstance() {
      return instance;
    }

    @Override
    public void write(Packer pk, OptionInfo v, boolean required) throws IOException {
      throw new UnsupportedOperationException();
    }

    @Override
    public OptionInfo read(Unpacker u, OptionInfo to, boolean required) throws IOException {
      if(!required && u.trySkipNil()) {
        return null;
      }

      if(to == null) {
        to = new OptionInfo();
      }

      int size = u.readMapBegin();
      to.defaultValue = null;
      to.enums = null;

      for(int i = 0; i < size; i++) {
        String key = u.readString();
        switch (key) {
          case "type":
            to.type = u.readString();
            break;
          case "required":
            to.required = u.readBoolean();
            break;
          case "advanced":
            to.advanced = u.readBoolean();
            break;
          case "evasion":
            to.evasion = u.readBoolean();
            break;
          case "desc":
            to.description = u.readString();
            break;
          case "default":
            Value val = u.readValue();
            to.defaultValue = readSingleObject(val);
            break;
          case "enums":
            to.enums = u.read(String[].class);
            break;
        }
      }

      u.readMapEnd();

      return to;
    }

    private Object readSingleObject(Value value) {
      switch (value.getType()) {
        case BOOLEAN:
          return value.asBooleanValue().getBoolean();
        case FLOAT:
          return value.asFloatValue().getFloat();
        case INTEGER:
          Long result = value.asIntegerValue().getLong();
          if(result < Integer.MAX_VALUE && result > Integer.MIN_VALUE) {
            return result.intValue();
          }
          return result;
        case RAW:
          return value.asRawValue().getString();
        case NIL:
        default:
          return null;
      }
    }
  }

  private static class SingleTemplate<T> extends AbstractTemplate<T> {

    private static final SingleTemplate<Integer> writeInstance = new SingleTemplate<>("write_count", int.class);
    private static final SingleTemplate<Integer> jobIdInstance = new SingleTemplate<>("job_id", int.class);
    private static final SingleTemplate<String>  readInstance  = new SingleTemplate<>("data", String.class);
    private static final SingleTemplate<Boolean> boolInstance = new SingleTemplate<>("result", boolean.class);
    private static final SingleTemplate<byte[]> payloadGenerateInstance = new SingleTemplate<>("payload", byte[].class);
    private static final SingleTemplate<String[]> modulesInstance = new SingleTemplate<>("modules", String[].class);
    private static final SingleTemplate<String[]> tabsInstance = new SingleTemplate<>("tabs", String[].class);

    private final String mapKey;
    private final Class<? extends T> klass;

    private SingleTemplate(String mapkey, Class<? extends T> klass) {
      super();
      this.mapKey = mapkey;
      this.klass = klass;
    }

    public static SingleTemplate<Integer> getWriteInstance() {
      return writeInstance;
    }

    public static SingleTemplate<Integer> getJobIdInstance() {
      return jobIdInstance;
    }

    public static SingleTemplate<String> getReadInstance() {
      return readInstance;
    }

    public static SingleTemplate<Boolean> getBoolInstance() {
      return boolInstance;
    }

    public static SingleTemplate<byte[]> getPayloadGenerateInstance() {
      return payloadGenerateInstance;
    }

    public static SingleTemplate<String[]> getModulesInstance() {
      return modulesInstance;
    }

    public static SingleTemplate<String[]> getTabsInstance() {
      return tabsInstance;
    }

    @Override
    public void write(Packer pk, T v, boolean required) throws IOException {
      throw new UnsupportedOperationException();
    }

    @Override
    public T read(Unpacker u, T to, boolean required) throws IOException {
      if(!required && u.trySkipNil()) {
        return null;
      }

      int size = u.readMapBegin();
      to = null;
      for(int i = 0; i < size; i++) {
        String key = u.readString();
        if(mapKey.equals(key)) {
          to = u.read(klass);
          break;
        }
        u.skip();
      }

      u.readMapEnd();

      return to;
    }
  }
}
