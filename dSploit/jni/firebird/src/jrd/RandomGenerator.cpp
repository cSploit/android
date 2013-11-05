/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "common.h"
#include "../jrd/RandomGenerator.h"
#include "../jrd/os/guid.h"


namespace Jrd {

RandomGenerator::RandomGenerator()
	: bufferPos(BUFFER_SIZE)
{
}


void RandomGenerator::getBytes(void* p, size_t size)
{
	while (size > 0)
	{
		const size_t size2 = MIN(size, BUFFER_SIZE);

		if (bufferPos + size2 > BUFFER_SIZE)
		{
			if (bufferPos < BUFFER_SIZE)
				memmove(buffer, buffer + bufferPos, BUFFER_SIZE - bufferPos);
			GenerateRandomBytes(buffer + (BUFFER_SIZE - bufferPos), bufferPos);
			bufferPos = 0;
		}

		memcpy(p, buffer + bufferPos, size2);
		p = ((char*) p) + size2;
		bufferPos += size2;
		size -= size2;
	}
}

} // namespace

