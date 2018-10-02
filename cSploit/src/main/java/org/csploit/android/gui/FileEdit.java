package org.csploit.android.gui;

import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import org.csploit.android.R;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;

import androidx.appcompat.app.AppCompatActivity;

public class FileEdit extends AppCompatActivity {
    private EditText mFileEditText = null;
    public final static String KEY_FILEPATH = "FilePath";
    private String mPath = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.file_edit);
        setTitle("");

        Button mCmdSave = findViewById(R.id.cmdSave);
        mFileEditText = findViewById(R.id.fileText);
        mFileEditText.setHorizontallyScrolling(true);


        if (getIntent() != null && getIntent().getExtras() != null &&
                getIntent().getExtras().getString(KEY_FILEPATH) != null) {
            mPath = getFilesDir().getAbsolutePath() + getIntent().getExtras().getString(KEY_FILEPATH);
            mFileEditText.setText(loadFile(mPath));
        }


        mCmdSave.setOnClickListener(view -> {
            if (mPath != null)
                saveFile(mFileEditText.getText().toString(), mPath);
        });
    }

    public String loadFile(String _path) {
        final StringBuilder builder = new StringBuilder();

        if (_path == null) {
            Toast.makeText(this, "Error: No file path provided", Toast.LENGTH_LONG).show();
            return "";
        }

        try (BufferedReader inputReader = new BufferedReader(new FileReader(_path))) {

            String _line;
            while ((_line = inputReader.readLine()) != null) {
                builder.append(_line).append("\n");
            }
        } catch (Exception e) {
            Toast.makeText(this, "Error loading \"" + _path + "\"\n\n" +
                    e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
        }

        return builder.toString();
    }

    public boolean saveFile(String _file_text, String _path) {
        File f = new File(_path);
        try (FileOutputStream fos = new FileOutputStream(f)) {
            fos.write(_file_text.getBytes());

            Toast.makeText(this, getString(R.string.saved), Toast.LENGTH_SHORT).show();

            return true;
        } catch (Exception e) {
            Toast.makeText(this, "Error saving \"" + _path + "\"\n\n" +
                    e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
        }

        return false;
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
        overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
    }
}
