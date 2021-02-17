/*
 * prelude.c
 * Copyright (c) 2001 Petr Vandrovec <vandrove@vc.cvut.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

extern void _TerminateNLM(unsigned int, unsigned int, unsigned int);
extern int _SetupArgv(void*);
extern int _StartNLM(void*, void*, const char*,
		     const char*, unsigned int, void*,
		     int (*)(), unsigned int, unsigned int,
		     unsigned int*, void*);
extern int main(int argc, char* argv[]);

/* unsigned int _argc = 0; */
static unsigned int NLMID = 0;


static int _cstart_(void) {
	return _SetupArgv(main);
}

void _Stop(void) {
	_TerminateNLM(NLMID, 0, 5);
}

unsigned int _Prelude(void* NLMHandle, void* errorScreen, const char* commandLine,
		      const char* loadDirectoryPath, 
		      unsigned int uninitializedDataLength, void* NLMFileHandle, 
		      int (*readRoutineP)(), unsigned int customDataOffset, 
		      unsigned int customDataSize) {
	return _StartNLM(NLMHandle, errorScreen, commandLine, loadDirectoryPath,
		uninitializedDataLength, NLMFileHandle, readRoutineP, customDataOffset,
		customDataSize, &NLMID, _cstart_);
}

