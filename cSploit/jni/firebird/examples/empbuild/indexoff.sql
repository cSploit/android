/*
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
alter index CUSTNAMEX  inactive;
alter index CUSTREGION  inactive;
alter index BUDGETX  inactive;
alter index NAMEX  inactive;
alter index MAXSALX  inactive;
alter index MINSALX  inactive;
alter index PRODTYPEX  inactive;
alter index CHANGEX  inactive;
alter index UPDATERX  inactive;
alter index NEEDX  inactive;
alter index QTYX  inactive;
alter index SALESTATX  inactive;
ALTER TRIGGER set_emp_no INACTIVE;
ALTER TRIGGER set_cust_no INACTIVE;
ALTER TRIGGER post_new_order INACTIVE;
