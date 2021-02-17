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
/** Added Jan 23 2003 Blas Rodriguez Somoza
CS_737, CS_775, CS_858, CS_862, CS_864, CS_866, CS_869
*/
#ifndef INTL_CHARSETS_H
#define INTL_CHARSETS_H

#define DEFAULT_ATTACHMENT_CHARSET	CS_NONE

#define   CS_NONE                0	/* No Character Set */
#define   CS_BINARY              1	/* BINARY BYTES     */
#define   CS_ASCII               2	/* ASCII            */
#define   CS_UNICODE_FSS         3	/* UNICODE in FSS format */
#define   CS_UTF8                4	/* UTF-8 */

#define   CS_SJIS                5	/* SJIS             */
#define   CS_EUCJ                6	/* EUC-J            */

#define   CS_JIS_0208            7	/* JIS 0208; 1990   */
#define   CS_UNICODE_UCS2        8	/* UNICODE v 1.10   */

#define   CS_DOS_737             9
#define   CS_DOS_437            10	/* DOS CP 437       */
#define   CS_DOS_850            11	/* DOS CP 850       */
#define   CS_DOS_865            12	/* DOS CP 865       */
#define   CS_DOS_860            13	/* DOS CP 860       */
#define   CS_DOS_863            14	/* DOS CP 863       */

#define   CS_DOS_775            15
#define   CS_DOS_858            16
#define   CS_DOS_862            17
#define   CS_DOS_864            18

#define   CS_NEXT               19	/* NeXTSTEP OS native charset */

#define   CS_ISO8859_1         21	/* ISO-8859.1       */
#define   CS_ISO8859_2         22	/* ISO-8859.2       */
#define   CS_ISO8859_3         23	/* ISO-8859.3       */
#define   CS_ISO8859_4         34	/* ISO-8859.4       */
#define   CS_ISO8859_5         35	/* ISO-8859.5       */
#define   CS_ISO8859_6         36	/* ISO-8859.6       */
#define   CS_ISO8859_7         37	/* ISO-8859.7       */
#define   CS_ISO8859_8         38	/* ISO-8859.8       */
#define   CS_ISO8859_9         39	/* ISO-8859.9       */
#define   CS_ISO8859_13        40	/* ISO-8859.13      */

#define   CS_KSC5601            44	/* KOREAN STANDARD 5601 */

#define   CS_DOS_852            45	/* DOS CP 852   */
#define   CS_DOS_857            46	/* DOS CP 857   */
#define   CS_DOS_861            47	/* DOS CP 861   */

#define   CS_DOS_866            48
#define   CS_DOS_869            49

#define   CS_CYRL               50
#define   CS_WIN1250            51	/* Windows cp 1250  */
#define   CS_WIN1251            52	/* Windows cp 1251  */
#define   CS_WIN1252            53	/* Windows cp 1252  */
#define   CS_WIN1253            54	/* Windows cp 1253  */
#define   CS_WIN1254            55	/* Windows cp 1254  */

#define	  CS_BIG5               56	/* Big Five unicode cs */
#define	  CS_GB2312             57	/* GB 2312-80 cs */

#define   CS_WIN1255            58	/* Windows cp 1255  */
#define   CS_WIN1256            59	/* Windows cp 1256  */
#define   CS_WIN1257            60	/* Windows cp 1257  */

#define   CS_UTF16              61	/* UTF-16 */
#define   CS_UTF32              62	/* UTF-32 */

#define   CS_KOI8R              63	/* Russian KOI8R */
#define   CS_KOI8U              64	/* Ukrainian KOI8U */

#define   CS_WIN1258            65	/* Windows cp 1258  */

#define   CS_TIS620             66	/* TIS620 */
#define   CS_GBK                67	/* GBK */
#define   CS_CP943C             68	/* CP943C */

#define   CS_GB18030            69	// GB18030

#define   CS_dynamic           127	// Pseudo number for runtime charset

#endif /* INTL_CHARSETS_H */
