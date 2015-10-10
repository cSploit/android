package org.csploit.android.net.metasploit;

import org.csploit.android.core.Logger;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Map;

/**
 * this class store data about MSF options.
 * options can be about both exploits and payload.
 */
@SuppressWarnings("unchecked")
public class Option {
  private final static String[] requiredFields = new String[] {"type","required","advanced","evasion","desc"};
  private final static Map<String,types> typesMap = new java.util.HashMap<String, types>() {
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

  public enum types {
    STRING,
    BOOLEAN,
    PATH,
    ADDRESS,
    INTEGER,
    PORT,
    ENUM
  }

  private String mName,mDesc,mValue;
  private Map<String,Object> mAttributes;
  private types mType;
  private boolean mAdvanced,mRequired,mEvasion;
  private String[] enums;

  public Option(String name, Map<String,Object> attrs) throws IllegalArgumentException {
    mName = name;
    mAttributes = attrs;
    // check for required data
    for(String field : requiredFields)
      if(!mAttributes.containsKey(field))
        throw new IllegalArgumentException("missing "+field+" field");
    // get type
    String type = (String) mAttributes.get("type");
    if(!typesMap.containsKey(type))
      throw new IllegalArgumentException("unknown option type: "+type);
    mType = typesMap.get(type);
    if(mType == types.ENUM) {
      if(!mAttributes.containsKey("enums"))
        throw new IllegalArgumentException("missing enums field");
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

  // TODO: make more setValue methods each with the corresponding type.
  public void setValue(String value) throws NumberFormatException {
    switch (mType) {
      case STRING:
        mValue = value;
        break;
      case ADDRESS:
        try {
          mValue = InetAddress.getByName(value).getHostAddress();
        } catch (UnknownHostException uhe) {
          throw new NumberFormatException("invalid IP address: " + value);
        }
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
