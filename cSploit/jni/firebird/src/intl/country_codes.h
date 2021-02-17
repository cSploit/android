/* 93-Jan-05
 * CC_<country> ===> CountryCode_<country>
 * Assigned codes are related to telephone system country dialling codes
 * but not exact, for instance FRENCHCAN == CANADA has a unique
 * ID, but shares a dialling code with US.  Former parts of YUGOSLAVIA
 * can be assigned codes, but don't have their own dialling codes.
 * INTL is a "generic International" code, not corresponding to any
 * Territory.
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

#ifndef INTL_COUNTRY_CODES_H
#define INTL_COUNTRY_CODES_H

#define   CC_C                     0	/* 'C' Language locale */

#define   CC_US                    1	/* USA              */
#define   CC_FRENCHCAN             2	/* FRENCHCAN        */
#define   CC_LATINAMER             3	/* LATINAMER        */
#define   CC_NEDERLANDS            31	/* NEDERLANDS       */
#define   CC_BELGIUM               32	/* BELGIUM          */
#define   CC_FRANCE                33	/* FRANCE           */
#define   CC_SPAIN                 34	/* SPAIN            */
#define   CC_HUNGARY               36	/* HUNGARY          */
#define   CC_YUGOSLAVIA            38	/* YUGOSLAVIA       */
#define   CC_ITALY                 39	/* ITALY            */
#define   CC_SWITZ                 41	/* SWITZ            */
#define   CC_CZECH                 42	/* CZECH            */
#define   CC_UK                    44	/* UK               */
#define   CC_DENMARK               45	/* DENMARK          */
#define   CC_SWEDEN                46	/* SWEDEN           */
#define   CC_NORWAY                47	/* NORWAY           */
#define   CC_POLAND                48	/* POLAND           */
#define   CC_GERMANY               49	/* GERMANY          */
#define   CC_BRAZIL                55	/* BRAZIL           */
#define   CC_INTL                  61	/* INTL             */
#define   CC_JAPAN                 81	/* JAPAN            */
#define   CC_PORTUGAL              351	/* PORTUGAL         */
#define   CC_ICELAND               354	/* ICELAND          */
#define   CC_FINLAND               358	/* FINLAND          */
#define   CC_KOREA                 82	/* KOREA            */
#define   CC_GREECE                30	/* TURKEY           */
#define   CC_TURKEY                90	/* TURKEY           */
#define   CC_RUSSIA                07	/* RUSSIA           */
#define   CC_LITHUANIA             370	/* LITHUANIA        */

#define   CC_NORDAN                92	/* NORDAY + DENMARK */
#define   CC_SWEDFIN               404	/* SWEDIN + FINLAND */

#endif /* INTL_COUNTRY_CODES_H */
