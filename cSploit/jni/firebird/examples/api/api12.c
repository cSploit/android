/*
 *    Program type:  API Interface
 *
 *    Description:
 *        This program has several active transactions:
 *
 *        Sales order records are entered continuously (transaction 1).
 *
 *        If it is discovered during transaction 1, that the customer
 *        placing the order is new, then a new customer record must be
 *        added (transaction 2).
 *
 *        If the customer record uses a country that does not exist
 *        in the 'country' table, a new country record is added (transaction 3).
 *
 *        Transaction 2 can be committed after the country is added.
 *        Transaction 1 can be committed after the customer record is added.
 *
 *        Transactions 1, 2, and 3 can be undone individually, if the user
 *        decides not to save the sales, customer, or country changes.
 *        If transaction 3 is undone, transactions 1 and 2 must be undone.
 *        If transaction 2 is undone, transaction 1 must be undone.
 *
 *        In addition, several independent transactions, selecting the
 *        customer number and the country records, take place during
 *        the three update transactions.
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include <stdlib.h>
#include <string.h>
#include <ibase.h>
#include <stdio.h>
#include "example.h"

#define BUFLEN        512
    
char* more_orders (void);
int do_trans (void);
int cleanup (void);

char    *customer    = "Maritime Museum";
char    *country    = "Cayman Islands";
char    *currency    = "CmnDlr";

isc_db_handle       db = NULL;
isc_tr_handle       sales_trans = NULL,
                    cust_trans = NULL,
                    cntry_trans = NULL,
                    trans = NULL;
ISC_STATUS_ARRAY    status;

static char *Sales[] = {"V88005", 0};
int Inp_ptr = 0;

char    *trans_str = "SET TRANSACTION ISOLATION LEVEL READ COMMITTED";


int main (int argc, char** argv)
{
    char empdb[128];

    if (argc > 1)
        strcpy(empdb, argv[1]);
    else
        strcpy(empdb, "employee.fdb");

    /* Zero the transaction handles. */

    if (isc_attach_database(status, 0, empdb, &db, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    /* Do the updates */
    do_trans();
    if (trans)
        isc_rollback_transaction(status, &trans);
    if (cust_trans)
        isc_rollback_transaction(status, &cust_trans);
    if (cntry_trans)
        isc_rollback_transaction(status, &cntry_trans);
    if (sales_trans)
        isc_rollback_transaction(status, &sales_trans);

    /* Remove them again */
    cleanup();

    if (trans)
        isc_rollback_transaction(status, &trans);

    isc_detach_database(status, &db);

    return 0;
}

/* 
 * Function does all the work.
*/
int do_trans (void)
{                   
    long            cust_no;
    char            sales_str[BUFLEN + 1];
    char            cust_str[BUFLEN + 1];
    char            cntry_str[BUFLEN + 1];
    char            sel_str[BUFLEN + 1];
    XSQLDA          *sqlda1, *sqlda2, *sqlda3;
    isc_stmt_handle stmt0 = NULL,
                    stmt1 = NULL,
                    stmt2 = NULL;
    short           flag0 = 0, flag1 = 0;
    long            fetch_stat;
    long            sqlcode;
                    
    /* Prepare a query for fetching data.  Make it read committed, so you
     * can see your own updates.  
     */

    if (isc_dsql_execute_immediate(status, &db, &trans, 0, trans_str,
                                   1, NULL))
    {
        ERREXIT(status, 1)
    }

    sprintf(sel_str, "SELECT cust_no FROM customer WHERE customer = '%s'",
            customer);

    if (isc_dsql_allocate_statement(status, &db, &stmt0))
    {
        ERREXIT(status, 1)
    }

    sqlda1 = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    sqlda1->sqln = 1;
    sqlda1->version = 1;

    if (isc_dsql_prepare(status, &trans, &stmt0, 0, sel_str, 1, sqlda1))
    {
        ERREXIT(status, 1)
    }

    sqlda1->sqlvar[0].sqldata = (char *) &cust_no;
    sqlda1->sqlvar[0].sqltype = SQL_LONG + 1;
    sqlda1->sqlvar[0].sqlind  = &flag0;

    /* Prepare a query for checking if a country exists. */

    sprintf(sel_str, "SELECT country FROM country WHERE country = '%s'",
            country);

    sqlda2 = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    sqlda2->sqln = 1;
    sqlda2->version = 1;

    if (isc_dsql_allocate_statement(status, &db, &stmt2))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_prepare(status, &trans, &stmt2, 0, sel_str, 1, sqlda2))
    {
        ERREXIT(status, 1)
    }

    sqlda2->sqlvar[0].sqldata = (char *) country;
    sqlda2->sqlvar[0].sqltype = SQL_TEXT + 1;
    sqlda2->sqlvar[0].sqlind  = &flag1;
                             
    /*
     *    Start transaction 1 -- add a sales order.
     *    for a customer. 
     */

    cust_no = 9999;
    /* This transaction is also read committed so it can see the results of
     *  other transactions 
     */
    if (isc_dsql_execute_immediate(status, &db, &sales_trans, 0, trans_str,
                                   1, NULL))
    {
        ERREXIT(status, 1)
    }

    sprintf(sales_str, "INSERT INTO sales (po_number, cust_no, \
            order_status, total_value) VALUES ('V88005', ?, \
            'new', 2000)"); 

    if (isc_dsql_allocate_statement(status, &db, &stmt1))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_prepare(status, &trans, &stmt1, 0, sales_str, 1, NULL))
    {
        ERREXIT(status, 1)
    }
    
    /* Insert parameter (cust_no) used for sales insert */

    sqlda3 = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    sqlda3->sqln = 1;
    sqlda3->version = 1;

    isc_dsql_describe_bind(status, &stmt1, 1, sqlda3);
    sqlda3->sqlvar[0].sqldata = (char *) &cust_no;;
    sqlda3->sqlvar[0].sqlind  = &flag0;

    isc_dsql_execute(status, &sales_trans, &stmt1, 1, sqlda3);
    sqlcode = isc_sqlcode(status);

    if (sqlcode == -530)
    {
        /* Integrity constraint indicates missing primary key*/
        printf ("No customer number %ld -- adding new customer \n", cust_no);
        /*
         *    This a new customer.
         * Start transaction 2 -- add a customer record.
         */
        if (isc_start_transaction(status, &cust_trans, 1, &db, 0, NULL))
        {
            ERREXIT(status, 1)
        }

        sprintf(cust_str, "INSERT INTO customer (customer, country) \
                VALUES ('%s', '%s')", customer, country);
        
        printf("Adding a customer record for %s\n", customer );

        /* Does the customer country exist in the validation table? 
         * Do a lookup this time instead of waiting for the constraint
         * violation.  Because trans is read committed, it will see
         * updates on other transactions.
         */

        if (isc_dsql_execute(status, &trans, &stmt2, 1, NULL))
        {
            ERREXIT(status, 1)
        }

        fetch_stat = isc_dsql_fetch(status, &stmt2, 1, sqlda2);
        
        /*
         *    Country was not found in the validation table.
         *    Start transaction 3 -- add a country record.
         */
        if (fetch_stat == 100L)
        {
            printf("Missing country record, adding %s\n", country);
            if (isc_start_transaction(status, &cntry_trans, 1, &db, 0, NULL))
            {
                ERREXIT (status, 1)
            }

            sprintf(cntry_str, "INSERT INTO country VALUES ('%s', '%s')",
                    country, currency);

            if (isc_dsql_execute_immediate(status, &db, &cntry_trans, 0,
                cntry_str, 1, NULL))
            {
                ERREXIT(status, 1)
            }

            /* This must be committed to be visible */
            isc_commit_transaction(status, &cntry_trans);
        }

        /*
         *    End transaction 2.
         *    Add the customer record, now with a reference.
         */
        if (isc_dsql_execute_immediate(status, &db, &cust_trans, 0, cust_str,
                                       1, NULL))
        {
            ERREXIT(status, 1)
        }

        /* Commit to make this reference visible */
        if (isc_commit_transaction(status, &cust_trans))
        {
            ERREXIT(status, 1)
        }

        /* Lookup the new cust_no for this record */
        if (isc_dsql_execute(status, &trans, &stmt0, 1, NULL))
        {
            ERREXIT(status, 1)
        }

        if (!isc_dsql_fetch(status, &stmt0, 1, sqlda1))
            printf("New customer number: %ld\n", cust_no);

        /* Then try to add the sales record again */
        if (isc_dsql_execute(status, &sales_trans, &stmt1, 1, sqlda3))
        {
            ERREXIT(status, 1)
        }
    }

    if (isc_commit_transaction(status, &sales_trans))
    {
        ERREXIT (status, 1)
    }

    printf("Added sales record for V88055\n");
    isc_commit_transaction(status, &trans);

    isc_dsql_free_statement(status, &stmt0, DSQL_close);
    isc_dsql_free_statement(status, &stmt1, DSQL_close);
    isc_dsql_free_statement(status, &stmt2, DSQL_close);
    free(sqlda1);
    free(sqlda2);
    free(sqlda3);

    return 0;
}

/* Cleanup removes all updates that might have been done
 * Make sure to cleanup in reverse order to avoid primary
 * key violations
 */
int cleanup (void)
{
    char del_str[100];

    printf ("Cleaning up...\n");

    if (isc_start_transaction(status, &trans, 1, &db, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    strcpy(del_str, "DELETE FROM SALES WHERE PO_NUMBER =  \"V88005\"");
    if (isc_dsql_execute_immediate(status, &db, &trans, 0, del_str, 1, NULL))
    {
        ERREXIT(status, 1)
    }

    strcpy (del_str, "DELETE FROM CUSTOMER WHERE COUNTRY LIKE \"Cayman%\"");
    if (isc_dsql_execute_immediate(status, &db, &trans, 0, del_str, 1, NULL))
    {
        ERREXIT (status, 1)
    }
    
    strcpy (del_str, "DELETE FROM COUNTRY WHERE COUNTRY LIKE \"Cayman%\"");
    if (isc_dsql_execute_immediate(status, &db, &trans, 0, del_str, 1, NULL))
    {
        ERREXIT(status, 1)
    }

    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 1)
    }

    return 0;
}

/*
 *    Return the order number for the next sales order to be entered.
 */
char* more_orders (void)
{
    return Sales[Inp_ptr++];
}

