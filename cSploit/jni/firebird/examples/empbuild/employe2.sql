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
set sql dialect 3;
create database "employe2.fdb";
/*
 *	Currency cross rates:  convert one currency type into another.
 *
 *	Ex.  5 U.S. Dollars = 5 * 1.3273 Canadian Dollars
 */

CREATE TABLE cross_rate
(
    from_currency   VARCHAR(10) NOT NULL,
    to_currency     VARCHAR(10) NOT NULL,
    conv_rate       FLOAT NOT NULL,	
    update_date     DATE,

    PRIMARY KEY (from_currency, to_currency)
);

INSERT INTO cross_rate VALUES ('Dollar', 'CdnDlr',  1.0027,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Dollar', 'Yen',     79.2400, '11/09/2012');
INSERT INTO cross_rate VALUES ('Dollar', 'SFranc',  0.9495,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Dollar', 'Pound',   0.6272,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Pound',  'Euro',    1.2542,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Pound',  'Yen',     126.3320,'11/09/2012');
INSERT INTO cross_rate VALUES ('Yen',    'Pound',   0.0079,  '11/09/2012');
INSERT INTO cross_rate VALUES ('CdnDlr', 'Dollar',  0.9973,  '11/09/2012');
INSERT INTO cross_rate VALUES ('CdnDlr', 'Euro',    0.7857,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Euro', 'Dollar',    1.2716,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Dollar', 'Euro',    0.7878,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Ruble', 'Euro',     0.0249,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Euro', 'Ruble',     40.1378, '11/09/2012');
INSERT INTO cross_rate VALUES ('Ruble', 'Dollar',   0.0316,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Dollar', 'Ruble',   31.5355, '11/09/2012');
INSERT INTO cross_rate VALUES ('RLeu', 'Dollar',    0.2811,  '11/09/2012');
INSERT INTO cross_rate VALUES ('Euro', 'RLeu',      4.5281,  '11/09/2012');


