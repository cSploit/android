package org.csploit.android.gui;

import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceScreen;
import android.text.InputType;
import android.util.Patterns;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.net.metasploit.MsfExploit;
import org.csploit.android.net.metasploit.Option;
import org.csploit.android.net.metasploit.Payload;

import java.util.ArrayList;
import java.util.Collection;

/**
 * activity fo setting exploit options.
 */

public class MsfPreferences extends PreferenceActivity {

  private Collection<Option> options;
  private final Preference.OnPreferenceChangeListener listener = new Preference.OnPreferenceChangeListener() {

    public boolean onPreferenceChange(Preference preference, Object newValue) {
      Option opt = null;
      String key;

      key = preference.getKey();

      for(Option o : options) {
        if(o.getName().equals(key)) {
          opt = o;
          break;
        }
      }
      if(opt==null)
        return false;
      switch (opt.getType()) {
        case STRING:
        case PATH:
        case ENUM:
          opt.setValue((String)newValue);
          return true;
        case ADDRESS:
          if(Patterns.IP_ADDRESS.matcher((String)newValue).matches()) {
            opt.setValue((String)newValue);
            return true;
          } else
            Toast.makeText(getApplicationContext(),getString(R.string.error_invalid_address_or_port),Toast.LENGTH_LONG).show();
          break;
        case INTEGER:
          try {
            int res = Integer.parseInt((String)newValue);
            opt.setValue(""+res);
            return true;
          } catch ( NumberFormatException e) {
            Toast.makeText(getApplicationContext(),getString(R.string.pref_err_invalid_number),Toast.LENGTH_SHORT).show();
          }
          break;
        case BOOLEAN:
          if((Boolean)newValue)
            opt.setValue("true");
          else
            opt.setValue("false");
          return true;
        case PORT:
          try {
            int res = Integer.parseInt((String)newValue);
            if(res <= 0 || res > 65535)
              throw new RuntimeException();
            opt.setValue(""+res);
            return true;
          } catch ( NumberFormatException e) {
            Toast.makeText(getApplicationContext(),getString(R.string.pref_err_invalid_number),Toast.LENGTH_SHORT).show();
          } catch (RuntimeException e) {
            Toast.makeText(getApplicationContext(),getString(R.string.invalid_port),Toast.LENGTH_SHORT).show();
          }
          break;
      }
      return false;
    }
  };


  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    // no actionbar in preferenceactivity
    //getActionBar().setDisplayHomeAsUpEnabled(true);

    setPreferenceScreen(createPreferenceHierarchy());
  }

  private PreferenceScreen createPreferenceHierarchy() {
    // Root
    PreferenceScreen root = getPreferenceManager().createPreferenceScreen(this);

    String title = null;
    PreferenceCategory cat_required = new PreferenceCategory(this);
    PreferenceCategory cat_general = new PreferenceCategory(this);
    PreferenceCategory cat_advanced = new PreferenceCategory(this);
    PreferenceCategory cat_evasion = new PreferenceCategory(this);
    ArrayList<Preference> required = new ArrayList<Preference>();
    ArrayList<Preference> general = new ArrayList<Preference>();
    ArrayList<Preference> advanced = new ArrayList<Preference>();
    ArrayList<Preference> evasion = new ArrayList<Preference>();

    Payload payload = System.getCurrentPayload();
    MsfExploit exploit = (MsfExploit) System.getCurrentExploit();
    System.setCurrentPayload(null);
    System.setCurrentExploit(null);

    if(payload != null) {
      options = payload.getOptions();
      title = payload.toString();
    } else if(exploit!=null) {
      options = exploit.getOptions();
      title = exploit.toString();
    }

    if(options==null) {
      options = new ArrayList<Option>();
      title = getString(R.string.error);
      String error_message;

      if(exploit!=null)
        error_message = String.format("cannot retrieve options for '%s'", exploit.getName());
      else if(payload!=null)
        error_message = String.format("cannot retrieve options for '%s'", payload.getName());
      else
        error_message = "called without Payload or MsfExploit";

      Logger.error(error_message);
    }

    setTitle(title + " > " + getString(R.string.menu_settings));
    cat_required.setTitle(R.string.required);
    cat_general.setTitle(R.string.pref_general);
    cat_advanced.setTitle(R.string.pref_advanced);
    cat_evasion.setTitle(R.string.evasion);

    for(Option opt : options) {
      Preference item = null;
      int inputType = 0;

      switch (opt.getType()) {
        case ADDRESS:
        case STRING:
        case PATH:
        case INTEGER:
        case PORT:
          item = new EditTextPreference(this);
          item.setTitle(opt.getName());
          ((EditTextPreference)item).setDialogTitle(opt.getName());
          ((EditTextPreference)item).setDialogMessage(opt.getDescription());
          item.setSummary(opt.getDescription());
          item.setKey(opt.getName());
          item.setDefaultValue(opt.getValue());
          break;
        case BOOLEAN:
          item = new CheckBoxPreference(this);
          item.setTitle(opt.getName());
          item.setKey(opt.getName());
          item.setSummary(opt.getDescription());
          ((CheckBoxPreference)item).setChecked(opt.getValue().equals("true"));
          break;
        case ENUM:
          item = new ListPreference(this);
          ((ListPreference)item).setEntries(opt.getEnum());
          ((ListPreference)item).setEntryValues(opt.getEnum());
          ((ListPreference)item).setDialogTitle(opt.getName());
          ((ListPreference)item).setValue(opt.getValue());
          item.setKey(opt.getName());
          item.setTitle(opt.getName());
          item.setSummary(opt.getDescription());
          break;
      }

      switch (opt.getType()) {
        case ADDRESS:
          inputType=InputType.TYPE_CLASS_PHONE;
          break;
        case PATH:
        case STRING:
          inputType=InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
          break;
        case PORT:
        case INTEGER:
          inputType=InputType.TYPE_CLASS_NUMBER;
          break;
      }

      if(inputType!=0)
        ((EditTextPreference)item).getEditText().setInputType(inputType);

      if(opt.isAdvanced())
        advanced.add(item);
      else if(opt.isRequired())
        required.add(item);
      else if(opt.isEvasion())
        evasion.add(item);
      else
        general.add(item);

      if (item != null) {
        item.setOnPreferenceChangeListener(listener);
      }
    }

    if(required.size()>0) {
      root.addPreference(cat_required);
      for(Preference i : required)
        cat_required.addPreference(i);
    }
    if(general.size()>0) {
      root.addPreference(cat_general);
      for(Preference i : general)
        cat_general.addPreference(i);
    }
    if(advanced.size()>0) {
      root.addPreference(cat_advanced);
      for(Preference i : advanced)
        cat_advanced.addPreference(i);
    }
    if(evasion.size()>0) {
      root.addPreference(cat_evasion);
      for(Preference i : evasion)
        cat_evasion.addPreference(i);
    }

    return root;
  }
}