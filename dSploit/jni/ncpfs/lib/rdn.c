/*
    rdn.c
    Copyright (C) 1999  Petr Vandrovec

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Revision history:

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Created this file from nwnet.c

	1.01  1999, December  5		Petr Vandrovec <vandrove@vc.cvut.cz>
		DN length check in __NWDSExtractRDN and NWDSRemoveAllTypesW

*/

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/mman.h>

#include "nwnet_i.h"
#include "ncplib_i.h"
#include "ncpcode.h"

struct RDNEntry {
	size_t typeLen;
	const wchar_t* type;	/* C, S, L, OU, O, CN, SA, ... */
	size_t valLen;
	const wchar_t* val;
	struct RDNEntry* up;
	struct RDNEntry* next;	/* for CN=A+Bindery Type=B... */
};

/* frees entry in case of failure */
static NWDSCCODE __NWDSAddRDN(struct RDNEntry** add, struct RDNEntry* entry, struct RDNEntry*** next) {
	if (entry->typeLen) {
		switch (entry->typeLen) {
			case 11:
				if (!wcsncasecmp(entry->type, L"Common Name", 11)) {
					entry->typeLen = 2;
					entry->type = L"CN";
				}
				break;
			case 12:
				if (!wcsncasecmp(entry->type, L"Country Name", 12)) {
					entry->typeLen = 1;
					entry->type = L"C";
				}
				break;
			case 13:
				if (!wcsncasecmp(entry->type, L"Locality Name", 13)) {
					entry->typeLen = 1;
					entry->type = L"L";
				}
				break;
			case 14:
				if (!wcsncasecmp(entry->type, L"Street Address", 14)) {
					entry->typeLen = 2;
					entry->type = L"SA";
				}
				break;
			case 17:
				if (!wcsncasecmp(entry->type, L"Organization Name", 17)) {
					entry->typeLen = 1;
					entry->type = L"O";
				}
				break;
			case 22:
				if (!wcsncasecmp(entry->type, L"State or Province Name", 22)) {
					entry->typeLen = 1;
					entry->type = L"S";
				}
				break;
			case 24:
				if (!wcsncasecmp(entry->type, L"Organizational Unit Name", 24)) {
					entry->typeLen = 2;
					entry->type = L"OU";
				}
				break;
		}
	}
	if ((entry->typeLen == 1) && (entry->valLen > 2) &&
	    ((*entry->type == L'C') || (*entry->type == L'c'))) {
	    	free(entry);
		return ERR_COUNTRY_NAME_TOO_LONG;
	}
	if (*add) {
		struct RDNEntry** curr = add;
		struct RDNEntry* nextEntry = *curr;
		size_t typeLen = entry->typeLen;
		const wchar_t* type = entry->type;
		
		if (typeLen) {
			if (!nextEntry->typeLen) {
				free(entry);
				return ERR_ATTR_TYPE_NOT_EXPECTED;
			}
			do {
				int prop;
				size_t l;
				int cmp;
			
				nextEntry = *curr;
				if (!nextEntry)
					break;
				l = typeLen;
				prop = 0;
				if (typeLen < nextEntry->typeLen) {
					prop = -1;
				} else if (typeLen > nextEntry->typeLen) {
					l = nextEntry->typeLen;
					prop = 1;
				}
				cmp = wcsncasecmp(type, nextEntry->type, l);
				if (cmp < 0)
					break;
				if (cmp == 0) {
					if (prop < 0)
						break;
					if (!prop) {
						free(entry);
						return ERR_DUPLICATE_TYPE;
					}
				}
				curr = &nextEntry->next;
			} while (1);
		} else {
			if (nextEntry->typeLen) {
				free(entry);
				return ERR_ATTR_TYPE_EXPECTED;
			}
			do {
				nextEntry = *curr;
				if (!nextEntry)
					break;
				curr = &nextEntry->next;
			} while (1);
		}
		entry->next = nextEntry;
		*curr = entry;
		/* '+' rule in action*/
		if (next)
			*next = &((*add)->up);
	} else {
		*add = entry;
		if (next)
			*next = &entry->up;
	}
	return 0;
}

void __NWDSDestroyRDN(struct RDNInfo* rdn) {
	struct RDNEntry* up;
	
	up = rdn->end;
	while (up) {
		struct RDNEntry* tmp;
		
		tmp = up;
		up = up->up;
		do {
			struct RDNEntry* tmp2;
			
			tmp2 = tmp;
			tmp = tmp->next;
			free(tmp2);
		} while (tmp);
	}
}

/* returned rdn contains pointers to DN! */
NWDSCCODE __NWDSCreateRDN(struct RDNInfo* rdn, const wchar_t* dn, size_t* trailingDots) {
	NWDSCCODE err = 0;
	int first = 1;
	struct RDNEntry** add;
	size_t dots = 0;
	struct RDNEntry* currEntry = NULL;
		
	rdn->depth = 0;
	rdn->end = NULL;
	/* map empty string to nothing */
	if (!*dn) {
		if (trailingDots)
			*trailingDots = 0;
		return 0;
	}
	add = &rdn->end;
	do {
		wchar_t x;
		
		x = *dn++;
		if (!x) {
			if (dots)
				break;
			if (!currEntry || !currEntry->val) {
				err = ERR_EXPECTED_IDENTIFIER;
				break;	/* end; we did not expect anything */
			}
			currEntry->valLen = dn-currEntry->val-1;
			err = __NWDSAddRDN(add, currEntry, &add);
			currEntry = NULL;
			rdn->depth++;
			break;	/* we have value... */
		} else if (x == '.') {
			if (first || dots) {
				dots++;
			} else if (!currEntry || !currEntry->val) {
				err = ERR_EXPECTED_IDENTIFIER;
				break;
			} else {
				currEntry->valLen = dn-currEntry->val-1;
				err = __NWDSAddRDN(add, currEntry, &add);
				currEntry = NULL;
				rdn->depth++;
				dots=1;
			}
		} else if (x == '+') {
			if (dots || !currEntry || !currEntry->val) {
				err = ERR_EXPECTED_IDENTIFIER;
				break;
			}
			currEntry->valLen = dn-currEntry->val-1;
			err = __NWDSAddRDN(add, currEntry, NULL);
			currEntry = NULL;
		} else if (x == '=') {
			if (dots || !currEntry || !currEntry->val) {
				err = ERR_EXPECTED_IDENTIFIER;
				break;
			}
			if (currEntry->type) {
				err = ERR_EXPECTED_RDN_DELIMITER;
				break;
			}
			currEntry->type = currEntry->val;
			currEntry->typeLen = dn - currEntry->type - 1;
			currEntry->val = NULL;
		} else {
			if (first + dots > 1) {
				err = ERR_EXPECTED_IDENTIFIER;
				break;
			}
			first = dots = 0;
			if (!currEntry) {
				currEntry = (struct RDNEntry*)malloc(sizeof(*currEntry));
				if (!currEntry) {
					err = ERR_NOT_ENOUGH_MEMORY;
					break;
				}
				memset(currEntry, 0, sizeof(*currEntry));
			}
			if (!currEntry->val)
				currEntry->val = dn-1;
			if (x == '\\') {
				x = *dn++;
				if (!x) {
					err = ERR_EXPECTED_IDENTIFIER;
					break;
				}
			}
		}
	} while (!err);
	if (currEntry)
		free(currEntry);
	if (!err) {
		if (trailingDots)
			*trailingDots = dots;
		else if (dots)
			err = ERR_EXPECTED_IDENTIFIER;
	}
	if (err)
		__NWDSDestroyRDN(rdn);
	return err;
}

static NWDSCCODE __NWDSApplyDefaultNamingRule(const struct RDNInfo* rdn) {
	size_t depth = rdn->depth;
	if (depth > 0) {
		struct RDNEntry* level = rdn->end;
		const wchar_t* type = L"CN";
		size_t typeLen = 2;	/* wcslen(type) */
		while (--depth) {
			if (!level->typeLen) {
				if (level->next)
					return ERR_INCONSISTENT_MULTIAVA;
				level->type = type;
				level->typeLen = typeLen;
			}
			type = L"OU";
			typeLen = 2;	/* wcslen(type) */
			level = level->up;
		}
		if (!level->typeLen) {
			if (level->next)
				return ERR_INCONSISTENT_MULTIAVA;
			level->type = L"O";
			level->typeLen = 1; /* wcslen(type) */
		}
	}
	return 0;
}

static NWDSCCODE __NWDSCopyRDN(struct RDNEntry** dst, struct RDNEntry* src) {
	while (src) {
		struct RDNEntry* tmp = src;
		struct RDNEntry** dst2 = dst;
		
		do {
			struct RDNEntry* wrt = *dst2 = (struct RDNEntry*)malloc(sizeof(*wrt));
			if (!wrt)
				return ERR_NOT_ENOUGH_MEMORY;
			wrt->type = tmp->type;
			wrt->typeLen = tmp->typeLen;
			wrt->val = tmp->val;
			wrt->valLen = tmp->valLen;
			wrt->up = NULL; /* wrt->next is set later */
			dst2 = &wrt->next;
			tmp = tmp->next;
		} while (tmp);
		*dst2 = NULL;
		src = src->up;
		dst = &(*dst)->up;
	}
	return 0;
}

static NWDSCCODE __NWDSExtractRDN(struct RDNInfo* rdn, wchar_t* dst, size_t maxlen, int typeLess, size_t tdots) {
	struct RDNEntry* entry;
	
	entry = rdn->end;
	if (entry || tdots) {
		while (entry) {
			struct RDNEntry* en;
		
			en = entry;
			while (en) {
				if (!typeLess && en->typeLen) {
					if (en->typeLen + 1 > maxlen)
						return ERR_DN_TOO_LONG;
					maxlen -= en->typeLen + 1;
					memcpy(dst, en->type, en->typeLen * sizeof(wchar_t));
					dst += en->typeLen;
					*dst++ = '=';
				}
				if (en->valLen > maxlen)
					return ERR_DN_TOO_LONG;
				maxlen -= en->valLen;
				memcpy(dst, en->val, en->valLen * sizeof(wchar_t));
				dst += en->valLen;
				en = en->next;
				if (en) {
					if (!maxlen)
						return ERR_DN_TOO_LONG;
					--maxlen;
					*dst++ = '+';
				}
			}
			entry = entry->up;
			if (entry) {
				if (!maxlen)
					return ERR_DN_TOO_LONG;
				--maxlen;
				*dst++ = '.';
			}
		}
		if (tdots > maxlen)
			return ERR_DN_TOO_LONG;
		for (;tdots;tdots--)
			*dst++ = '.';
		*dst++ = 0;
	} else
		wcscpy(dst, L"[Root]");
	return 0;
}

/* DN modification procedures */
NWDSCCODE NWDSRemoveAllTypesW(UNUSED(NWDSContextHandle ctx), const wchar_t* src,
		wchar_t* dst) {
	wchar_t c;
	wchar_t lc = 0;
	wchar_t* dst2 = dst;
	wchar_t* dste = dst + MAX_DN_CHARS;
	int bdot = 0;
	int mdot = 0;
	int dsteq = 1;

	while ((c = *src++) != 0) {
		if (c == '.') {
			if (dsteq) {
				/* two or more consecutive dots */
				if (lc == '.')
					mdot = 1;
				/* dot on the begining of name */
				else if (!lc)
					bdot = 1;
				else 
					return ERR_EXPECTED_IDENTIFIER;
			}
			if (dst == dste)
				return ERR_DN_TOO_LONG;
			*dst++ = c;
			dst2 = dst;
			dsteq = 1;
		} else if (mdot) {
			return ERR_INVALID_DS_NAME;
		} else if (c == '=') {
			if (!dst2)
				return ERR_EXPECTED_RDN_DELIMITER;
			if (dsteq)
				return ERR_EXPECTED_IDENTIFIER;	/* empty before = */
			dst = dst2;
			dst2 = NULL;
			dsteq = 1;
		} else if (c == '+') {
			if (dsteq)
				return ERR_EXPECTED_IDENTIFIER;	/* empty before + */
			if (dst == dste)
				return ERR_DN_TOO_LONG;
			*dst++ = c;
			dst2 = dst;
			dsteq = 1;
		} else {
			if (dst == dste)
				return ERR_DN_TOO_LONG;
			dsteq = 0;
			*dst++ = c;
			if (c == L'\\') {
				wchar_t tmp_c;
				
				tmp_c = *src++;
				if (!tmp_c)
					return ERR_INVALID_DS_NAME;
				if (dst == dste)
					return ERR_DN_TOO_LONG;
				*dst++ = tmp_c;
			}
		}
		lc = c;
	}
	/* DN was empty or terminated with '.', '+' or '=' */
	if (dsteq) {
		/* FIXME: Should we allow empty string? */
		/* only '.' is legal terminator */
		if (lc != '.')
			return ERR_INVALID_DS_NAME;
		/* and we must not have dots on both end of name */
		if (bdot)
			return ERR_INVALID_DS_NAME;
	}
	*dst++ = c;
	return 0;
}

static inline int __NWDSIsSpecialName(const wchar_t* src) {
	if (*src == '[') {
		if (!wcscasecmp(src, L"[Root]") ||
		    !wcscasecmp(src, L"[Supervisor]") ||
		    !wcscasecmp(src, L"[Public]") ||
		    !wcscasecmp(src, L"[Self]") ||
		    !wcscasecmp(src, L"[Creator]") ||
		    !wcscasecmp(src, L"[Inheritance Mask]") ||
		    !wcscasecmp(src, L"[Root Template]") ||
		    !wcscasecmp(src, L"[Nothing]")) {
		    	return 1;
		}
	}
	return 0;
}

NWDSCCODE NWDSCanonicalizeNameW(NWDSContextHandle ctx, const wchar_t* src,
		wchar_t* dst) {
	struct RDNInfo rdn;
	struct RDNInfo ctxRDN;
	struct RDNEntry* ctxEntry;
	struct RDNEntry** rdnEntry;
	size_t dots;
	size_t oldDepth;
	int absolute = 0;
	NWDSCCODE err;
	int typeLess;
	u_int32_t flags;
	
	err = NWDSGetContext(ctx, DCK_FLAGS, &flags);
	if (err)
		return err;
	typeLess = (flags & DCV_TYPELESS_NAMES) ? 1 : 0;
	/* check for specials... */
	if (__NWDSIsSpecialName(src)) {
	    	wcscpy(dst, src);
		return 0;
	}
	if (*src == '.') {
		absolute = 1;
		src++;
	}
	err = __NWDSCreateRDN(&rdn, src, &dots);
	if (err)
		return err;
	err = NWDSGetContext2(ctx, DCK_RDN, &ctxRDN, sizeof(ctxRDN));
	if (err) {
		__NWDSDestroyRDN(&rdn);
		return err;
	}
	if (absolute) {
		if (dots) {
			if (rdn.depth) {
				__NWDSDestroyRDN(&rdn);
				return ERR_INVALID_OBJECT_NAME;
			}
			dots++;
		} else {
			if (rdn.depth) {
				dots = ctxRDN.depth;
			} else
				dots++;
		}
	}
	if (dots > ctxRDN.depth) {
		__NWDSDestroyRDN(&rdn);
		return ERR_TOO_MANY_TOKENS;
	}
	ctxEntry = ctxRDN.end;
	rdnEntry = &rdn.end;
	oldDepth = rdn.depth;
	rdn.depth = rdn.depth + ctxRDN.depth - dots;
	if (dots > oldDepth) {
		while (dots-- > oldDepth) {
			ctxEntry = ctxEntry->up;
		}
	} else if (dots < oldDepth) {
		while (dots++ < oldDepth) {
			rdnEntry = &(*rdnEntry)->up;
		}
	}
	if (typeLess) {
		/* skip to end, ignore types */
		while (*rdnEntry) {
			rdnEntry = &(*rdnEntry)->up;
			ctxEntry = ctxEntry->up;
		}
	} else {
		/* copy types */
		while (*rdnEntry) {
			struct RDNEntry* re;
			
			re = *rdnEntry;
			/* copy types from context if DN have no types */
			/* and context has types */
			/* If both are typeless, DefaultNamingRule tries to fix it */
			if (!re->typeLen && ctxEntry->typeLen) {
				struct RDNEntry* ctxE = ctxEntry;
				
				while (re) {
					/* context has less items than DN: boom */
					if (!ctxE) {
						err = ERR_INCONSISTENT_MULTIAVA;
						goto returnerr;
					}
					re->typeLen = ctxE->typeLen;
					re->type = ctxE->type;
					/* fix country attribute... */
					if ((re->typeLen == 1) && (re->valLen > 2) &&
					    ((*re->type == L'C') || (*re->type == L'c')))
						re->type = L"O";
					ctxE = ctxE->next;
					re = re->next;
				}	
			}
			rdnEntry = &(*rdnEntry)->up;
			ctxEntry = ctxEntry->up;
		}
	}
	err = __NWDSCopyRDN(rdnEntry, ctxEntry);
	if (!err) {
		if (!typeLess)
			err = __NWDSApplyDefaultNamingRule(&rdn);
		if (!err)
			err = __NWDSExtractRDN(&rdn, dst, MAX_DN_CHARS, typeLess, 0);
	}
returnerr:;	
	__NWDSDestroyRDN(&rdn);
	return err;
}

NWDSCCODE NWDSAbbreviateNameW(NWDSContextHandle ctx, const wchar_t* src,
		wchar_t* dst) {
	struct RDNInfo rdn;
	struct RDNInfo ctxRDN;
	struct RDNEntry* ctxEntry;
	struct RDNEntry** rdnEntry;
	struct RDNEntry** ostop;
	size_t oldDepth;
	NWDSCCODE err;
	int typeLess;
	u_int32_t flags;
	size_t trailingDots;
	size_t adots;
	
	err = NWDSGetContext(ctx, DCK_FLAGS, &flags);
	if (err)
		return err;
	typeLess = (flags & DCV_TYPELESS_NAMES) ? 1 : 0;
	/* check for specials... */
	if (__NWDSIsSpecialName(src)) {
	    	wcscpy(dst, src);
		return 0;
	}
	err = __NWDSCreateRDN(&rdn, src, NULL);
	if (err)
		return err;
	err = NWDSGetContext2(ctx, DCK_RDN, &ctxRDN, sizeof(ctxRDN));
	if (err) {
		__NWDSDestroyRDN(&rdn);
		return err;
	}
	
	trailingDots = 0;
	
	ctxEntry = ctxRDN.end;
	rdnEntry = &rdn.end;
	oldDepth = rdn.depth;

	if (rdn.depth < ctxRDN.depth) {
		while (rdn.depth < ctxRDN.depth--) {
			ctxEntry = ctxEntry->up;
			trailingDots++;
		}
	} else if (rdn.depth > ctxRDN.depth) {
		while (rdn.depth-- >ctxRDN.depth) {
			rdnEntry = &(*rdnEntry)->up;
		}
	}
	ostop = rdnEntry;
	adots = 0;
	while (*rdnEntry) {
		struct RDNEntry* re = *rdnEntry;
	
		adots++;
			
		if (((re->typeLen && ctxEntry->typeLen) &&
		     ((re->typeLen != ctxEntry->typeLen) ||
		      wcsncasecmp(re->type, ctxEntry->type, re->typeLen))) ||
		     ((re->valLen != ctxEntry->valLen) ||
		      wcsncasecmp(re->val, ctxEntry->val, re->valLen))) {
			rdnEntry = &re->up;
			ctxEntry = ctxEntry->up;
			trailingDots += adots;
			ostop = rdnEntry;
			adots = 0;
		} else {
			rdnEntry = &re->up;
			ctxEntry = ctxEntry->up;
		}
	}
	if (ostop == &rdn.end) {
		if (*ostop) {
			ostop = &(*ostop)->up;	/* do 'a..' instead of '.' */
			trailingDots++;
		} else
			trailingDots = 0;	/* emit '[Root]' instead of '.' */
	}
	ctxEntry = *ostop;
	*ostop = NULL;
	if (!err) {
		/* FIXME: should we limit length by MAX_DN_CHARS or MAX_RDN_CHARS?! */
		err = __NWDSExtractRDN(&rdn, dst, MAX_DN_CHARS, typeLess, trailingDots);
		/* FIXME: and should we return ERR_DN_TOO_LONG or ERR_RDN_TOO_LONG */
	}
	*ostop = ctxEntry;
	__NWDSDestroyRDN(&rdn);
	return err;
}

