/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * TAKEN FROM Brad Greco Directory Picker http://www.bgreco.net/
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
package org.csploit.android.gui;

import android.app.ListActivity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;

import org.csploit.android.R;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;

public class DirectoryPicker extends ListActivity{
  public static final String START_DIR = "startDir";
  public static final String ONLY_DIRS = "onlyDirs";
  public static final String SHOW_HIDDEN = "showHidden";
  public static final String CHOSEN_DIRECTORY = "chosenDir";
  public static final String AFFECTED_PREF = "prefKey";
  public static final int PICK_DIRECTORY = 1212;
  private File dir;
  private boolean showHidden = false;
  private boolean onlyDirs = true;
  private String  mAffectedPref = null;

  @Override
  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);

    String preferredStartDir = null;

    Bundle extras = getIntent().getExtras();
    dir = Environment.getExternalStorageDirectory();

    if(extras != null){
      preferredStartDir = extras.getString(START_DIR);
      showHidden = extras.getBoolean(SHOW_HIDDEN, false);
      onlyDirs = extras.getBoolean(ONLY_DIRS, true);
      mAffectedPref = extras.getString(AFFECTED_PREF);

      if(preferredStartDir != null){
        File startDir = new File(preferredStartDir);
        if(startDir.isDirectory()){
          dir = startDir;
        }
      }
    }

    setContentView(R.layout.dirpicker_chooser_list);
    setTitle(dir.getAbsolutePath());
    Button btnChoose = (Button) findViewById(R.id.btnChoose);
    String name = dir.getName();
    if(name.length() == 0)
      name = "/";
    btnChoose.setText(getString(R.string.choose_dir) + "'" + name + "'");
    btnChoose.setOnClickListener(new View.OnClickListener(){
      public void onClick(View v){
        returnDir(dir.getAbsolutePath());
      }
    });

    ListView lv = getListView();
    lv.setTextFilterEnabled(true);

    if(!dir.canRead()){
      Context context = getApplicationContext();
      String msg = getString(R.string.could_not_read_folder);
      if(context != null) Toast.makeText(context, msg, Toast.LENGTH_LONG).show();
      return;
    }

    final ArrayList<File> files = filter(dir.listFiles(), onlyDirs,
      showHidden);
    String[] names = names(files);
    setListAdapter(new ArrayAdapter<String>(this, R.layout.dirpicker_list_item, names));

    lv.setOnItemClickListener(new OnItemClickListener(){
      public void onItemClick(AdapterView<?> parent, View view,
                              int position, long id){
        if(!files.get(position).isDirectory())
          return;
        String path = files.get(position).getAbsolutePath();
        Intent intent = new Intent(DirectoryPicker.this,
          DirectoryPicker.class);
        intent.putExtra(DirectoryPicker.START_DIR, path);
        intent.putExtra(DirectoryPicker.SHOW_HIDDEN, showHidden);
        intent.putExtra(DirectoryPicker.ONLY_DIRS, onlyDirs);
        intent.putExtra(DirectoryPicker.AFFECTED_PREF, mAffectedPref);
        startActivityForResult(intent, PICK_DIRECTORY);
      }
    });
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data){
    if(requestCode == PICK_DIRECTORY && resultCode == RESULT_OK){
      Bundle extras = data.getExtras();
      String path = (String) (extras != null ? extras.get(DirectoryPicker.CHOSEN_DIRECTORY) : null);
      returnDir(path);
    }
  }

  private void returnDir(String path){
    Intent result = new Intent();
    result.putExtra(CHOSEN_DIRECTORY, path);
    result.putExtra(AFFECTED_PREF, mAffectedPref);
    setResult(RESULT_OK, result);
    finish();
  }

  public ArrayList<File> filter(File[] file_list, boolean onlyDirs, boolean showHidden){
    ArrayList<File> files = new ArrayList<File>();

    if(file_list != null){
      for(File file : file_list){
        if(onlyDirs && !file.isDirectory())
          continue;
        if(!showHidden && file.isHidden())
          continue;
        files.add(file);
      }
      Collections.sort(files);
    }

    return files;
  }

  public String[] names(ArrayList<File> files){
    String[] names = new String[files.size()];
    int i = 0;
    for(File file : files){
      names[i] = file.getName();
      i++;
    }
    return names;
  }
}