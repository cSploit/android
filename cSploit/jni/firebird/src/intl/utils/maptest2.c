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
#include <stdio.h>
#include <stdlib.h>

#define CANT_MAP_CHARACTER	0

class codepage_map
{

public:
	unsigned short *to_unicode_map;
	unsigned char *from_unicode_mapping_array;
	unsigned short *from_unicode_map;
	unsigned short sizeof_to_unicode_map;
	unsigned short sizeof_from_unicode_mapping_array;
	unsigned short sizeof_from_unicode_map;
	const char* name;

	codepage_map()
	{
		to_unicode_map = NULL;
		from_unicode_map = NULL;
		from_unicode_mapping_array = NULL;
		sizeof_to_unicode_map = 0;
		sizeof_from_unicode_map = 0;
		sizeof_from_unicode_mapping_array = 0;
		name = NULL;
	}

	codepage_map(const unsigned short *_to,
				 const unsigned char *_from_array,
				 const unsigned short *_from_map,
				 const unsigned short sizeof_to,
				 unsigned short sizeof_from_array,
				 unsigned short sizeof_from_map, const char *_name)
	{
		to_unicode_map = _to;
		from_unicode_mapping_array = _from_array;
		from_unicode_map = _from_map;
		sizeof_to_unicode_map = sizeof_to;
		sizeof_from_unicode_mapping_array = sizeof_from_array;
		sizeof_from_unicode_map = sizeof_from_map;
		name = _name;
		test_codepage();
	}

	unsigned short to_unicode(unsigned char c)
	{
		return to_unicode_map[c];
	}

	unsigned short from_unicode(unsigned short c)
	{
		return from_unicode_mapping_array[from_unicode_map[c / 256] + (c % 256)];
	}

	void test_codepage()
	{
		if (sizeof_to_unicode_map != 256 * sizeof(to_unicode_map[0]))
			printf("The to_unicode_map is too small! %d entries!\n",
				   sizeof_to_unicode_map / sizeof(to_unicode_map[0]));

		if (sizeof_from_unicode_map != 256 * sizeof(from_unicode_map[0]))
			printf("The from_unicode_map is too small! %d entries!\n",
				   sizeof_from_unicode_map / sizeof(from_unicode_map[0]));

		for (int i = 0; i <= 255; i++)
		{
			unsigned short uch = to_unicode((unsigned char) i);

			if (uch == CANT_MAP_CHARACTER)
				continue;

			unsigned char ch2 = from_unicode(uch);
			if (ch2 != i)
			{
				printf("Mapping error: Character %02x -> Unicode %04x -> Char %02x\n", i, uch, ch2);

				/* Find the Character in the from_unicode_mapping_array */
				int j;
				for (j = 0; j < sizeof_from_unicode_mapping_array; j++)
				{
					if (from_unicode_mapping_array[j] == i)
					{
						/* Mapping table is wrong - recommend a fix for it */
						printf
							("Recommend from_unicode_map[0x%02x] be set to %d\n",
							 uch / 256, j - (uch % 256));
						break;
					}
				}
				if (j == sizeof_from_unicode_mapping_array)
				{
					/* Oops - found a character that does exists in the character set
					but not in unicode - an obvious error! */
				printf("Error: Character %d does not exist in the from_unicode_mapping_array\n", i);
				}
			}
		}

		for (i = 0; i <= 255; i++)
		{
			if (from_unicode_map[i] + 0xFF >= sizeof_from_unicode_mapping_array)
			{
				printf("From_unicode array bounds error at position %02x00\n", i);
				continue;
			}

			for (int j = 0; j <= 255; j++)
			{
				unsigned short uch = i * 256 + j;
				unsigned char ch2 = from_unicode(uch);
				if (ch2 == CANT_MAP_CHARACTER)
					continue;

				unsigned short uch2 = to_unicode(ch2);
				if (uch != uch2)
				{
					printf("Mapping error: Unicode %04x -> ch %02x -> Unicode %04x\n", uch, ch2, uch2);

					int k;
					for (k = 0; k <= 255; k++)
					{
						if (to_unicode_map[k] == uch) {
							/* Can map this character from charset to unicode */
							/* Assume fix was printed out above */
						}
					}
					if (k > 255) {
						/* This unicode doesn't exist in charset */
						/* Mapping table is wrong - recommend a fix for it */
						printf("Recommend from_unicode_map[0x%02x] be set to %d\n", uch / 256, 0);
					}
				}
			}
		}

		printf("Test of %s completed\n", name);
	}
}





/*
typedef unsigned short	USHORT;
typedef unsigned char	UCHAR;

*/
#define UCHAR	unsigned char
#define USHORT	unsigned short
#define JRD_COMMON_H

codepage_map *get_w1250()
{
#include "cs_w1250.h"
	return new codepage_map(to_unicode_map,
							from_unicode_mapping_array,
							from_unicode_map,
							sizeof(to_unicode_map),
							sizeof(from_unicode_mapping_array),
							sizeof(from_unicode_map), "cs_w1250.h");
}


int main(int argc, char *argv[])
{
	codepage_map *w1250 = get_w1250();
	w1250->test_codepage();
}


/*
fixed
#include "cs_w1254.h"
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

