package org.csploit.android.gui;

import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.core.Logger;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;

public class FileEdit extends ActionBarActivity {
    private Button mCmdSave = null;
    private EditText mFileEditText = null;
    public final static String KEY_FILEPATH = "FilePath";
    private String mPath = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.file_edit);
        setTitle("");

        mCmdSave = (Button) findViewById(R.id.cmdSave);
        mFileEditText = (EditText) findViewById(R.id.fileText);
        mFileEditText.setHorizontallyScrolling(true);


        if (getIntent() != null && getIntent().getExtras() != null && getIntent().getExtras().getString(KEY_FILEPATH) != null) {
            mPath = getFilesDir().getAbsolutePath() + getIntent().getExtras().getString(KEY_FILEPATH);
            Logger.info("Path: " + mPath);
            mFileEditText.setText(loadFile(mPath));
        }
        else
            Toast.makeText(this, "No file path provided, nothing to load, nothing to save", Toast.LENGTH_LONG).show();


        mCmdSave.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mPath != null)
                    saveFile(mFileEditText.getText().toString(), mPath);
                else
                    Toast.makeText(view.getContext(), "No file path provided, nothing to load, nothing to save", Toast.LENGTH_LONG).show();
            }
        });
    }

    public String loadFile (String _path) {
        String _line = "";
        String _str_line = "";
        BufferedReader inputReader = null;

        if (_path == null){
            Toast.makeText(this, "Error: No file path provided", Toast.LENGTH_LONG).show();
            return "";
        }

        try {
            inputReader = new BufferedReader(new FileReader(_path));
            while ((_line = inputReader.readLine()) != null) {
                _str_line += _line + "\n";
            }
        }
        catch (Exception e){
            Toast.makeText(this, "Error loading \"" + _path + "\"\n\n" + e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
            Logger.error("loadFile error: " + e.getLocalizedMessage());
        }
        finally {
            try {
                if (inputReader != null)
                    inputReader.close();
            }
            catch (Exception e){}
        }

        return _str_line;
    }

    public boolean saveFile (String _file_text, String _path){
        FileOutputStream fos = null;
        try{
            Logger.info("saveDNSList() saving dnss to: " + _path);
            File f = new File(_path);
            fos = new FileOutputStream(f);
            fos.write(_file_text.getBytes());

            Toast.makeText(this, "Saved", Toast.LENGTH_SHORT).show();

            return true;
        }
        catch (Exception e){
            Toast.makeText(this, "Error saving \"" + _path + "\"\n\n" + e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
        }
        finally {
            try {
                if (fos != null)
                    fos.close();
            }
            catch (Exception e){

            }
        }

        return false;
    }

	@Override
	public void onBackPressed() {
		super.onBackPressed();
		overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
	}
}
