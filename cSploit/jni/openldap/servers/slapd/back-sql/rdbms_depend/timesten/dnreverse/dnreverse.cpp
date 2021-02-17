// Copyright 1997-2014 The OpenLDAP Foundation, All Rights Reserved.
//  COPYING RESTRICTIONS APPLY, see COPYRIGHT file

// (c) Copyright 1999-2001 TimesTen Performance Software. All rights reserved.

//// Note: This file was contributed by Sam Drake of TimesTen Performance
////       Software for use and redistribution as an intregal part of
////       OpenLDAP Software.  -Kdz

#include <stdlib.h>

#include <TTConnectionPool.h>
#include <TTConnection.h>
#include <TTCmd.h>
#include <TTXla.h>

#include <signal.h>

TTConnectionPool pool;
TTXlaConnection  conn;
TTConnection     conn2;
TTCmd            assignDn_ru;
TTCmd            getNullDNs;

//----------------------------------------------------------------------
// This class contains all the logic to be implemented whenever
// the SCOTT.MYDATA table is changed.  This is the table that is
// created by "sample.cpp", one of the other TTClasses demos.
// That application should be executed before this one in order to 
// create and populate the table.
//----------------------------------------------------------------------

class LDAPEntriesHandler: public TTXlaTableHandler {
private:
  // Definition of the columns in the table
  int Id;
  int Dn;
  int Oc_map_id;
  int Parent;
  int Keyval;
  int Dn_ru;

protected:

public:
  LDAPEntriesHandler(TTXlaConnection& conn, const char* ownerP, const char* nameP);
  ~LDAPEntriesHandler();

  virtual void HandleDelete(ttXlaUpdateDesc_t*);
  virtual void HandleInsert(ttXlaUpdateDesc_t*);
  virtual void HandleUpdate(ttXlaUpdateDesc_t*);

  static void ReverseAndUpper(char* dnP, int id, bool commit=true);

};

LDAPEntriesHandler::LDAPEntriesHandler(TTXlaConnection& conn,
				       const char* ownerP, const char* nameP) :
  TTXlaTableHandler(conn, ownerP, nameP)
{
  Id = Dn = Oc_map_id = Parent = Keyval = Dn_ru = -1;

  // We are looking for several particular named columns.  We need to get
  // the ordinal position of the columns by name for later use.

  Id = tbl.getColNumber("ID");
  if (Id < 0) {
    cerr << "target table has no 'ID' column" << endl;
    exit(1);
  }
  Dn = tbl.getColNumber("DN");
  if (Dn < 0) {
    cerr << "target table has no 'DN' column" << endl;
    exit(1);
  }
  Oc_map_id = tbl.getColNumber("OC_MAP_ID");
  if (Oc_map_id < 0) {
    cerr << "target table has no 'OC_MAP_ID' column" << endl;
    exit(1);
  }
  Parent = tbl.getColNumber("PARENT");
  if (Parent < 0) {
    cerr << "target table has no 'PARENT' column" << endl;
    exit(1);
  }
  Keyval = tbl.getColNumber("KEYVAL");
  if (Keyval < 0) {
    cerr << "target table has no 'KEYVAL' column" << endl;
    exit(1);
  }
  Dn_ru = tbl.getColNumber("DN_RU");
  if (Dn_ru < 0) {
    cerr << "target table has no 'DN_RU' column" << endl;
    exit(1);
  }

}

LDAPEntriesHandler::~LDAPEntriesHandler()
{

}

void LDAPEntriesHandler::ReverseAndUpper(char* dnP, int id, bool commit)
{
  TTStatus stat;
  char dn_rn[512];
  int i;
  int j;

  // Reverse and upper case the given DN

  for ((j=0, i = strlen(dnP)-1); i > -1; (j++, i--)) {
    dn_rn[j] = toupper(*(dnP+i));
  }
  dn_rn[j] = '\0';


  // Update the database

  try {
    assignDn_ru.setParam(1, (char*) &dn_rn[0]);
    assignDn_ru.setParam(2, id);
    assignDn_ru.Execute(stat);
  }
  catch (TTStatus stat) {
    cerr << "Error updating id " << id << " ('" << dnP << "' to '" 
	 << dn_rn << "'): " << stat;
    exit(1);
  }

  // Commit the transaction
  
  if (commit) {
    try {
      conn2.Commit(stat);
    }
    catch (TTStatus stat) {
      cerr << "Error committing update: " << stat;
      exit(1);
    }
  }

}



void LDAPEntriesHandler::HandleInsert(ttXlaUpdateDesc_t* p)
{
  char* dnP; 
  int   id;

  row.Get(Dn, &dnP);
  cerr << "DN '" << dnP << "': Inserted ";
  row.Get(Id, &id);

  ReverseAndUpper(dnP, id);

}

void LDAPEntriesHandler::HandleUpdate(ttXlaUpdateDesc_t* p)
{    
  char* newDnP; 
  char* oldDnP;
  char  oDn[512];
  int   id;

  // row is 'old'; row2 is 'new'
  row.Get(Dn, &oldDnP);
  strcpy(oDn, oldDnP);
  row.Get(Id, &id);
  row2.Get(Dn, &newDnP);
  
  cerr << "old DN '" << oDn << "' / new DN '" << newDnP << "' : Updated ";

  if (strcmp(oDn, newDnP) != 0) {	
    // The DN field changed, update it
    cerr << "(new DN: '" << newDnP << "')";
    ReverseAndUpper(newDnP, id);
  }
  else {
    // The DN field did NOT change, leave it alone
  }

  cerr << endl;

}

void LDAPEntriesHandler::HandleDelete(ttXlaUpdateDesc_t* p)
{    
  char* dnP; 

  row.Get(Dn, &dnP);
  cerr << "DN '" << dnP << "': Deleted ";
}




//----------------------------------------------------------------------

int pleaseStop = 0;

extern "C" {
  void
  onintr(int sig)
  {
    pleaseStop = 1;
    cerr << "Stopping...\n";
  }
};

//----------------------------------------------------------------------

int 
main(int argc, char* argv[])
{

  char* ownerP;

  TTXlaTableList list(&conn);	// List of tables to monitor

  // Handlers, one for each table we want to monitor

  LDAPEntriesHandler* sampP = NULL;

  // Misc stuff

  TTStatus stat;

  ttXlaUpdateDesc_t ** arry;

  int records;

  SQLUBIGINT  oldsize;
  int j;

  if (argc < 2) {
    cerr << "syntax: " << argv[0] << " <username>" << endl;
    exit(3);
  }

  ownerP = argv[1];

  signal(SIGINT, onintr);    /* signal for CTRL-C */
#ifdef _WIN32
  signal(SIGBREAK, onintr);  /* signal for CTRL-BREAK */
#endif

  // Before we do anything related to XLA, first we connect
  // to the database.  This is the connection we will use
  // to perform non-XLA operations on the tables.

  try {
    cerr << "Connecting..." << endl;

    conn2.Connect("DSN=ldap_tt", stat);
  }
  catch (TTStatus stat) {
    cerr << "Error connecting to TimesTen: " << stat;
    exit(1);
  }

  try {
    assignDn_ru.Prepare(&conn2,
			"update ldap_entries set dn_ru=? where id=?", 
			"", stat);
    getNullDNs.Prepare(&conn2,
		       "select dn, id from ldap_entries "
			"where dn_ru is null "
			"for update", 
		       "", stat);
    conn2.Commit(stat);
  }
  catch (TTStatus stat) {
    cerr << "Error preparing update: " << stat;
    exit(1);
  }

  // If there are any entries with a NULL reversed/upper cased DN, 
  // fix them now.

  try {
    cerr << "Fixing NULL reversed DNs" << endl;
    getNullDNs.Execute(stat);
    for (int k = 0;; k++) {
      getNullDNs.FetchNext(stat);
      if (stat.rc == SQL_NO_DATA_FOUND) break;
      char* dnP;
      int   id;
      getNullDNs.getColumn(1, &dnP);
      getNullDNs.getColumn(2, &id);
      // cerr << "Id " << id << ", Dn '" << dnP << "'" << endl;
      LDAPEntriesHandler::ReverseAndUpper(dnP, id, false);
      if (k % 1000 == 0) 
        cerr << ".";
    }
    getNullDNs.Close(stat);
    conn2.Commit(stat);
  }
  catch (TTStatus stat) {
    cerr << "Error updating NULL rows: " << stat;
    exit(1);
  }


  // Go ahead and start up the change monitoring application

  cerr << "Starting change monitoring..." << endl;
  try {
    conn.Connect("DSN=ldap_tt", stat);
  }
  catch (TTStatus stat) {
    cerr << "Error connecting to TimesTen: " << stat;
    exit(1);
  }

  /* set and configure size of buffer */
  conn.setXlaBufferSize((SQLUBIGINT) 1000000, &oldsize, stat);
  if (stat.rc) {
    cerr << "Error setting buffer size " << stat << endl;
    exit(1);
  }

  // Make a handler to process changes to the MYDATA table and
  // add the handler to the list of all handlers

  sampP = new LDAPEntriesHandler(conn, ownerP, "ldap_entries");
  if (!sampP) {
    cerr << "Could not create LDAPEntriesHandler" << endl;
    exit(3);
  }
  list.add(sampP);

  // Enable transaction logging for the table we're interested in 

  sampP->EnableTracking(stat);

  // Get updates.  Dispatch them to the appropriate handler.
  // This loop will handle updates to all the tables.

  while (pleaseStop == 0) {
    conn.fetchUpdates(&arry, 1000, &records, stat);
    if (stat.rc) {
      cerr << "Error fetching updates" << stat << endl;
      exit(1);
    }

    // Interpret the updates

    for(j=0;j < records;j++){
      ttXlaUpdateDesc_t *p;

      p = arry[j];

      list.HandleChange(p, stat);

    } // end for each record fetched
    
    if (records) {
      cerr << "Processed " << records << " records\n";
    }

    if (records == 0) {
#ifdef _WIN32
      Sleep(250);
#else
      struct timeval t;
      t.tv_sec = 0;
      t.tv_usec = 250000; // .25 seconds
      select(0, NULL, NULL, NULL, &t);
#endif
    }
  } // end while pleasestop == 0
  

  // When we get to here, the program is exiting.

  list.del(sampP);		// Take the table out of the list 
  delete sampP;

  conn.setXlaBufferSize(oldsize, NULL, stat);

  return 0;

}

