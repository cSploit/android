package it.evilsocket.dsploit.gui;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.view.MenuItem;

import java.util.ArrayList;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.*;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.SpinnerDialog;
import it.evilsocket.dsploit.net.metasploit.MsfExploit;
import it.evilsocket.dsploit.net.metasploit.Option;
import it.evilsocket.dsploit.net.metasploit.Payload;

/**
 * activity fo setting exploit options.
 */

public class MsfPreferences extends SherlockActivity {

  private ListView 		          mListView     = null;
  private OptionAdapter  mAdapter      = null;
  private ArrayList<Object> objectsList = new ArrayList<Object>();
  private static final int ITEM_VIEW_TYPE_OPTION = 0;
  private static final int ITEM_VIEW_TYPE_SEPARATOR = 1;
  private static final int ITEM_VIEW_TYPE_COUNT = 2;

  private final AdapterView.OnItemClickListener clickListener = new AdapterView.OnItemClickListener() {
    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
      //we are sure that we are dealing with an option ( look at OptionAdapter.isEnabled )
      final Option opt = (Option)mAdapter.getItem(position);
      if(opt.getType() == Option.types.ENUM ){
        final String[] enums = opt.getEnum();
        String currentValue = opt.getValue();
        if(currentValue.isEmpty()){
          new SpinnerDialog(opt.getName(), opt.getDescription(), enums, MsfPreferences.this, new SpinnerDialog.SpinnerDialogListener() {
            @Override
            public void onItemSelected(int index) {
              opt.setValue(enums[index]);
            }
          }).show();
        } else {
          int i;
          for(i=0;i<enums.length && !(enums[i].equals(currentValue));i++);
          if(i==enums.length && i > 0) {
            StringBuilder debug = new StringBuilder();
            debug.append(enums[0]);
            for(i=1;i<enums.length;i++)
              debug.append(", " + enums[i]);
            Logger.error("cannot find " + currentValue + "in (" + debug + ")");
            i=0;
          } else {
            Logger.error("empty enum! option name: "+opt.getName()+", option value: "+opt.getValue());
            return;
          }
          new SpinnerDialog(opt.getName(), opt.getDescription(), enums, i, MsfPreferences.this, new SpinnerDialog.SpinnerDialogListener() {
            @Override
            public void onItemSelected(int index) {
              opt.setValue(enums[index]);
            }
          }).show();
        }
      } else {
        new InputDialog(opt.getName(),opt.getDescription(), opt.getValue(), true, false, MsfPreferences.this, new InputDialog.InputDialogListener() {
          @Override
          public void onInputEntered(String input) {
            try {
              opt.setValue(input);
            } catch (Exception e) {
              Logger.error(e.toString());
              new ErrorDialog(getString(R.string.error),e.getLocalizedMessage(),MsfPreferences.this).show();
            }
          }
        }).show();
      }
    }
  };

  private class OptionAdapter extends BaseAdapter
  {
    private class option_holder {
      TextView    itemTitle;
      TextView    itemDescription;
    }

    private class separator_holder {
      ImageView itemIcon;
      TextView  itemTitle;
    }

    @Override
    public int getCount() {
      return objectsList.size();
    }

    @Override
    public Object getItem(int position) {
      return objectsList.get(position);
    }

    @Override
    public long getItemId(int position) {
      return position;
    }

    @Override
    public int getViewTypeCount() {
      return ITEM_VIEW_TYPE_COUNT;
    }

    @Override
    public int getItemViewType(int position) {
      return (objectsList.get(position) instanceof String) ? ITEM_VIEW_TYPE_SEPARATOR : ITEM_VIEW_TYPE_OPTION;
    }

    @Override
    public boolean isEnabled(int position) {
      // A separator cannot be clicked !
      return getItemViewType(position) != ITEM_VIEW_TYPE_SEPARATOR;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
      Object holder;
      final boolean isSeparator = getItemViewType(position) == ITEM_VIEW_TYPE_SEPARATOR;

      // First, let's create a new convertView if needed. You can also
      // create a ViewHolder to speed up changes if you want ;)
      if (convertView == null) {
        int layout;

        if(isSeparator) {
          layout = R.layout.exploit_preferences_separator;
          holder = new separator_holder();
        } else {
          layout = R.layout.exploit_preferences_item;
          holder = new option_holder();
        }
        LayoutInflater inflater = ( LayoutInflater )MsfPreferences.this.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
        convertView = inflater.inflate( layout, parent, false);

        if(isSeparator) {
          ((separator_holder)holder).itemTitle = ( TextView )convertView.findViewById( R.id.itemTitle );
          ((separator_holder)holder).itemIcon = ( ImageView )convertView.findViewById( R.id.itemIcon);
        } else {
          ((option_holder)holder).itemTitle = ( TextView )convertView.findViewById( R.id.itemTitle );
          ((option_holder)holder).itemDescription = ( TextView )convertView.findViewById( R.id.itemDescription );
        }

        convertView.setTag(holder);
      } else {
        holder = convertView.getTag();
      }

      // We can now fill the list item view with the appropriate data.
      if (isSeparator) {
        ((separator_holder)holder).itemTitle.setText((String)getItem(position));
        //TODO: separator icon
      } else {
        final Option opt = (Option) getItem(position);
        ((option_holder)holder).itemTitle.setText(opt.getName());
        ((option_holder)holder).itemDescription.setText(opt.getValue());
      }

      return convertView;
    }

  }

  @Override
  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    setContentView(R.layout.exploit_preferences);
    getSupportActionBar().setDisplayHomeAsUpEnabled(true);

    mListView= (ListView)findViewById( android.R.id.list);
    mAdapter = new OptionAdapter();

    Option[] options = null;
    String title;
    ArrayList<Option> required = new ArrayList<Option>();
    ArrayList<Option> general = new ArrayList<Option>();
    ArrayList<Option> advanced = new ArrayList<Option>();
    ArrayList<Option> evasion = new ArrayList<Option>();

    Payload payload = System.getCurrentPayload();
    MsfExploit exploit = (MsfExploit) System.getCurrentExploit();

    if(payload != null) {
      options = payload.getOptions();
      title = payload.toString();
    } else if(exploit!=null) {
      options = exploit.getOptions();
      title = exploit.toString();
    } else {
      options = new Option[0];
      title = getString(R.string.error);
      Logger.error("called without Payload or MsfExploit");
    }

    for(Option opt : options) {
      if(opt.isAdvanced())
        advanced.add(opt);
      else if(opt.isRequired())
        required.add(opt);
      else if(opt.isEvasion())
        evasion.add(opt);
      else
        general.add(opt);
    }

    if(!required.isEmpty()) {
      objectsList.add(getString(R.string.required));
      objectsList.addAll(required);
    }
    if(!general.isEmpty()) {
      objectsList.add(getString(R.string.pref_general));
      objectsList.addAll(general);
    }
    if(!advanced.isEmpty()) {
      objectsList.add(getString(R.string.pref_advanced));
      objectsList.addAll(advanced);
    }
    if(!evasion.isEmpty()) {
      objectsList.add(getString(R.string.evasion));
      objectsList.addAll(evasion);
    }

    // force Garbage Collector to munmap(2)
    required.clear();
    general.clear();
    advanced.clear();
    evasion.clear();
    required = general = advanced = evasion = null;

    setTitle(title + " > " + getString(R.string.menu_settings));

    mListView.setAdapter( mAdapter );

    mListView.setOnItemClickListener(clickListener);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    switch(item.getItemId()){
      case android.R.id.home:

        onBackPressed();

        return true;

      default:
        return super.onOptionsItemSelected(item);
    }
  }

  @Override
  public void onBackPressed(){
    super.onBackPressed();
    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
    System.setCurrentExploit(null);
    System.setCurrentPayload(null);
  }
}