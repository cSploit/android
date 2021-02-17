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
alter index CUSTNAMEX  active;
alter index CUSTREGION  active;
alter index BUDGETX  active;
alter index NAMEX  active;
alter index MAXSALX  active;
alter index MINSALX  active;
alter index PRODTYPEX  active;
alter index CHANGEX  active;
alter index UPDATERX  active;
alter index NEEDX  active;
alter index QTYX  active;
alter index SALESTATX  active;
ALTER TRIGGER set_emp_no ACTIVE;
ALTER TRIGGER set_cust_no ACTIVE;
ALTER TRIGGER post_new_order ACTIVE;
