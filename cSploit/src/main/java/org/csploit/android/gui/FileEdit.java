package org.csploit.android.gui;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import org.csploit.android.R;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;

public class FileEdit extends AppCompatActivity {
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
            mFileEditText.setText(loadFile(mPath));
        }


        mCmdSave.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mPath != null)
                    saveFile(mFileEditText.getText().toString(), mPath);
            }
        });
    }

    public String loadFile (String _path) {
        final StringBuilder builder = new StringBuilder();

        BufferedReader inputReader = null;

        if (_path == null){
            Toast.makeText(this, "Error: No file path provided", Toast.LENGTH_LONG).show();
            return "";
        }

        try {
            inputReader = new BufferedReader(new FileReader(_path));

            String _line;
            while ((_line = inputReader.readLine()) != null) {
                builder.append(_line).append("\n");
            }
        }
        catch (Exception e){
            Toast.makeText(this, "Error loading \"" + _path + "\"\n\n" + e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
        }
        finally {
            try {
                if (inputReader != null)
                    inputReader.close();
            }
            catch (Exception e){}
        }

        return builder.toString();
    }

    public boolean saveFile (String _file_text, String _path){
        FileOutputStream fos = null;
        try{
            File f = new File(_path);
            fos = new FileOutputStream(f);
            fos.write(_file_text.getBytes());

            Toast.makeText(this, getString(R.string.saved), Toast.LENGTH_SHORT).show();

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
