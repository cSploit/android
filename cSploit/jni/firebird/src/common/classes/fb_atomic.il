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
 *  The Original Code was created by Bill Oliver
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Bill Oliver <Bill.Oliver@sas.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.

Solaris 9 sparc assembly language implementation of atomic operators

See http://developers.sun.com/solaris/articles/atomic_sparc/

*/

/* sparc version of fetch_and_add atomic operation, see fb_atomic.h */
/* extern int fetch_and_add_il(volatile unsigned *word_addr, int value); */

.inline fetch_and_add_il,8
	membar #LoadStore|#StoreStore
	ld [%o0],%g1 ! load *word_addr from %o0 to %g1
	add %g1,%o1,%o2 ! compute the desired result, %g1 + %o1 (value) -> %o2 (temporary)
0:
	cas [%o0],%g1,%o2 ! try to CAS it into place
	cmp %g1,%o2 ! compare old to new
	bne,a,pn %icc,0b ! CAS failed, try again
	mov %o2,%g1 ! copy result -> %g1 for next iteration
	mov %o2,%o0 ! return old value
	membar #LoadLoad|#LoadStore
.end

/* sparc version of compare_and_swap, see fb_atomic.h */
/* extern boolean_t compare_and_swap_il(volatile unsigned *word_addr, unsigned *old_val_addr, int new_val); */

.inline compare_and_swap_il,12
	membar #LoadStore|#StoreStore
	ld    [%o1],%g1 ! set the old value
	cas   [%o0],%g1,%o2 ! try the CAS
	cmp   %g1,%o2
	be,a  true
	mov   1,%o0 ! return TRUE/1
	mov   0,%o0 ! return FALSE/0
true:
	st    %o2,[%o1] ! store existing value in memory
	membar #LoadLoad|#LoadStore
.end
