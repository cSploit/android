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
package it.evilsocket.dsploit.net;

import android.os.Parcel;
import android.os.Parcelable;
import android.os.StrictMode;

import org.apache.http.conn.util.InetAddressUtils;

import java.io.BufferedReader;
import java.io.IOException;
import java.net.InetAddress;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.prefs.InvalidPreferencesFormatException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.Locale;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.net.Network.Protocol;
import it.evilsocket.dsploit.net.metasploit.RPCClient;

public class Target
{
  // remove "dev" "rc" and other extra version infos
  private final static Pattern VERSION_PATTERN = Pattern.compile( "(([0-9]+\\.)+[0-9]+)[a-zA-Z]+");

  public enum Type{
    NETWORK,
    ENDPOINT,
    REMOTE;

    public static Type fromString(String type) throws Exception{
      if(type != null){
        type = type.trim().toLowerCase(Locale.US);
        if(type.equals("network"))
          return Type.NETWORK;

        else if(type.equals("endpoint"))
          return Type.ENDPOINT;

        else if(type.equals("remote"))
          return Type.REMOTE;
      }

      throw new Exception("Could not deserialize target type from string.");
    }
  }

  public static class Port{
    public Protocol protocol;
    public int number;
    public String service = "";
    public String	version = "";
    private ArrayList<Vulnerability> vulns = new ArrayList<Target.Vulnerability>();

    public Port( int port, Protocol proto, String service, String version) {
      Matcher matcher;
      this.number = port;
      this.protocol = proto;
      this.service = service;
      if(version != null && (matcher = VERSION_PATTERN.matcher(version)) != null && matcher.find())
        this.version = matcher.group(1);
      else
        this.version = version;
    }

    public Port( int port, Protocol proto, String service ) {
      this(port, proto, service, "");
    }

    public Port( int port, Protocol proto ) {
      this( port, proto, "", "" );
    }

    public String getServiceQuery() {
      if(version!=null)
        return service + " " + version;
      else
        return service;
    }

    // needed for vulnerabilities hashmap
    public String toString(){
      return protocol.toString() + "|" + number + "|" + service + "|" + version;
    }

    public void addVulnerability(Vulnerability vuln)
    {
      if(!vulns.contains(vuln))
        vulns.add(vuln);
    }

    public void delVulnerability(Vulnerability vuln)
    {
      if(vulns.contains(vuln))
        vulns.remove(vuln);
    }

    public void clearVulnerabilities()
    {
      vulns.clear();
    }

    public ArrayList<Vulnerability> getVulnerabilities()
    {
      return vulns;
    }

    public boolean equals(Object o) {
      if(o == null || o.getClass() != this.getClass())
        return false;
      Port p = (Port)o;
      if(p.number != this.number || p.protocol != this.protocol)
        return false;
      if(this.version!=null) {
        if(!this.version.equals(p.version))
          return false;
      } else if (p.version != null) {
        return false;
      }
      if(this.service!=null) {
        if(!this.service.equals(p.service))
          return false;
      } else if (p.version!=null) {
        return false;
      }
      return true;
    }
  }

  public static class Vulnerability
  {
    public String cve_id = null;
    public int osvdb_id = 0;
    public double severity   = 0;
    public String summary	   = null;
    private Port mPort = null;
    public boolean has_msf_exploit = false; // used for speedup search
    private ArrayList<Exploit> mExploits = new ArrayList<Target.Exploit>();

    public Vulnerability(){

    }

    public Vulnerability( BufferedReader reader ) throws Exception {
      String serialized = reader.readLine();
      String[] parts	  = serialized.split( "\\|", 3 );

      cve_id = parts[0];
      severity   = Double.parseDouble( parts[1] );
      summary	= parts[2];
    }

    public void from_osvdb( int id, double _severity, String _summary ) {
      osvdb_id = id;
      severity   = _severity;
      summary	= _summary;
    }

    public String getIdentifier() {
      if(osvdb_id>0)
        return Integer.toString(osvdb_id);
      else
        return cve_id;
    }

    public void setIdentifier( String identifier ) {
      this.cve_id = identifier;
    }



    public String toString(){
      if(osvdb_id>0)
        return Integer.toString(osvdb_id) + "|" + severity + "|" + summary;
      else
        return cve_id + "|" + severity + "|" + summary;
    }

    public String getHtmlColor( )
    {
      if( severity < 5.0 )
        return "#59FF00";

      else if( severity < 7 )
        return "#FFD732";

      else
        return "#FF0000";
    }

    public Port getPort()
    {
      return mPort;
    }

    public void setPort(Port port)
    {
      if(mPort==port)
        return;
      if(mPort!=null)
        mPort.delVulnerability(this);
      mPort = port;
    }

    public ArrayList<Exploit> getExploits()
    {
      return mExploits;
    }

    public void clearExploit()
    {
      mExploits.clear();
    }

    public void addExploit(Exploit ex)
    {
      if(!mExploits.contains(ex))
        mExploits.add(ex);
    }

    public void delExploit(Exploit ex)
    {
      if(mExploits.contains(ex))
        mExploits.remove(ex);
    }
  }

  @SuppressWarnings("unchecked")
  public static class ExploitOption {
    private final static String[] requiredFields = new String[] {"type","required","advanced","evasion","desc"};
    private final static HashMap<String,types> typesMap = new HashMap<String, types>() {
      {
        put("string", types.STRING);
        put("bool", types.BOOLEAN);
        put("address", types.ADDRESS);
        put("integer", types.INTEGER);
        put("port", types.PORT);
        put("path", types.PATH);
        put("enum", types.ENUM);
      }
    };

    public static enum types {
      STRING,
      BOOLEAN,
      PATH,
      ADDRESS,
      INTEGER,
      PORT,
      ENUM
    }

    private String mName,mDesc,mValue;
    private HashMap<String,Object> mAttributes;
    private types mType;
    private boolean mAdvanced,mRequired,mEvasion;
    private String[] enums;

    public ExploitOption(String name, HashMap<String,Object> attrs) throws RuntimeException {
      mName = name;
      mAttributes = attrs;
      // check for required data
      for(String field : requiredFields)
        if(!mAttributes.containsKey(field))
          throw new RuntimeException("missing "+field+" field");
      // get type
      String type = (String) mAttributes.get("type");
      if(!typesMap.containsKey(type))
        throw new RuntimeException("unknown option type: "+type);
      mType = typesMap.get(type);
      if(mType == types.ENUM) {
        if(!mAttributes.containsKey("enums"))
          throw new RuntimeException("missing enums field");
        //TODO: search if exists not string enums
        enums = new String[((ArrayList<String>)mAttributes.get("enums")).size()];
        enums = ((ArrayList<String>) mAttributes.get("enums")).toArray(enums);
      }
      // get all other data
      mDesc = (String) mAttributes.get("desc");
      mAdvanced = (Boolean) mAttributes.get("advanced");
      mRequired = (Boolean) mAttributes.get("required");
      mEvasion  = (Boolean) mAttributes.get("evasion");
    }


    public String getName() {
      return mName;
    }

    public String getDescription() {
      return mDesc;
    }

    public types getType() {
      return mType;
    }

    public boolean isAdvanced() {
      return mAdvanced;
    }

    public boolean isRequired() {
      return mRequired;
    }

    public boolean isEvasion() {
      return mEvasion;
    }

    public String[] getEnum() {
      return enums;
    }

    public void setValue(String value) throws NumberFormatException {
      switch (mType) {
        case STRING:
          mValue = value;
          break;
        case ADDRESS:
          if(!InetAddressUtils.isIPv4Address(value))
            throw new NumberFormatException("invalid IP address");
          mValue = value;
          break;
        case PORT:
          int i = Integer.parseInt(value);
          if(i<0 || i > 65535)
            throw new NumberFormatException("port must be between 0 and 65535");
          break;
        case BOOLEAN:
          value=value.toLowerCase();
          if(value.equals("true") || value.equals("false"))
            mValue=value;
          else
            throw new NumberFormatException("boolean must be true or false");
          break;
        case ENUM:
          //TODO: handle integer enums
          ArrayList<String> valid = ((ArrayList<String>)mAttributes.get("enums"));
          if(!valid.contains(value)) {
            String valid_line = "";
            for(String v : valid) {
              valid_line+=" " + v;
            }
            Logger.warning("expected: (" + valid_line + ") got: " + value);
            throw new NumberFormatException("invalid choice");
          }
          mValue = value;
          break;
        case PATH:
          //TODO:
          mValue = value;
          break;
      }
    }

    public String getValue() {
      if(mValue!=null)
        return mValue;
      else if(mAttributes.containsKey("default"))
        return mAttributes.get("default").toString();
      else
        return "";
    }
  }

  public static class Exploit
  {
    public String url;
    public String name;
    public String msf_name;
    private String mDescription = null;
    public int payload_size;
    public boolean started = false;
    private Vulnerability mVuln = null;
    private ArrayList<ExploitOption> options = null;
    //TODO: get payload_size
    //TODO: refactoring

    public Exploit(String name, boolean isMsfExploit) {
      if(isMsfExploit) {
        msf_name = name;
        if(System.getMsfRpc()!=null) {
          retrieveInfos();
          retrieveOptions();
        }
      } else {
        this.name = name;
      }
    }

    public Exploit(String name) {
      this(name,false);
    }

    @SuppressWarnings("unchecked")
    private void retrieveOptions() {
      try
      {
        HashMap<String,Object>  map =  (HashMap<String,Object>) System.getMsfRpc().execute("module.options","exploit",msf_name);
        Iterator it = map.entrySet().iterator();
        options = new ArrayList<ExploitOption>();
        while(it.hasNext()) {
          Map.Entry<String,HashMap<String,Object>> item = (Map.Entry<String, HashMap<String, Object>>) it.next();
          String name = item.getKey();
          ExploitOption opt = new ExploitOption(name,item.getValue());
          if(name.equals("RHOST"))
            opt.setValue(System.getCurrentTarget().getAddress().getHostAddress());
          options.add(opt);
        }
      } catch ( IOException e) {
        Logger.error(e.getMessage());
      } catch (RPCClient.MSFException e) {
        Logger.error(e.getMessage());
      }
    }

    @SuppressWarnings("unchecked")
    private void retrieveInfos() {
      try
      {
        HashMap<String,Object> map = (HashMap<String,Object>) System.getMsfRpc().execute("module.info","exploit",msf_name);
        mDescription=(String)map.get("name");
        //TODO:get all other things
      } catch ( IOException e) {
        Logger.error(e.getMessage());
      } catch (RPCClient.MSFException e) {
        Logger.error(e.getMessage());
      }
    }

    public String toString()
    {
      if(msf_name!=null)
        return msf_name.substring(msf_name.lastIndexOf('/')+1);
      return name;
    }

    public int getDrawableResourceId() {
      if(msf_name!=null)
        return R.drawable.exploit_msf;
      return R.drawable.exploit;
    }

    public String getDescription() {
      if(mDescription!=null)
        return mDescription;
      else
        return url;
    }

    public boolean isMsf() {
      return this.msf_name != null;
    }

    public void setVulnerability(Vulnerability vuln)
    {
      if(mVuln==vuln)
        return;
      if(mVuln!=null)
        mVuln.delExploit(this);
      mVuln=vuln;
      if(mVuln!=null && this.msf_name != null && options!=null) {
        for(ExploitOption opt : options)
          if(opt.getName().equals("RPORT"))
            opt.setValue(mVuln.getPort().number + "");
      }
    }

    public Vulnerability getVulnerability()
    {
      return mVuln;
    }

    public ArrayList<ExploitOption> getOptions() {
      if(options!=null)
        return options;
      else
        return new ArrayList<ExploitOption>();
    }

    public boolean equals(Object o) {
      if(o == null || o.getClass() != this.getClass())
        return false;
      Exploit e = (Exploit)o;
      return ((Exploit)o).name.equals(this.name);
    }
  }

  private Network mNetwork = null;
  private Endpoint mEndpoint = null;
  private int mPort = 0;
  private String mHostname = null;
  private Type mType = null;
  private InetAddress mAddress = null;
  private List<Port> mPorts = new ArrayList<Port>();
  private String mDeviceType = null;
  private String mDeviceOS = null;
  private String mAlias = null;
  private ArrayList<Vulnerability> mVulnerabilities = new ArrayList<Vulnerability>();
  private ArrayList<Exploit> mExploits = new ArrayList<Target.Exploit>();

  public static Target getFromString(String string){
    final Pattern PARSE_PATTERN = Pattern.compile("^(([a-z]+)://)?([0-9a-z\\-\\.]+)(:([\\d]+))?[0-9a-z\\-\\./]*$", Pattern.CASE_INSENSITIVE);
    final Pattern IP_PATTERN = Pattern.compile("^[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}$");

    Matcher matcher = null;
    Target target = null;

    try{
      string = string.trim();

      if((matcher = PARSE_PATTERN.matcher(string)) != null && matcher.find()){
        String protocol = matcher.group(2),
          address = matcher.group(3),
          sport = matcher.group(4);

        protocol = protocol != null ? protocol.toLowerCase(Locale.US) : null;
        sport = sport != null ? sport.substring(1) : null;

        if(address != null){
          // attempt to get the port from the protocol or the specified one
          int port = 0;

          if(sport != null)
            port = Integer.parseInt(sport);

          else if(protocol != null)
            port = System.getPortByProtocol(protocol);

          // determine if the 'address' part is an ip address or a host name
          if(IP_PATTERN.matcher(address).find()){
            // internal ip address
            if(System.getNetwork().isInternal(address)){
              target = new Target(new Endpoint(address, null));
              target.setPort(port);
            }
            // external ip address, return as host name
            else{
              target = new Target(address, port);
            }
          }
          // found a host name
          else
            target = new Target(address, port);
        }
      }
    }
    catch(Exception e){
      System.errorLogging(e);
    }

    // determine if the target is reachable.
    if(target != null){
      try{
        // This is needed to avoid NetworkOnMainThreadException
        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
        StrictMode.setThreadPolicy(policy);

        InetAddress.getByName(target.getCommandLineRepresentation());
      }
      catch(Exception e){
        target = null;
      }
    }

    return target;
  }

  public Target(BufferedReader reader) throws Exception{
    mType = Type.fromString(reader.readLine());
    mDeviceType = reader.readLine();
    mDeviceType = mDeviceType.equals("null") ? null : mDeviceType;
    mDeviceOS = reader.readLine();
    mDeviceOS = mDeviceOS.equals("null") ? null : mDeviceOS;
    mAlias = reader.readLine();
    mAlias = mAlias.equals("null") ? null : mAlias;

    if(mType == Type.NETWORK){
      return;
    }
    else if(mType == Type.ENDPOINT){
      mEndpoint = new Endpoint(reader);
    }
    else if(mType == Type.REMOTE){
      mHostname = reader.readLine();
      mHostname = mHostname.equals("null") ? null : mHostname;
      if(mHostname != null){
        // This is needed to avoid NetworkOnMainThreadException
        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
        StrictMode.setThreadPolicy(policy);

        mAddress = InetAddress.getByName(mHostname);
      }
    }

    int ports = Integer.parseInt(reader.readLine());
    for(int i = 0; i < ports; i++){
      String key = reader.readLine();
      String[] parts = key.split("\\|", 4);
      Port port = new Port
      (
        Integer.parseInt(parts[1]),
        Protocol.fromString(parts[0]),
        parts[2],
        parts[3]
      );

      mPorts.add(port);

      int nvulns = Integer.parseInt(reader.readLine());
      for(int j = 0; j < nvulns; j++){
        Vulnerability v = new Vulnerability(reader);

        v.setPort(port);
        port.addVulnerability(v);
        mVulnerabilities.add(v);
      }
    }
  }

  public void serialize(StringBuilder builder){
    builder.append(mType).append("\n");
    builder.append(mDeviceType).append("\n");
    builder.append(mDeviceOS).append("\n");
    builder.append(mAlias).append("\n");

    // a network can't be saved in a session file
    if(mType == Type.NETWORK){
      return;
    }
    else if(mType == Type.ENDPOINT){
      mEndpoint.serialize(builder);
    }
    else if(mType == Type.REMOTE){
      builder.append(mHostname).append("\n");
    }

    builder.append(mPorts.size()).append("\n");
    builder.append( mPorts.size() + "\n" );
    for( Port port : mPorts )
    {
      String key = port.toString();
      builder.append( key + "\n" );
      builder.append(port.getVulnerabilities().size()+"\n");
      for(Vulnerability vuln : port.getVulnerabilities())
        builder.append(vuln.toString() + "\n");
    }
  }

  public void setAlias(String alias){
    mAlias = alias.trim();
  }

  public String getAlias(){
    return mAlias;
  }

  public boolean hasAlias(){
    return mAlias != null && !mAlias.isEmpty();
  }

  public boolean comesAfter(Target target){
    return mType != Type.NETWORK && (mType != Type.ENDPOINT || target.getType() == Type.ENDPOINT && mEndpoint.getAddressAsLong() > target.getEndpoint().getAddressAsLong());
  }

  public Target(Network net){
    setNetwork(net);
  }

  public Target(InetAddress address, byte[] hardware){
    setEndpoint(address, hardware);
  }

  public Target(Endpoint endpoint){
    setEndpoint(endpoint);
  }

  public Target(String hostname, int port){
    setHostname(hostname, port);
  }

  public InetAddress getAddress(){
    if(mType == Type.ENDPOINT)
      return mEndpoint.getAddress();

    else if(mType == Type.REMOTE)
      return mAddress;

    else
      return null;
  }

  public boolean equals(Target target){
    if(mType == target.getType()){
      if(mType == Type.NETWORK)
        return mNetwork.equals(target.getNetwork());

      else if(mType == Type.ENDPOINT)
        return mEndpoint.equals(target.getEndpoint());

      else if(mType == Type.REMOTE)
        return mHostname.equals(target.getHostname());
    }


    return false;
  }

  public boolean equals(Object o){
    return o instanceof Target && equals((Target) o);
  }

  public String getDisplayAddress(){
    if(mType == Type.NETWORK)
      return mNetwork.getNetworkRepresentation();

    else if(mType == Type.ENDPOINT)
      return mEndpoint.getAddress().getHostAddress() + (mPort == 0 ? "" : ":" + mPort);

    else if(mType == Type.REMOTE)
      return mHostname + (mPort == 0 ? "" : ":" + mPort);

    else
      return "???";
  }

  public String toString(){
    if(hasAlias())
      return mAlias;

    else
      return getDisplayAddress();
  }

  public String getDescription(){
    if(mType == Type.NETWORK)
      return System.getContext().getString(R.string.network_subnet_mask);

    else if(mType == Type.ENDPOINT){
      String vendor = System.getMacVendor(mEndpoint.getHardware()),
        desc = mEndpoint.getHardwareAsString();

      if(vendor != null) desc += " - " + vendor;

      try{
        if(mEndpoint.getAddress().equals(System.getNetwork().getGatewayAddress()))
          desc += "\n" + System.getContext().getString(R.string.gateway_router);

        else if(mEndpoint.getAddress().equals(System.getNetwork().getLocalAddress()))
          desc += System.getContext().getString(R.string.this_device);
      }
      catch(SocketException e){
        System.errorLogging(e);
      }

      return desc.trim();
    } else if(mType == Type.REMOTE)
      return mAddress.getHostAddress();

    return "";
  }

  public String getCommandLineRepresentation(){
    if(mType == Type.NETWORK)
      return mNetwork.getNetworkRepresentation();

    else if(mType == Type.ENDPOINT)
      return mEndpoint.getAddress().getHostAddress();

    else if(mType == Type.REMOTE)
      return mHostname;

    else
      return "???";
  }

  public boolean isRouter(){
    try{
      return (mType == Type.ENDPOINT && mEndpoint.getAddress().equals(System.getNetwork().getGatewayAddress()));
    }
    catch(Exception e){
      System.errorLogging(e);
    }

    return false;
  }

  public int getDrawableResourceId(){
    try{
      if(mType == Type.NETWORK)
        return R.drawable.target_network;

      else if(mType == Type.ENDPOINT)
        if(isRouter())
          return R.drawable.target_router;

        else if(mEndpoint.getAddress().equals(System.getNetwork().getLocalAddress()))
          return R.drawable.target_self;

        else
          return R.drawable.target_endpoint;

      else if(mType == Type.REMOTE)
        return R.drawable.target_remote;
    }
    catch(Exception e){
      System.errorLogging(e);
    }

    return R.drawable.target_network;
  }

  public void setNetwork(Network net){
    mNetwork = net;
    mType = Type.NETWORK;
  }

  public void setEndpoint(Endpoint endpoint){
    mEndpoint = endpoint;
    mType = Type.ENDPOINT;
  }

  public void setEndpoint(InetAddress address, byte[] hardware){
    mEndpoint = new Endpoint(address, hardware);
    mType = Type.ENDPOINT;
  }

  public void setHostname(String hostname, int port){
    mHostname = hostname;
    mPort = port;
    mType = Type.REMOTE;

    try{
      // This is needed to avoid NetworkOnMainThreadException
      StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
      StrictMode.setThreadPolicy(policy);

      mAddress = InetAddress.getByName(mHostname);
    } catch(Exception e){
      Logger.debug(e.toString());
    }
  }

  public Type getType(){
    return mType;
  }

  public Network getNetwork(){
    return mNetwork;
  }

  public Endpoint getEndpoint(){
    return mEndpoint;
  }

  public void setPort(int port){
    mPort = port;
  }

  public int getPort(){
    return mPort;
  }

  public String getHostname(){
    return mHostname;
  }

  public void addOpenPort(Port port){
    if(!mPorts.contains(port))
      mPorts.add( port );
  }

  public void addOpenPort(int port, Protocol protocol){
    addOpenPort(new Port(port, protocol));
  }

  public void addOpenPort(int port, Protocol protocol, String service){
    addOpenPort(new Port(port, protocol, service));
  }

  public List<Port> getOpenPorts(){
    return mPorts;
  }

  public boolean hasOpenPorts(){
    return !mPorts.isEmpty();
  }

  public boolean hasOpenPortsWithService(){
    if(!mPorts.isEmpty()){
      for(Port p : mPorts){
        if(p.service != null && !p.service.isEmpty())
          return true;
      }
    }

    return false;
  }

  public boolean hasOpenPort(int port){
    for(Port p : mPorts){
      if(p.number == port)
        return true;
    }

    return false;
  }

  public boolean hasVulnerabilities() {
    return !mVulnerabilities.isEmpty();
  }

  public void setDeviceType(String type){
    mDeviceType = type;
  }

  public String getDeviceType(){
    return mDeviceType;
  }

  public void setDeviceOS(String os){
    mDeviceOS = os;
  }

  public String getDeviceOS(){
    return mDeviceOS;
  }

  public void addVulnerability( Port port, Vulnerability v ) {
    if(mVulnerabilities.contains(v))
      return;
    mVulnerabilities.add(v);
    port.addVulnerability(v);
    v.setPort(port);
  }

  public ArrayList< Vulnerability > getVulnerabilities() {
    return mVulnerabilities;
  }

  public void addExploit(Vulnerability v, Exploit ex)
  {
    if(mExploits.contains(ex) || !mVulnerabilities.contains(v))
      return;
    mExploits.add(ex);
    v.addExploit(ex);
    ex.setVulnerability(v);
  }

  public ArrayList<Exploit> getExploits()
  {
    return mExploits;
  }
}
