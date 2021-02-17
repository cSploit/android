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

#include "firebird.h"
#include "cs_w1254.h"

/*
fixed
#include "cs_cyrl.h"
#include "cs_865.h"
#include "cs_863.h"
#include "cs_861.h"
#include "cs_860.h"
#include "cs_857.h"
#include "cs_w1251.h"
#include "cs_437.h"
#include "cs_850.h"
#include "cs_852.h"
#include "cs_w1250.h"
#include "cs_w1252.h"
#include "cs_w1253.h"

fixed with errors
#include "cs_next.h" -- NeXT has two characters mapping to the same
			Unicode (FE & FF).  So round trip is not possible.
			They map to FFFD, which is the replacement character
			so this is not an error

No errors
#include "../intl/cs_iso8859_1.h"
*/

/*
-- Multibyte character sets --
#include "../intl/cs_big5.h"
#include "../intl/cs_gb2312.h"
#include "../intl/cs_jis_0208_1990.h"
#include "../intl/cs_ksc5601.h"
#include "../intl/csjis2_p.h"
*/


#define UNICODE_INDEX(u)	(((u) % 256) + from_unicode_map [((u) / 256)])
#define UNICODE_LOOKUP(u)	from_unicode_mapping_array [ UNICODE_INDEX(u) ]

main()
{
	unsigned short uch;
	int i;

	for (i = 0; i <= 255; i++) {
		uch = to_unicode_map[i];
		printf("0x%02X\t0x%04X\t#\n", i, uch);
	}

	printf("Test completed\n");
}
