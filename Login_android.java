package com.example.pallavibhandari.myapplication;

import android.net.Uri;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.EditText;
import android.widget.Button;
import android.view.View;
import android.text.TextUtils;
import android.content.Intent;
public class Login extends AppCompatActivity {
    EditText TheUsername,ThePassword;
    Button TheSubmit;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);
        TheUsername=findViewById(R.id.TheUsername);
        ThePassword= findViewById(R.id.ThePassword);
        TheSubmit = findViewById(R.id.TheSubmit);

        TheSubmit.setOnClickListener(new View.OnClickListener(){

        public void onClick(View v) {
         String unString = TheUsername.getText().toString();
         String passString = ThePassword.getText().toString();
         if(TextUtils.isEmpty(unString)){
              TheUsername.setError("Please fill the field");
         }
         else if (TextUtils.isEmpty(passString))
         {

             ThePassword.setError("Please Enter The Password");
          }

          else
         {
             String userId= "ABC34";
             /*----------To go to new activity----------*/
        // Intent Home= new Intent(Login.this,Home.class);
         //Home.putExtra("userId",userId);
         //startActivity(Home);

             /*-----------to go to settings-----------*/
             //Intent GoToSettings=new Intent(Settings.ACTION_SETTINGS);
             //startActivity(GoTOSettings);

             /*----------To Open Contacts App----------*/
             Uri aNewNumber= Uri.parse("tel: 70236146556");
             // Telling the intent what to do and what to dial
             Intent openContacts=new Intent(Intent.ACTION_DIAL, aNewNumber);
             //starting contacts application
             startActivity(openContacts);
        }

        }});

    }
}

