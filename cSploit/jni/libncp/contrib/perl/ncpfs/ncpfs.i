#define _include	#include
#define _define		#define
#define _if		#if
#define _else		#else
#define _endif		#endif

%module ncpfs

%pragma(swig) nodefault

%clear SWIGTYPE [];

%runtime %{
typedef char fixedCharArray;
typedef u_int8_t fixedArray;
typedef u_int32_t fixedU32Array;
typedef char byteLenPrefixCharArray;
typedef char size_tLenPrefixCharArray;
typedef char uint_fast16_tLenPrefixCharArray;
typedef u_int8_t NWCCTranAddr_buffer;

_define SWIG_BUILD

_include <ncp/nwclient.h>
_include <ncp/eas.h>
%}

_define inline
_define __attribute__(x)

%rename(Buf_T) tagBuf_T;
%rename(NWDSContextHandle) __NWDSContextHandle;
/* %rename(NWCONN_HANDLE) ncp_conn; */
%rename(Asn1ID_T) tagAsn1ID_T;

/* How to get these without including couple of headers ?! */
typedef unsigned char  u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int   u_int32_t;
typedef u_int32_t      uid_t;
typedef unsigned long  __off_t;
typedef unsigned int   size_t;
typedef int	       int32_t;
typedef double         ncp_off64_t;
typedef unsigned short uint_fast16_t;
typedef unsigned int   uint_fast32_t;
typedef unsigned short uint_least16_t;
typedef unsigned int   uint_least32_t;
typedef unsigned int   time_t;

%runtime %{
_define getBasicSV(sv) _getBasicSV(aTHX_ sv)
static SV* _getBasicSV(pTHX_ SV *sv) {
	MAGIC *mg;

	if (!sv_isobject(sv)) {
		return NULL;
	}
	sv = SvRV(sv);
	if (SvTYPE(sv) != SVt_PVHV) {
		return sv;
	}
	if (!SvMAGICAL(sv)) {
		return sv;
	}
	mg = mg_find(sv,'P');
	if (!mg) {
		return sv;
	}
	sv = mg->mg_obj;
	if (!sv_isobject(sv)) {
		return NULL;
	}
	return SvRV(sv);
}

_define checkSVType(sv,_t,tc) _checkSVType(aTHX_ sv, _t, tc)
static inline int _checkSVType(pTHX_ SV *sv, swig_type_info *_t, swig_type_info **tc) {
	HV *stash;
	
	if (!_t || !sv) {
		goto maybe;
	}
	if (SvOBJECT(sv)) {
		stash = SvSTASH(sv);
	} else if (SvROK(sv)) {
		SV *t = SvRV(sv);
		if (!SvOBJECT(t)) {
			goto nope;
		}
		stash = SvSTASH(t);
	} else {
nope:;
		if (tc) {
			*tc = NULL;
		}
		return 0;
	}
	_t = SWIG_TypeCheck(HvNAME(stash), _t);
	if (tc) {
		*tc = _t;
	}
	return _t != NULL;

maybe:;		
	if (tc) {
		*tc = NULL;
	}
	return -1;
}

_define MyConvertPtr(sv,ptr,len,t) _MyConvertPtr(aTHX_ sv, ptr, len, t)
static SV* _MyConvertPtr(pTHX_ SV *sv, void **ptr, STRLEN *len, swig_type_info *_t) {
	swig_type_info	*tc = NULL;
	void		*tmp;
	size_t		goffset;
	STRLEN		glen;
	int		glen_valid;
	STRLEN		cl;
	SV		*mainsv;

	/* If magical, apply more magic */
	SvGETMAGIC(sv);
	if (checkSVType(sv, _t, &tc) == 0) {
		return NULL;
	}
	mainsv = getBasicSV(sv);
	if (!mainsv) {
		if (!SvOK(sv)) {		/* Check for undef */
			*ptr = NULL;
			return sv;
		}
		return NULL;			/* All rest... */
	}
	sv = mainsv;
	goffset = 0;
	glen = 0;
	glen_valid = 0;
	do {
		size_t go;

		SvGETMAGIC(sv);
		if (SvTYPE(sv) != SVt_PVLV || LvTYPE(sv) != 'n') {	/* Should we support also other LVALUEs? */
			break;
		}
		go = LvTARGOFF(sv);
		cl = LvTARGLEN(sv);
		if (glen_valid) {
			if (goffset + glen > cl) {
				return NULL;
			}
		} else {
			glen = cl;
			glen_valid = 1;
		}
		goffset += go;
		sv = LvTARG(sv);
	} while (1);
	if (SvPOKp(sv)) {
		tmp = SvPV(sv, cl);
	} else {
		return NULL;
	}
	if (glen_valid) {
		if (goffset + glen > cl) {
			return NULL;
		}
	} else {
		glen = cl;
	}
	*ptr = (u_int8_t*)(tc ? SWIG_TypeCast(tc, tmp) : tmp) + goffset;
	*len = glen;
	return mainsv;
}

_define SwigConvertPtr(sv,ptr,t) _SwigConvertPtr(aTHX_ sv, ptr, t)
static SV* _SwigConvertPtr(pTHX_ SV *sv, void **ptr, swig_type_info *_t) {
	swig_type_info	*tc = NULL;
	void		*tmp;
	SV		*mainsv;

	/* If magical, apply more magic */
	SvGETMAGIC(sv);
	if (checkSVType(sv, _t, &tc) == 0) {
		return NULL;
	}
	mainsv = getBasicSV(sv);
	if (!mainsv) {
		if (!SvOK(sv)) {		/* Check for undef */
			*ptr = NULL;
			return sv;
		}
		return NULL;			/* All rest... */
	}
	if (SvIOKp(mainsv)) {
		tmp = (void*)SvIV(mainsv);
	} else {
		return NULL;
	}
	*ptr = (u_int8_t*)(tc ? SWIG_TypeCast(tc, tmp) : tmp);
	return mainsv;
}

_define getReferenceToIV(sv,ptr) _getReferenceToIV(aTHX_ sv, ptr)
static SV* _getReferenceToIV(pTHX_ SV *sv, IV *ptr) {
	/* If magical, apply more magic */
	SvGETMAGIC(sv);
	if (!SvROK(sv)) {
		return NULL;
	}
	sv = SvRV(sv);
	*ptr = SvIV(sv);
	return sv;
}

_define blessit(sv,type) _blessit(aTHX_ sv, type)
static inline void _blessit(pTHX_ SV *sv, const char* type) {
	sv_bless(sv, gv_stashpv(type, TRUE));
}

_define createTiedHash(rv,type) _createTiedHash(aTHX_ rv, type)
static SV *_createTiedHash(pTHX_ SV *rv, const swig_type_info *type) {
	HV* hash;
	HV* stash;
	SV* tie;
	SV* result;
	const char* name;

	name = type->clientdata?(const char*)type->clientdata:type->name;
	hash = newHV();
	tie = sv_2mortal(rv);
	stash = gv_stashpv(name, TRUE);
	sv_bless(tie, stash);
	hv_magic(hash, (GV*)tie, 'P');
	result = sv_2mortal(newRV_noinc((SV*)hash));
_if 1
	sv_bless(result, stash);
_else
	{
		dSP;
		
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		XPUSHs(sv_2mortal(newSVpv(name, 0)));
		XPUSHs(result);
		PUTBACK;
		call_method("TIEHASH", G_ARRAY | G_DISCARD | G_EVAL | G_KEEPERR);
		FREETMPS;
		LEAVE;
	}
_endif	
	return result;
}

_define createTiedArray(rv,type) _createTiedArray(aTHX_ rv, type)
static SV *_createTiedArray(pTHX_ SV *rv, const char *type) {
	AV* array;
	HV* stash;
	SV* tie;
	SV* result;
	
	array = newAV();
	tie = sv_2mortal(rv);
	stash = gv_stashpv(type, TRUE);
	sv_bless(tie, stash);
	sv_magic((SV*)array, tie, 'P', NULL, 0);
	result = sv_2mortal(newRV_noinc((SV*)array));
_if 1
//	sv_bless(result, stash);
_else
	{
		dSP;
		
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		XPUSHs(sv_2mortal(newSVpv(type, 0)));
		XPUSHs(result);
		PUTBACK;
		call_method("TIEARRAY", G_ARRAY | G_DISCARD | G_EVAL | G_KEEPERR);
		FREETMPS;
		LEAVE;
	}
_endif
	AvREAL_off(array);
	SvREADONLY_on((SV*)array);
	return result;
}

_define createTiedHashReference(pv,type) _createTiedHashReference(aTHX_ pv, type)
static inline SV* _createTiedHashReference(pTHX_ SV* pv, const swig_type_info* type) {
	return createTiedHash(newRV_noinc(pv), type);
}

_define createTiedArrayReference(pv,type) _createTiedArrayReference(aTHX_ pv, type)
static inline SV* _createTiedArrayReference(pTHX_ SV* pv, const char* type) {
	return createTiedArray(newRV_noinc(pv), type);
}

_define createLV(pv,off,len) _createLV(aTHX_ pv, off, len)
static SV* _createLV(pTHX_ SV *pv, size_t off, size_t len) {
	SV *sv = newSV(0);
	SvUPGRADE(sv, SVt_PVLV);
	LvTYPE(sv) = 'n';
	LvTARG(sv) = SvREFCNT_inc(pv);
	LvTARGOFF(sv) = off;
	LvTARGLEN(sv) = len;
	return sv;
}

_define createMagicChild(pv,off,len,type) _createMagicChild(aTHX_ pv, off, len, type)
static SV* _createMagicChild(pTHX_ SV *pv, size_t off, size_t len, const swig_type_info* type) {
	SV *sv = createLV(pv, off, len);
	return createTiedHashReference(sv, type);
}

_define setupAVLen(sv,itemlen) _setupAVLen(aTHX_ sv, itemlen)
static inline void _setupAVLen(pTHX_ SV* sv, size_t itemlen) {
	sv_magic(sv, NULL, '~', NULL, itemlen);
}

_define setupAVTypeLen(sv,type,itemlen) _setupAVTypeLen(aTHX_ sv, type, itemlen)
static inline void _setupAVTypeLen(pTHX_ SV* sv, swig_type_info* type, size_t itemlen) {
	SV* svp = newSVuv((UV)type);
	sv_magic(sv, svp, '~', NULL, itemlen);
	SvREFCNT_dec(svp);
}

_define getAVLen(sv) _getAVLen(aTHX_ sv)
static inline I32 _getAVLen(pTHX_ SV* sv) {
	MAGIC* mg = mg_find(sv, '~');
	return mg ? mg->mg_len : 0;
}

_define getAVTypeLen(sv,name) _getAVTypeLen(aTHX_ sv, name)
static inline I32 _getAVTypeLen(pTHX_ SV* sv, swig_type_info** name) {
	MAGIC* mg = mg_find(sv, '~');
	if (mg) {
		*name = (swig_type_info*)SvUV(mg->mg_obj);
		return mg->mg_len;
	}
	*name = NULL;
	return 0;
}

#define emit_buildArrayReferenceFromXArray(name,type,op)	\
_define buildArrayReferenceFrom##name##Array(items,array) _buildArrayReferenceFrom##name##Array(aTHX_ items, array)	\
static SV* _buildArrayReferenceFrom##name##Array(pTHX_ size_t items, const type * array) {				\
	AV* av;							\
								\
	av = newAV();						\
	while (items > 0) {					\
		av_push(av, op(*array));			\
		array++;					\
		items--;					\
	}							\
	return sv_2mortal(newRV_noinc((SV*)av));		\
}

emit_buildArrayReferenceFromXArray(Byte,  u_int8_t,  newSViv);
emit_buildArrayReferenceFromXArray(DWord, u_int32_t, newSViv);
emit_buildArrayReferenceFromXArray(NWConn, NWCONN_NUM, newSVuv);

#define _emit_av_2xarray(name,type,op)					\
static const char * _av_2##name##Array(pTHX_ AV *av, size_t array_size, type ** result) {	\
	type * r;							\
	size_t p;							\
									\
	if (array_size == 0) {						\
		*result = NULL;						\
		return NULL;						\
	}								\
	New((I32) #name, r, array_size, type);				\
	*result = r;							\
	for (p = 0; p < array_size; p++) {				\
		SV * sv;						\
		SV ** v = av_fetch(av, p, 0);				\
									\
		if (!v) {						\
			Safefree(*result);				\
			return "Cannot fetch element from array.";	\
		}							\
		sv = *v;						\
		op;							\
		r++;							\
	}								\
	return NULL;							\
}

#define emit_getXArray(name,type,op)							\
_emit_av_2xarray(name,type,op)								\
_define get##name##Array(sv,as,res,maxl) _get##name##Array(aTHX_ sv,as,res,maxl)	\
static const char * _get##name##Array(pTHX_ SV *sv, size_t *array_size, type ** result, size_t maxlen) {	\
	AV* av;							\
	size_t asize;						\
	/* If magical, apply more magic */			\
	SvGETMAGIC(sv);						\
	if (!SvROK(sv)) {            /* Check for undef */	\
		type * r;					\
		if (!SvOK(sv)) {				\
			*result = NULL;				\
			*array_size = 0;			\
			return NULL;				\
		}						\
		New((I32) #name, r, 1, type);			\
		*result = r;					\
		op;						\
		*array_size = 1;				\
		return NULL;					\
	}							\
	sv = SvRV(sv);						\
	SvGETMAGIC(sv);						\
	if (SvTYPE(sv) != SVt_PVAV) {				\
		return "Expected reference to array.";		\
	}							\
	av = (AV*)sv;						\
								\
	asize = av_len(av) + 1;					\
	if (asize >= maxlen) {					\
		return "Array too large.";			\
	}							\
	*array_size = asize;					\
	return _av_2##name##Array(aTHX_ av, asize, result);	\
}								\
								\
static inline void free##name##Array(type * buf) {		\
	Safefree(buf);						\
}

emit_getXArray(Byte,			u_int8_t,			*r = SvUV(sv))
emit_getXArray(UnsignedInt,		unsigned int,			*r = SvUV(sv))
emit_getXArray(NWConn,			NWCONN_NUM,			*r = SvUV(sv))

static SV * fetchItem(pTHX_ SV * sv, void * dst, swig_type_info * tp, size_t want_len) {
	void * buf;
	STRLEN len;
	
	if (!MyConvertPtr(sv, &buf, &len, tp)) {
		return sv_2mortal(Perl_newSVpvf(aTHX_ "Unexpected element type. Expected %s.", tp->name));
	}
	if (!buf) {
		return sv_2mortal(newSVpv("Undefined element found in array.", 0));
	}
	if (len != want_len) {
		return sv_2mortal(Perl_newSVpvf(aTHX_ "Element with length %u was found in array. Expected length of %s is %u.", len, tp->name, want_len));
	}
	memcpy(dst, buf, want_len);
	return NULL;
}

static const char * _av_2StructArray(pTHX_ AV *av, size_t array_size, void ** result, swig_type_info * tp, size_t want_len) {
	unsigned char * r;
	size_t p;
	
	if (array_size == 0) {
		*result = NULL;
		return NULL;
	}
	New((I32) tp->name, r, array_size * want_len, unsigned char);
	*result = r;
	for (p = 0; p < array_size; p++) {
		SV * sv;
		SV ** v = av_fetch(av, p, 0);
		if (!v) {
			Safefree(*result);
			return "Cannot fetch element from array.";
		}
		sv = *v;
		sv = fetchItem(aTHX_ sv, r, tp, want_len);
		if (sv) {
			Safefree(*result);
			return SvPV_nolen(sv);
		}
		r += want_len;
	}
	return NULL;
}

static const char * _getStructArray(pTHX_ SV *sv, size_t *array_size, void ** result, size_t maxlen, swig_type_info * tp, size_t want_len) {
	AV* av;
	size_t asize;
	/* If magical, apply more magic */
	SvGETMAGIC(sv);
	if (!SvROK(sv)) {            /* Check for undef */
		unsigned char * r;
		if (!SvOK(sv)) {
			*result = NULL;
			*array_size = 0;
			return NULL;
		}
		New((I32) tp->name, r, want_len, unsigned char);
		*result = r;
		sv = fetchItem(aTHX_ sv, r, tp, want_len);
		if (sv) {
			Safefree(*result);
			return SvPV_nolen(sv);
		}
		*array_size = 1;
		return NULL;
	}
	sv = SvRV(sv);
	SvGETMAGIC(sv);
	if (SvTYPE(sv) != SVt_PVAV) {
		return "Expected reference to array.";
	}
	av = (AV*)sv;

	asize = av_len(av) + 1;
	if (asize >= maxlen) {
		return "Array too large.";
	}
	*array_size = asize;
	return _av_2StructArray(aTHX_ av, asize, result, tp, want_len);
}

#define emit_getXStructArray(name, type)	\
static inline const char * _get##name##StructArray(pTHX_ SV * sv, size_t * sz, type ** res, size_t maxl, swig_type_info * tp, size_t want_len) {		\
	return _getStructArray(aTHX_ sv, sz, (void**)res, maxl, tp, want_len);	\
}								\
								\
static inline void free##name##StructArray(type * buf) {	\
	Safefree(buf);						\
}

emit_getXStructArray(NCPTrustee, struct ncp_trustee_struct)
#define getNCPTrusteeStructArray(sv,sz,res,maxl,type) _getNCPTrusteeStructArray(aTHX_ sv, sz, res, maxl, type##_descriptor, sizeof(type##_basetype))
%}

%wrapper %{
#define decltied	\
	dXSARGS;	\
	STRLEN len;	\
	I32 avlen;	\
	SV* sv;		\
	unsigned char* v
	
#define gettiedinfo()						\
	sv = MyConvertPtr(ST(0), (void**)&v, &len, NULL);	\
	if (!sv) {						\
		Perl_croak(aTHX_ "Type error in argument 1 of %s", __FUNCTION__);	\
		XSRETURN(1);					\
	}							\
	avlen = getAVLen(sv);					\
	if (avlen <= 0) {					\
		Perl_croak(aTHX_ "Type error in argument 1 of %s", __FUNCTION__);	\
		XSRETURN(1);					\
	}							\
	len = len / avlen;

#define gettiedPVinfo()						\
	sv = MyConvertPtr(ST(0), (void**)&v, &len, NULL);	\
	if (!sv) {						\
		Perl_croak(aTHX_ "Type error in argument 1 of %s", __FUNCTION__);	\
		XSRETURN(1);					\
	}							\
	avlen = getAVTypeLen(sv, &name);			\
	if (avlen <= 0) {					\
		Perl_croak(aTHX_ "Type error in argument 1 of %s", __FUNCTION__);	\
		XSRETURN(1);					\
	}							\
	len = len / avlen;


XS(_wrap_uvar_fetchsize) {
	decltied;
	
	if (items != 1) {
		Perl_croak(aTHX_ "Usage: %s(uv);", __FUNCTION__);
		XSRETURN(1);
	}
	gettiedinfo();
	ST(0) = sv_2mortal(newSVuv(len));
	XSRETURN(1);
}

XS(_wrap_uvar_storesize) {
	decltied;
	
	if (items != 2) {
		Perl_croak(aTHX_ "Usage: %s(uv, newsize);", __FUNCTION__);
		XSRETURN(1);
	}
	gettiedinfo();
	if (SvUV(ST(1)) != len) {
		Perl_croak(aTHX_ "Cannot change size of fixed array");
		XSRETURN(1);
	}
	XSRETURN(0);
}

XS(_wrap_uvar_exists) {
	decltied;
	UV idx;
	
	if (items != 2) {
		Perl_croak(aTHX_ "Usage: %s(uv, index);", __FUNCTION__);
		XSRETURN(1);
	}
	gettiedinfo();
	idx = SvUV(ST(1));
	ST(0) = sv_2mortal(newSViv(idx < 0 || idx >= len));
	XSRETURN(1);
}

XS(_wrap_uvar_fetch) {
	decltied;
	UV idx;
	
	if (items != 2) {
		Perl_croak(aTHX_ "Usage: %s(uv, index);", __FUNCTION__);
		XSRETURN(1);
	}
	gettiedinfo();
	idx = SvUV(ST(1));
	if (idx < 0 || idx >= len) {
		ST(0) = &PL_sv_undef;
	} else {
		unsigned char* b = v + idx * avlen;
		UV uv;
		
		switch (avlen) {
			case 1:	uv = *b; break;
			case 2: uv = *(u_int16_t*)b; break;
			case 4: uv = *(u_int32_t*)b; break;
			case 8: uv = *(u_int64_t*)b; break;
			default:
				Perl_croak(aTHX_ "Element size %u is not supported", avlen);
				uv = 0;
		}
		ST(0) = sv_2mortal(newSVuv(uv));
	}
	XSRETURN(1);
}

XS(_wrap_pvar_fetch) {
	decltied;
	swig_type_info* name;
	UV idx;
	
	if (items != 2) {
		Perl_croak(aTHX_ "Usage: %s(uv, index);", __FUNCTION__);
		XSRETURN(1);
	}
	gettiedPVinfo();
	idx = SvUV(ST(1));
	if (idx < 0 || idx >= len) {
		ST(0) = &PL_sv_undef;
	} else {
		SV* s;
		
		s = createLV(sv, idx * avlen, avlen);
		ST(0) = createTiedHashReference(s, name);
	}
	XSRETURN(1);
}

XS(_wrap_uvar_store) {
	decltied;
	UV idx;
	UV uv;
	unsigned char* b;
	
	if (items != 3) {
		Perl_croak(aTHX_ "Usage: %s(uv, index, value);", __FUNCTION__);
		XSRETURN(1);
	}
	gettiedinfo();
	idx = SvUV(ST(1));
	if (idx < 0 || idx >= len) {
		Perl_croak(aTHX_ "Index %u out of bounds <0...%u>", idx, len-1);
		XSRETURN(1);
	}
	b = v + idx * avlen;
	uv = SvUV(ST(2));
	switch (avlen) {
		case 1:	*b = uv; break;
		case 2: *(u_int16_t*)b = uv; break;
		case 4: *(u_int32_t*)b = uv; break;
		case 8: *(u_int64_t*)b = uv; break;
		default:
			Perl_croak(aTHX_ "Element size %u is not supported", avlen);
	}
	XSRETURN(0);
}

XS(_wrap_pvar_store) {
	decltied;
	UV idx;
	swig_type_info* name;
	unsigned char* b;
	STRLEN vlen;
	SV* sv_arg;
	unsigned char* data;
	
	if (items != 3) {
		Perl_croak(aTHX_ "Usage: %s(uv, index, value);", __FUNCTION__);
		XSRETURN(1);
	}
	gettiedPVinfo();
	idx = SvUV(ST(1));
	if (idx < 0 || idx >= len) {
		Perl_croak(aTHX_ "Index %u out of bounds <0...%u>", idx, len-1);
		XSRETURN(1);
	}
	b = v + idx * avlen;
	if ((sv_arg = MyConvertPtr(ST(2), (void**)&data, &vlen, name)) == NULL) {
		Perl_croak(aTHX_ "Type error in argument 3 of STORE. Expected %s.\n", name->name);
		XSRETURN(1);
	}
	if (vlen != avlen) {
		Perl_croak(aTHX_ "Problem with argument 3 of STORE. Expected %u bytes, but got %u.\n", avlen, vlen);
		XSRETURN(1);
	}
	memcpy(b, data, avlen);
	SvSETMAGIC(sv);
	XSRETURN(0);
}

XS(_wrap_uvar_nomodify) {
	dXSARGS;

	Perl_croak(aTHX_ PL_no_modify);
	XSRETURN(0);
}
%}

#undef __FILE__
#define __FILE__ __FILE__
%init %{
	newXSproto("ncpfs::uvarray::FETCHSIZE", _wrap_uvar_fetchsize,	__FILE__, "$");
	newXSproto("ncpfs::uvarray::STORESIZE", _wrap_uvar_storesize,	__FILE__, "$$");
	newXSproto("ncpfs::uvarray::EXISTS",    _wrap_uvar_exists,	__FILE__, "$$");
	newXSproto("ncpfs::uvarray::FETCH",	_wrap_uvar_fetch,	__FILE__, "$$");
	newXSproto("ncpfs::uvarray::STORE",	_wrap_uvar_store,	__FILE__, "$$$");
	newXSproto("ncpfs::uvarray::DELETE",	_wrap_uvar_nomodify,	__FILE__, "$$");
	newXSproto("ncpfs::uvarray::CLEAR",	_wrap_uvar_nomodify,	__FILE__, "$");
	newXSproto("ncpfs::uvarray::PUSH",	_wrap_uvar_nomodify,	__FILE__, "$@");
	newXSproto("ncpfs::uvarray::POP",	_wrap_uvar_nomodify,	__FILE__, "$");
	newXSproto("ncpfs::uvarray::SHIFT",	_wrap_uvar_nomodify,	__FILE__, "$");
	newXSproto("ncpfs::uvarray::UNSHIFT",	_wrap_uvar_nomodify,	__FILE__, "$@");
	newXSproto("ncpfs::uvarray::SPLICE",	_wrap_uvar_nomodify,	__FILE__, "$$$@");
	newXSproto("ncpfs::pvarray::FETCHSIZE", _wrap_uvar_fetchsize,	__FILE__, "$");
	newXSproto("ncpfs::pvarray::STORESIZE", _wrap_uvar_storesize,	__FILE__, "$$");
	newXSproto("ncpfs::pvarray::EXISTS",    _wrap_uvar_exists,	__FILE__, "$$");
	newXSproto("ncpfs::pvarray::FETCH",	_wrap_pvar_fetch,	__FILE__, "$$");
	newXSproto("ncpfs::pvarray::STORE",	_wrap_pvar_store,	__FILE__, "$$$");
	newXSproto("ncpfs::pvarray::DELETE",	_wrap_uvar_nomodify,	__FILE__, "$$");
	newXSproto("ncpfs::pvarray::CLEAR",	_wrap_uvar_nomodify,	__FILE__, "$");
	newXSproto("ncpfs::pvarray::PUSH",	_wrap_uvar_nomodify,	__FILE__, "$@");
	newXSproto("ncpfs::pvarray::POP",	_wrap_uvar_nomodify,	__FILE__, "$");
	newXSproto("ncpfs::pvarray::SHIFT",	_wrap_uvar_nomodify,	__FILE__, "$");
	newXSproto("ncpfs::pvarray::UNSHIFT",	_wrap_uvar_nomodify,	__FILE__, "$@");
	newXSproto("ncpfs::pvarray::SPLICE",	_wrap_uvar_nomodify,	__FILE__, "$$$@");
%}

%wrapper %{
_define hv_safe_store(hv,key,sv) _hv_safe_store(aTHX_ hv, key, sv)
static void _hv_safe_store(pTHX_ HV* hv, const char* key, SV* sv) {
	SV** t;

	t = hv_store(hv, key, strlen(key), sv, 0);
	if (!t) {
		SvREFCNT_dec(sv);
	}
}

_define newHVitemPV(hv,key,val) _newHVitemPV(aTHX_ hv, key, val)
static void _newHVitemPV(pTHX_ HV* hv, const char* key, const char* val) {
	hv_safe_store(hv, key, val ? newSVpv(val, 0) : &PL_sv_undef);
}

_define newHVitemUV(hv,key,val) _newHVitemUV(aTHX_ hv, key, val)
static void _newHVitemUV(pTHX_ HV* hv, const char* key, UV val) {
	hv_safe_store(hv, key, newSVuv(val));
}

_define fetchHVitemPV_nolen(hv,key) _fetchHVitemPV_nolen(aTHX_ hv, key)
static void * _fetchHVitemPV_nolen(pTHX_ HV* hv, const char* key) {
	SV** r;
	
	r = hv_fetch(hv, key, strlen(key), 0);
	if (!r) {
		return NULL;
	}
	return swig_PV_maynull_nolen(*r);
}

_define fetchHVitemUV(hv,key) _fetchHVitemUV(aTHX_ hv, key)
static UV _fetchHVitemUV(pTHX_ HV* hv, const char* key) {
	SV** r;
	
	r = hv_fetch(hv, key, strlen(key), 0);
	if (!r) {
		return SvUV(&PL_sv_undef);
	}
	return SvUV(*r);
}

_define newAVitemPV(av,val) _newAVitemPV(aTHX_ av, val)
static void _newAVitemPV(pTHX_ AV* av, const char* val) {
	av_push(av, val ? newSVpv(val, 0) : &PL_sv_undef);
}
%}

%typemap(perl5,in,numinputs=0) PTRBASED * ($1_basetype temp)
%{ $1 = &temp; %}

%typemap(perl5,argout) PTRBASED * %{
	if (argvi >= items) {
  		EXTEND(sp, 1);
	}
	$result = sv_newmortal();
	if (!result) {
		SWIG_MakePtr($result, *($1), $*1_descriptor, 0);
	}
	argvi++;
%}

%typemap(perl5,out) PTRBASED %{
	if (argvi >= items) {
  		EXTEND(sp, 1);
	}
	$result = sv_newmortal();
	if (!result) {
		SWIG_MakePtr($result, $1, $1_descriptor, 0);
	}
	argvi++;
%}

%typemap(perl5,perl5out) PTRBASED ""

#define PTR_BASED_OUTPUT(x)			\
%typemap(perl5,in,numinputs=0) x * = PTRBASED *;	\
%typemap(perl5,argout) x * = PTRBASED *;	\
%typemap(perl5,out)    x   = PTRBASED;		\
%typemap(perl5,perl5out) x = PTRBASED;

PTR_BASED_OUTPUT(NWCONN_HANDLE);
PTR_BASED_OUTPUT(NWVOL_HANDLE);

/* ncp_get_file_server_description_strings */
%typemap(perl5,in,numinputs=0) char descstring [512] ( char temp[512] )
%{ $1 = temp; %}

%typemap(perl5,argout) char descstring [512] {
	int i;
	char* x = temp;

	for (i = 0; i < 5; i++) {
		size_t p;

		if (argvi >= items) {
			EXTEND(sp, 1);
		}
		$result = sv_newmortal();
		p = strlen(x);
		sv_setpvn($result, x, p);
		argvi++;
		x += p + 1;
	}
}

/* error value from old API */
%typemap(perl5,in,numinputs=0) long * err ( long temp ) %{
	temp = NWE_REQUESTER_FAILURE;
	$1 = &temp;
%}

%typemap(perl5,argout) long * err %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	$result = sv_2mortal(newSViv(*$1));
	argvi++;
%}

%typemap(perl5,in) UVBASED input
%{ $1 = SvUV($input); %}

%typemap(perl5,out) UVBASED
%{ $result = sv_2mortal(newSVuv($1)); %}

%typemap(perl5,in) IVBASED input
%{ $1 = SvIV($input); %}

%typemap(perl5,in,numinputs=0) IVBASED * target ( $1_basetype temp )
%{ $1 = &temp; %}

%typemap(perl5,argout) IVBASED * target %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSViv(*$1));
	}
	argvi++;
%}

%typemap(perl5,in,numinputs=0) UVBASED * target ( $*1_type temp )
%{ $1 = &temp; %}

%typemap(perl5,argout) UVBASED * target %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVuv(*$1));
	}
	argvi++;
%}

%typemap(perl5,in) IVBASED * both (SV * sv_arg, $1_basetype tempiv) {
	IV val;
	
	sv_arg = getReferenceToIV($input, &val);
	if (!sv_arg) {
		SWIG_croak("Type error in argument %u of %s. Expected reference to integer.\n", $argnum, "$symname");
		XSRETURN(1);
	}
	tempiv = val;
	$1 = &tempiv;
}

%typemap(perl5,argout) IVBASED * both %{
	if (!result) {
		sv_setiv_mg(sv_arg, *$1);
	}
%}

%typemap(perl5,in) IVBASED * both_mn (SV * sv_arg, $1_basetype tempiv) {
	IV val;
	
	SvGETMAGIC($input);
	if (SvOK($input)) {
		sv_arg = getReferenceToIV($input, &val);
		if (!sv_arg) {
			SWIG_croak("Type error in argument %u of %s. Expected undef or reference to integer.\n", $argnum, "$symname");
			XSRETURN(1);
		}
		tempiv = val;
		$1 = &tempiv;
	} else {
		$1 = NULL;
	}
}

%typemap(perl5,argout) IVBASED * both_mn %{
	if (!result && $1) {
		sv_setiv_mg(sv_arg, *$1);
	}
%}

#define IV_BASED_OUTPUT(x)			\
%typemap(perl5,in,numinputs=0) x = IVBASED * target;	\
%typemap(perl5,argout) x = IVBASED * target;

#define UV_BASED_OUTPUT(x)			\
%typemap(perl5,in,numinputs=0) x = UVBASED * target;	\
%typemap(perl5,argout) x = UVBASED * target;

#define IV_BASED_BOTH(x)			\
%typemap(perl5,in) x = IVBASED * both;		\
%typemap(perl5,argout) x = IVBASED * both;

#define IV_BASED_BOTH_MN(x)			\
%typemap(perl5,in) x = IVBASED * both_mn;	\
%typemap(perl5,argout) x = IVBASED * both_mn;

/* relocatable structures with automatic refcounting */
%typemap(perl5,in,numinputs=0) PVBASED * target ($1_basetype temp)
%{ $1 = &temp; %}

%typemap(perl5,in) PVBASED * (SV * sv_arg), const PVBASED * (SV * sv_arg) {
	STRLEN len;

	if ((sv_arg = MyConvertPtr($input, (void**)&$1, &len, $1_descriptor)) == NULL) {
		SWIG_croak("Type error in argument %u of %s. Expected %s.\n", $argnum, "$symname", $1_descriptor->name);
		XSRETURN(1);
	}
	if (len != sizeof(*$1)) {
		SWIG_croak("Problem with argument %u of %s. Expected %u bytes, but got %u.\n", $argnum, "$symname", sizeof(*$1), len);
		XSRETURN(1);
	}
}

%typemap(perl5,in) PVBASED_relaxed * (SV * sv_arg) {
	STRLEN len;
	
	if ((sv_arg = MyConvertPtr($input, (void**)&$1, &len, $1_descriptor)) == NULL) {
		SWIG_croak("Type error in argument %u of %s. Expected %s.\n", $argnum, "$symname", $1_descriptor->name);
		XSRETURN(1);
	}
}

%typemap(perl5,in) PVBASED * both (SV * sv_arg) {
	STRLEN len;
	
	sv_arg = MyConvertPtr($input, (void**)&$1, &len, $1_descriptor);
	if (!sv_arg || !$1) {
		SWIG_croak("Type error in argument %u of %s. Expected %s.\n", $argnum, "$symname", $1_descriptor->name);
		XSRETURN(1);
	}
	if (len != sizeof(*$1)) {
		SWIG_croak("Problem with argument %u of %s. Expected %u bytes, but got %u.\n", $argnum, "$symname", sizeof(*$1), len);
		XSRETURN(1);
	}
}

%typemap(perl5,argout) PVBASED * target %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		SV* s;

		s = newSVpvn((char*)$1, sizeof(*$1));
		$result = createTiedHashReference(s, $1_descriptor);
	}
	argvi++;
%}

%typemap(perl5,argout) PVBASED * both
%{
	if (!result) {
		SvSETMAGIC(sv_arg);
	}
%}

%typemap(perl5,out) PVBASED *
%{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if ($1) {
		SV* s;

		s = newSVpvn((char*)$1, sizeof(*$1));
		$result = createTiedHashReference(s, $1_descriptor);
	} else {
		$result = &PL_sv_undef;
	}
	argvi++;
%}

/* FIXME */
%typemap(perl5,out) size_t [ANY] %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if ($1) {
		SV* s;
		
		s = createLV(sv_arg1, (u_int8_t*)$1 - (u_int8_t*)arg1, sizeof(*$1) * $1_dim0);
		setupAVLen(s, sizeof(*$1));
		$result = createTiedArrayReference(s, "ncpfs::uvarray");
	} else {
		$result = &PL_sv_undef;
	}
	argvi++;
%}

%typemap(perl5,out) u_int32_t reserved1[7] %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if ($1) {
		SV* s;
		
		s = createLV(sv_arg1, (u_int8_t*)$1 - (u_int8_t*)arg1, sizeof(*$1) * $1_dim0);
		setupAVLen(s, sizeof(*$1));
		$result = createTiedArrayReference(s, "ncpfs::uvarray");
	} else {
		$result = &PL_sv_undef;
	}
	argvi++;
%}

%typemap(perl5,out) PVBASED * depend
%{
	if (G_VOID != GIMME_V) {
		if (argvi >= items) {
			EXTEND(sp, 1);
		}
		if ($1) {
			$result = createMagicChild(sv_arg1, (u_int8_t*)$1 - (u_int8_t*)arg1, sizeof(*$1), $1_descriptor);
		} else {
			$result = &PL_sv_undef;
		}
		argvi++;
	}
%}

%typemap(perl5,perl5out) PVBASED * ""

%typemap(perl5,perl5out) PVBASED   ""

#define PV_BASED_NOINIT(x)					\
/* %remember x *; */						\
%typemap(perl5,in,numinputs=0)	x * target = PVBASED * target;	\
%typemap(perl5,in)		x * = PVBASED *;		\
%typemap(perl5,in)		const x * = const PVBASED *;	\
%typemap(perl5,in)		x * target = PVBASED * target;	\
%typemap(perl5,argout)		x * target = PVBASED * target;	\
%typemap(perl5,out)		x * = PVBASED *;		\
%typemap(perl5,perl5out)	x * = PVBASED *;		\
%typemap(perl5,perl5out)	x = PVBASED;

#define PV_BASED_INIT(x,y)					\
PV_BASED_NOINIT(y);						\
%extend x {							\
	x(void) { static y cs; return &cs; };			\
}

#define PV_BASED(x)		PV_BASED_INIT(x,x)

#define PV_BASED_STRUCT(x)	PV_BASED_INIT(x, struct x);

#define PV_BASED_OUTPUT(x)					\
%typemap(perl5,in,numinputs=0)	x = PVBASED * target;		\
%typemap(perl5,in)		x = PVBASED * target;		\
%typemap(perl5,argout)		x = PVBASED * target;

#define PV_BASED_BOTH(x)					\
%typemap(perl5,in)		x = PVBASED * both;		\
%typemap(perl5,argout)		x = PVBASED * both;		\
%typemap(perl5,in)		const x = PVBASED * both;	\
%typemap(perl5,argout)		const x = PVBASED * both;

#define PV_BASED_OUTDEP(x)					\
%typemap(perl5,out)		x = PVBASED * depend;

#define PV_BASED_MEMBER(structure,member,type)		\
PV_BASED_OUTDEP(type * structure##_##member##_getset);	\
PV_BASED_OUTDEP(type * structure##_##member##_get);

#define PV_BASED_ARRAY(itemdesc, count)			\
%typemap(perl5,out) itemdesc %{				\
	if (argvi >= items) {				\
		EXTEND(sp, 1);				\
	}						\
	if ($1) {					\
		SV* s;					\
							\
		s = createLV(sv_arg1, (u_int8_t*)$1 - (u_int8_t*)arg1, sizeof(*$1) * count);	\
		setupAVTypeLen(s, $1_descriptor, sizeof(*$1));			\
		$result = createTiedArrayReference(s, "ncpfs::pvarray");	\
	} else {					\
		$result = &PL_sv_undef;			\
	}						\
	argvi++;					\
%};							\
%typemap(perl5,perl5out) itemdesc ""

#define PV_BASED_RELAXED(x)				\
%typemap(perl5,in) x = PVBASED_relaxed *;

PV_BASED_STRUCT(ncp_request_header);
PV_BASED_STRUCT(ncp_reply_header);
PV_BASED_STRUCT(ncp_file_server_info);
PV_BASED_STRUCT(ncp_conn_spec);
PV_BASED_STRUCT(ncp_property_info);
PV_BASED_STRUCT(ncp_station_addr);
PV_BASED_STRUCT(ncp_prop_login_control);
PV_BASED_STRUCT(ncp_bindery_object);
PV_BASED_STRUCT(ncp_conn_ent);
PV_BASED_STRUCT(nw_property);
PV_BASED_STRUCT(ncp_volume_info);
PV_BASED_STRUCT(ncp_filesearch_info);
PV_BASED_BOTH(struct ncp_filesearch_info * fsinfo);
PV_BASED_STRUCT(ncp_file_info);
PV_BASED_STRUCT(nw_info_struct);
PV_BASED_MEMBER(nw_file_info, i, struct nw_info_struct);
PV_BASED_STRUCT(nw_info_struct2);
PV_BASED_STRUCT(NSI_Attributes);
PV_BASED_MEMBER(nw_info_struct2, Attributes, struct NSI_Attributes);
PV_BASED_STRUCT(NSI_TotalSize);
PV_BASED_MEMBER(nw_info_struct2, TotalSize, struct NSI_TotalSize);
PV_BASED_STRUCT(NSI_ExtAttrInfo);
PV_BASED_MEMBER(nw_info_struct2, ExtAttrInfo, struct NSI_ExtAttrInfo);
PV_BASED_STRUCT(NSI_Change);
PV_BASED_MEMBER(nw_info_struct2, Archive, struct NSI_Change);
PV_BASED_MEMBER(nw_info_struct2, Modify, struct NSI_Change);
PV_BASED_MEMBER(nw_info_struct2, Creation, struct NSI_Change);
PV_BASED_STRUCT(NSI_LastAccess);
PV_BASED_MEMBER(nw_info_struct2, LastAccess, struct NSI_LastAccess);
PV_BASED_STRUCT(NSI_Directory);
PV_BASED_MEMBER(nw_info_struct2, Directory, struct NSI_Directory);
PV_BASED_STRUCT(NSI_DOSName);
PV_BASED_MEMBER(nw_info_struct2, DOSName, struct NSI_DOSName);
PV_BASED_STRUCT(NSI_MacTimes);
PV_BASED_MEMBER(nw_info_struct2, MacTimes, struct NSI_MacTimes);
PV_BASED_STRUCT(NSI_Name);
PV_BASED_MEMBER(nw_info_struct2, Name, struct NSI_Name);
PV_BASED_STRUCT(nw_info_struct3);
PV_BASED_STRUCT(ncp_dos_info);
PV_BASED_MEMBER(ncp_dos_info, Creation, struct NSI_Change);
PV_BASED_MEMBER(ncp_dos_info, Modify, struct NSI_Change);
PV_BASED_MEMBER(ncp_dos_info, Archive, struct NSI_Change);
PV_BASED_MEMBER(ncp_dos_info, LastAccess, struct NSI_LastAccess);
PV_BASED_STRUCT(ncp_dos_info_rights);
PV_BASED_MEMBER(ncp_dos_info, Rights, struct ncp_dos_info_rights)
PV_BASED_STRUCT(ncp_namespace_format_BitMask);
PV_BASED_STRUCT(ncp_namespace_format_BitsDefined);
PV_BASED_STRUCT(ncp_namespace_format);
PV_BASED_MEMBER(ncp_namespace_format, BitMask, struct ncp_namespace_format_BitMask);
PV_BASED_MEMBER(ncp_namespace_format, BitsDefined, struct ncp_namespace_format_BitsDefined);
PV_BASED_STRUCT(nw_modify_dos_info);
PV_BASED_STRUCT(nw_file_info);
PV_BASED_STRUCT(ncp_search_seq);
PV_BASED_BOTH(struct ncp_search_seq * seq);
PV_BASED_STRUCT(nw_search_sequence);
PV_BASED_MEMBER(ncp_search_seq, s, struct nw_search_sequence);
PV_BASED_STRUCT(queue_job);
PV_BASED_BOTH(struct queue_job * job);
PV_BASED_STRUCT(nw_queue_job_entry);
PV_BASED_MEMBER(queue_job, j, struct nw_queue_job_entry);
PV_BASED_STRUCT(ncp_trustee_struct);
PV_BASED_NOINIT(struct ncp_deleted_file);
%extend ncp_deleted_file {
	ncp_deleted_file(void) { static struct ncp_deleted_file cs = { .seq = -1 }; return &cs; };
}
PV_BASED_BOTH(struct ncp_deleted_file * finfo);

PV_BASED(OPEN_FILE_CONN_CTRL);
PV_BASED_BOTH(OPEN_FILE_CONN_CTRL * openCtrl);
PV_BASED(OPEN_FILE_CONN);
PV_BASED_OUTPUT(OPEN_FILE_CONN * openFile);

PV_BASED(CONNS_USING_FILE);
PV_BASED_BOTH(CONNS_USING_FILE * cfa);
PV_BASED(CONN_USING_FILE);
PV_BASED_OUTPUT(CONN_USING_FILE * cf);
PV_BASED_ARRAY(CONN_USING_FILE [ANY], arg1->connCount);

PV_BASED(PHYSICAL_LOCKS);
PV_BASED_BOTH(PHYSICAL_LOCKS * locks);
PV_BASED(PHYSICAL_LOCK);
//%extend __PHYSICAL_LOCK {
//	PHYSICAL_LOCK(void) { static PHYSICAL_LOCK cs; return &cs; };
//}
PV_BASED_OUTPUT(PHYSICAL_LOCK * lock);
PV_BASED_ARRAY(PHYSICAL_LOCK [ANY], arg1->numRecords);

PV_BASED(CONN_SEMAPHORES);
PV_BASED_BOTH(CONN_SEMAPHORES * semaphores);
PV_BASED(CONN_SEMAPHORE);
PV_BASED_OUTPUT(CONN_SEMAPHORE * semaphore);

PV_BASED(SEMAPHORES);
PV_BASED_BOTH(SEMAPHORES * semaphores);
PV_BASED(SEMAPHORE);
PV_BASED_OUTPUT(SEMAPHORE * semaphore);
PV_BASED_ARRAY(SEMAPHORE [ANY], arg1->semaphoreCount);

PV_BASED(NWOBJ_REST);
PV_BASED(NWVolumeRestrictions);
PV_BASED(NWVOL_RESTRICTIONS);
PV_BASED_ARRAY(NWOBJ_REST [ANY], arg1->numberOfEntries);

PV_BASED(__NW_LIMIT_LIST_list);
PV_BASED(NW_LIMIT_LIST);
PV_BASED_ARRAY(__NW_LIMIT_LIST_list [ANY], arg1->numEntries);

PV_BASED(DIR_SPACE_INFO);
PV_BASED(NWCCVersion);
PV_BASED_STRUCT(NWCCRootEntry);

PV_BASED(NWCCTranAddr);
%typemap(perl5,check) const NWCCTranAddr* %{
	$1->buffer = $1->bufferdata;
%}
%typemap(perl5,check) NWCCTranAddr* = const NWCCTranAddr*;

PV_BASED_STRUCT(ncp_ea_enumerate_info);
PV_BASED_BOTH(struct ncp_ea_enumerate_info * info);
PV_BASED_STRUCT(ncp_ea_read_info);
PV_BASED_STRUCT(ncp_ea_write_info);
PV_BASED_RELAXED(struct ncp_ea_info_level1 *);
PV_BASED_RELAXED(struct ncp_ea_info_level6 *);

%typemap(perl5,in) char * INPUT %{
	SvGETMAGIC($input);
	if (!SvOK($input)) {
		SWIG_croak("Type error in argument %u of %s. Expected string.\n", $argnum, "$symname");
		XSRETURN(1);
	}
	$1 = (typeof($1))SvPV_nolen($input);
%}

%typemap(perl5,in) const unsigned char * = char * INPUT;

%typemap(perl5,in) time_t * source (time_t temp) %{
	temp = SvIV($input);
	$1 = &temp;
%}

IV_BASED_OUTPUT(time_t * target);		/* ncp_get_file_server_time */
IV_BASED_OUTPUT(time_t * login_time);		/* ncp_get_stations_logged_info */
IV_BASED_OUTPUT(u_int16_t * my_effective_rights);
IV_BASED_OUTPUT(u_int16_t * OUTPUT);
IV_BASED_OUTPUT(u_int32_t * OUTPUT);
IV_BASED_OUTPUT(u_int8_t * target);		/* directory handle... */
IV_BASED_OUTPUT(u_int16_t * target);		/* ncp_get_effective_dir_rights */
IV_BASED_OUTPUT(unsigned int * ttl);		/* ncp_get_dentry_ttl */
IV_BASED_OUTPUT(unsigned int * volnum);		/* ncp_volume_list_next */
IV_BASED_OUTPUT(u_int8_t * oc_action);		/* ncp_ns_open_create_entry */
IV_BASED_OUTPUT(u_int8_t * oc_callback);	/* ncp_ns_open_create_entry */
IV_BASED_OUTPUT(uid_t * uid);			/* ncp_get_mount_uid */
IV_BASED_OUTPUT(NWDIR_HANDLE * dirhandle);
IV_BASED_OUTPUT(NWVOL_NUM * ovol);
IV_BASED_OUTPUT(NWVOL_NUM * __vol);
IV_BASED_OUTPUT(size_t * destlen);		/* ncp_ns_extract_info_field_size */

%typemap(perl5,in,numinputs=0) fixedCharArray OUTPUT ( char temp[$1_dim0] ) [ANY]
%{ $1 = temp; %}

%typemap(perl5,argout) fixedCharArray OUTPUT [ANY]
%{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn($1, strnlen($1, $1_dim0)));
	}
	argvi++;
%}

%typemap(perl5,out) fixedCharArray[ANY]
%{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	$result = sv_2mortal(newSVpvn($1, strnlen($1, $1_dim0)));
	argvi++;
%}

%typemap(perl5,in) fixedCharArray[ANY] (STRLEN temp)
%{
	SvGETMAGIC($input);
 	if (!SvOK($input)) {
		$1 = NULL;
	} else {
		$1 = SvPV($input, temp);
	}
%}

%typemap(perl5,memberin) fixedCharArray[ANY]
%{
	if ($input) {
		if (temp2 >= $1_dim0) {
			memcpy($1, $input, $1_dim0);
		} else {
			memcpy($1, $input, temp2);
			$1[temp2] = 0;
		}
	}
%}

#define PREFIXED_CHAR_ARRAY(pref,type)					\
%typemap(perl5,in,numinputs=0) pref##LenPrefixCharArray OUTPUT ( char temp[$1_dim0+1] ) [ANY]	\
%{ $1 = temp; %}							\
									\
%typemap(perl5,argout) pref##LenPrefixCharArray OUTPUT [ANY]		\
%{									\
	if (argvi >= items) {						\
		EXTEND(sp, 1);						\
	}								\
	if (result) {							\
		$result = &PL_sv_undef;					\
	} else {							\
		$result = sv_2mortal(newSVpvn($1, ((type*)$1)[-1]));	\
	}								\
	argvi++;							\
%}									\
									\
%typemap(perl5,out) pref##LenPrefixCharArray[ANY]			\
%{									\
	if (argvi >= items) {						\
		EXTEND(sp, 1);						\
	}								\
	$result = sv_2mortal(newSVpvn($1, ((type*)$1)[-1]));		\
	argvi++;							\
%}									\
									\
%typemap(perl5,in) pref##LenPrefixCharArray (STRLEN temp) [ANY]		\
%{									\
	SvGETMAGIC($input);						\
 	if (!SvOK($input)) {						\
		$1 = NULL;						\
	} else {							\
		$1 = SvPV($input, temp);				\
	}								\
%}									\
									\
%typemap(perl5,memberin) pref##LenPrefixCharArray[ANY]			\
%{									\
	if ($input) {							\
		if (temp2 > $1_dim0) {					\
			temp2 = $1_dim0;				\
		}							\
		memcpy($1, $input, temp2);				\
		((type*)$1)[-1] = temp2;				\
	}								\
%}

PREFIXED_CHAR_ARRAY(byte, u_int8_t);
PREFIXED_CHAR_ARRAY(size_t, size_t);
PREFIXED_CHAR_ARRAY(uint_fast16_t, uint_fast16_t);

#define ASCIIZ_CHAR_ARRAY(pref,len)			\
%typemap(perl5,in,numinputs=0) pref ( char temp[len+1] )	\
%{ $1 = temp; %}					\
							\
%typemap(perl5,argout) pref				\
%{							\
	if (argvi >= items) {				\
		EXTEND(sp, 1);				\
	}						\
	if (result) {					\
		$result = &PL_sv_undef;			\
	} else {					\
		$result = sv_2mortal(newSVpvn($1, strnlen($1, len)));	\
	}						\
	argvi++;					\
%}

#define WCHART_ARRAY(pref,len)				\
%typemap(perl5,in,numinputs=0) pref ( wchar_t temp[len+1] )	\
%{ $1 = temp; %}					\
							\
%typemap(perl5,argout) pref				\
%{							\
	if (argvi >= items) {				\
		EXTEND(sp, 1);				\
	}						\
	if (result) {					\
		$result = &PL_sv_undef;			\
	} else {					\
		$result = sv_2mortal(newSVpvn((char*)$1, wcsnlen($1, len) * sizeof(wchar_t)));	\
	}						\
	argvi++;					\
%}

%typemap(perl5,in,numinputs=0) fixedArray OUTPUT ( $1_basetype temp[$1_dim0] ) [ANY]
%{ $1 = temp; %}

%typemap(perl5,argout) fixedArray OUTPUT[ANY] %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn($1, $1_dim0));
	}
	argvi++;
%}

%typemap(perl5,out) fixedArray[ANY] %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	$result = sv_2mortal(newSVpvn($1, $1_dim0));
	argvi++;
%}

%typemap(perl5,in) fixedArray[ANY], const fixedArray[ANY]
{
	STRLEN len;

	SvGETMAGIC($input);
	if (!SvOK($input)) {
		SWIG_croak("Type error in argument %u of %s. Expected string.\n", $argnum, "$symname");
		XSRETURN(1);
	}
	$1 = SvPV($input, len);
	if (len != $1_dim0) {
		SWIG_croak("Type error in argument %u of %s. Expected string with %u chars, but got %u chars.\n",
			$argnum, "$symname", $1_dim0, len);
		XSRETURN(1);
	}
}

%typemap(perl5,memberin) fixedArray[ANY] "memcpy($1, $input, $1_dim0);"

#define FIXEDARRAY_IN(x)				\
%typemap(perl5,in) x = const fixedArray [ANY];

#define FIXEDARRAY_OUTPUT(x)				\
%typemap(perl5,in,numinputs=0) x = fixedArray OUTPUT [ANY];	\
%typemap(perl5,argout) x = fixedArray OUTPUT [ANY];

#define FIXEDARRAYLEN_OUTPUT(x,len)			\
%typemap(perl5,in,numinputs=0) x ( unsigned char temp[len] )	\
%{ $1 = temp; %}					\
							\
%typemap(perl5,argout) x %{				\
	if (argvi >= items) {				\
		EXTEND(sp, 1);				\
	}						\
	if (result) {					\
		$result = &PL_sv_undef;			\
	} else {					\
		$result = sv_2mortal(newSVpvn($1, len));\
	}						\
	argvi++;					\
%}

%typemap(perl5,in,numinputs=0) (int * returned_no, u_int8_t conn_numbers[256]) (int conn_count, u_int8_t conns[256]) %{
	$1 = &conn_count;
	$2 = conns;
%}

%typemap(perl5,argout) (int * returned_no, u_int8_t conn_numbers[256]) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = buildArrayReferenceFromByteArray(conn_count, $2);
	}
	argvi++;
%}

%typemap(perl5,default) (size_t __maxlen, nuint8 * __nslist, size_t * __nslen) %{
	$1 = 256;
%}

%typemap(perl5,in) (size_t __maxlen, nuint8 * __nslist, size_t * __nslen) %{
	$1 = SvUV($input);
%}

%typemap(perl5,check) (size_t __maxlen, nuint8 * __nslist, size_t * __nslen) (size_t temp) %{
	$3 = &temp;
	New((I32)"$2_name", $2, $1, nuint8);
%}

%typemap(perl5,argout) (size_t __maxlen, nuint8 * __nslist, size_t * __nslen) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = buildArrayReferenceFromByteArray(*$3, $2);
	}
	argvi++;
%}

%typemap(perl5,freearg) (size_t __maxlen, nuint8 * __nslist, size_t * __nslen) %{
	Safefree($2);
%}

%typemap(perl5,in,numinputs=0) struct sockaddr * station_addr (struct sockaddr temp)
%{ $1 = &temp; %}

%typemap(perl5,argout) struct sockaddr * station_addr %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn((char*)$1, sizeof(*$1)));
	}
	argvi++;
%}

%typemap(perl5,out) u_int8_t data[0] %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if ($1) {
		size_t ro;
		size_t len;
		
		ro = (u_int8_t*)$1 - (u_int8_t*)arg1;
		len = SvCUR(sv_arg1);
		if (len > ro) {
			len -= ro;
		} else {
			len = 0;
		}
		$result = createLV(sv_arg1, ro, len);
	} else {
		$result = &PL_sv_undef;
	}
	argvi++;
%}

IV_BASED_OUTPUT(u_int8_t * conn_type);

/* ncp_send_broadcast */
%typemap(perl5,in) (u_int8_t no_conn, const u_int8_t * connections) {
	size_t asize;
	const char *msg;
	
	msg = getByteArray($input, &asize, &$2, 256);
	if (msg) {
		SWIG_croak("Type error in argument %u of %s. %s\n", $argnum, "$symname", msg);
		XSRETURN(1);
	}
	$1 = asize;
}

%typemap(perl5,freearg) (u_int8_t no_conn, const u_int8_t * connections)
%{ freeByteArray($2); %}
/* end of ncp_send_broadcast */

/* ncp_send_broadcast2 */
%typemap(perl5,in) (unsigned int no_conn, const unsigned int * connections) {
	size_t asize;
	const char *msg;
	
	msg = getUnsignedIntArray($input, &asize, &$2, 65536);
	if (msg) {
		SWIG_croak("Type error in argument %u of %s. %s\n", $argnum, "$symname", msg);
		XSRETURN(1);
	}
	$1 = asize;
}

%typemap(perl5,freearg) (unsigned int no_conn, const unsigned int * connections)
%{ freeUnsignedIntArray($2); %}
/* end of ncp_send_broadcast2 */

/* ncp_write */
#define FETCHPV(src,tgt,tgtlen)			\
	do {					\
		STRLEN len;			\
						\
		SvGETMAGIC(src);		\
		if (!SvOK(src)) {		\
			len = 0;		\
			tgt = NULL;		\
		} else {			\
			tgt = SvPV(src, len);	\
		}				\
		tgtlen = len;			\
	} while (0)

%typemap(perl5,in) (size_t IGNORE, const char * STRING_LENPREV)
%{ FETCHPV($input, $2, $1); %}
/* end of ncp_write */

/* ncp_get_encryption_key */
%typemap(perl5,in,numinputs=0) char * encryption_key ( char temp[8] )
%{ $1 = temp; %}

%typemap(perl5,argout) char * encryption_key %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn($1, 8));
	}
	argvi++;
%}
/* end of ncp_get_encryption_key */

/* ncp_read */
%typemap(perl5,in) (size_t count, char * RETBUFFER_LENPREV) %{
	$1 = SvUV($input);
	New((I32)"retbuffer_lenprev", $2, $1, char);
%}

%typemap(perl5,argout) (size_t count, char * RETBUFFER_LENPREV) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result < 0) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn($2, result));
	}
	argvi++;
%}

%typemap(perl5,freearg) (size_t count, char * RETBUFFER_LENPREV) %{
	Safefree($2);
%}
/* end of ncp_read */

IV_BASED_OUTPUT(int * target);

IV_BASED_OUTPUT(u_int16_t * OUTPUT);
IV_BASED_BOTH(u_int16_t * REFERENCE);

IV_BASED_BOTH(u_int16_t * iterHandle);

/* ncp_initialize_search2 */
%typemap(perl5,in) (const unsigned char * enc_subpath, int    subpathlen),
		   (const unsigned char * encpath,     size_t pathlen),
                   (const unsigned char * encpath,     int    pathlen)
%{ FETCHPV($input, $1, $2); %}

IV_BASED_OUTPUT(u_int32_t * queue_length);	/* ncp_get_queue_length */
IV_BASED_OUTPUT(u_int16_t * rights);		/* ncp_str_to_perms */

/* ncp_get_queue_job_ids */
%typemap(perl5,in,numinputs=0) (u_int32_t * length1, u_int32_t * length2, u_int32_t ids[]) (u_int32_t ids_size, u_int32_t reply_ids_size, u_int32_t ids_array[125]) %{
	$1 = &ids_size;
	$2 = &reply_ids_size;
	$3 = ids_array;
	ids_size = 125;
%}

%typemap(perl5,argout) (u_int32_t * length1, u_int32_t * length2, u_int32_t ids[]) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
		argvi++;
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVuv(ids_size));
		argvi++;
		$result = buildArrayReferenceFromDWordArray(reply_ids_size, ids_array);
	}
	argvi++;
%}
/* end of ncp_get_queue_job_ids */

/* ncp_get_broadcast_message */
%typemap(perl5,in,numinputs=0) char message[256] = fixedCharArray OUTPUT [ANY];
%typemap(perl5,argout) char message[256] = fixedCharArray OUTPUT [ANY];
/* end of ncp_get_broadcast_message */

/* ncp_add_trustee_set */
%typemap(perl5,in) (int object_count, const struct ncp_trustee_struct * rights) {
	size_t asize;
	const char *msg;
	
	msg = getNCPTrusteeStructArray($input, &asize, &$2, 2048, $2);
	if (msg) {
		SWIG_croak("Type error in argument %u of %s. %s\n", $argnum, "$symname", msg);
		XSRETURN(1);
	}
	$1 = asize;
}

%typemap(perl5,freearg) (int object_count, const struct ncp_trustee_struct * rights)
%{ freeNCPTrusteeStructArray($2); %}
/* end of ncp_add_trustee_set */

#define RETPTR_AND_MAXLEN(parm,deflen)				\
%typemap(perl5,default) parm %{ $2 = deflen; %}			\
%typemap(perl5,in)      parm %{ $2 = SvUV($input); %}		\
%typemap(perl5,check)   parm					\
%{ New((I32)"$1_name", $1, $2, $1_basetype); %}			\
%typemap(perl5,argout)	parm %{					\
	if (argvi >= items) {					\
		EXTEND(sp, 1);					\
	}							\
        if (result) {						\
                $result = &PL_sv_undef;				\
        } else {						\
                $result = sv_2mortal(newSVpvn($1, strlen($1)));	\
        }							\
        argvi++;						\
%}								\
%typemap(perl5,freearg)	parm %{					\
	Safefree($1);						\
%}

RETPTR_AND_MAXLEN((char * retname, size_t retname_maxlen), 256);
RETPTR_AND_MAXLEN((char * retname, int retname_maxlen), 256);
RETPTR_AND_MAXLEN((unsigned char * encbuff, int encbuffsize), 4096);

/* ncp_path_to_NW_format */
%typemap(perl5,out) int ncp_path_to_NW_format %{
	if (G_ARRAY == GIMME_V) {
		if (argvi >= items) {
			EXTEND(sp, 1);
		}
		if ($1 < 0) {
			$result = sv_2mortal(newSViv(-$1));
		} else {
			$result = sv_2mortal(newSViv(0));
		}
		argvi++;
	}
%}

%typemap(perl5,argout) (unsigned char * encbuff, int encbuffsize) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result < 0) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn($1, result));
	}
	argvi++;
%}
/* end of ncp_path_to_NW_format */

/* ncp_perms_to_str */
%typemap(perl5,in,numinputs=0) char r (char temp[11]) [11]
%{ $1 = temp; %}
/* end of ncp_perms_to_str */

/* ncp_namespace_to_str */
%typemap(perl5,in,numinputs=0) char r (char temp[5]) [5]
%{ $1 = temp; %}
/* end of ncp_namespace_to_str */

%constant NW_INFO_STRUCT = sizeof(struct nw_info_struct);
%constant NW_INFO_STRUCT2 = sizeof(struct nw_info_struct2);
%constant NW_INFO_STRUCT3 = sizeof(struct nw_info_struct3);

%extend nw_info_struct3 {
	~nw_info_struct3(void) { free(self->data); };
}

%typemap(perl5,in) (void * target, size_t sizeoftarget) (swig_type_info * sti) %{
	$2 = SvIV($input);
%}

%typemap(perl5,check) (void * target, size_t sizeoftarget) %{
	switch ($2) {
		case 0:
			sti = SWIGTYPE_p_void;
			break;
		case sizeof(struct nw_info_struct):
			sti = SWIGTYPE_p_nw_info_struct;
			break;
		case sizeof(struct nw_info_struct2):
			sti = SWIGTYPE_p_nw_info_struct2;
			break;
		case sizeof(struct nw_info_struct3):
			sti = SWIGTYPE_p_nw_info_struct3;
			break;
		default:
			SWIG_croak("Invalid value of argument %u of %s.\n", $argnum, "$symname");
			XSRETURN(1);
	}
	Newz((I32)"nw_info_target", $1, $2, unsigned char);
%}

%typemap(perl5,argout) (void * target, size_t sizeoftarget) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		SV* s;

		s = newSVpvn((char*)$1, $2);
		$result = createTiedHashReference(s, sti);
	}
	argvi++;
%}

%typemap(perl5,freearg) (void * target, size_t sizeoftarget)
%{ Safefree($1); %}

%constant NCP_NAMESPACE_FORMAT = sizeof(struct ncp_namespace_format);

%typemap(perl5,in) (struct ncp_namespace_format * format, size_t sizeofformat) (swig_type_info* sti)
%{ $2 = SvUV($input); %}

%typemap(perl5,check) (struct ncp_namespace_format * format, size_t sizeofformat) %{
	switch ($2) {
		case sizeof(struct ncp_namespace_format):
			sti = SWIGTYPE_p_ncp_namespace_format;
			break;
		default:
			SWIG_croak("Invalid value of argument %u of %s.\n", $argnum, "$symname");
			XSRETURN(1);
	}
	Newz((I32)"ncp_namespace_format", (unsigned char*)$1, $2, unsigned char);
%}

%typemap(perl5,argout) (struct ncp_namespace_format * format, size_t sizeofformat) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		SV* s;

		s = newSVpvn((char*)$1, $2);
		$result = createTiedHashReference(s, sti);
	}
	argvi++;
%}

%typemap(perl5,freearg) (struct ncp_namespace_format * format, size_t sizeofformat)
%{ Safefree($1); %}

%typemap(perl5,in) (const void * mnsbuf, size_t mnsbuflen)
%{ FETCHPV($input, $1, $2); %}

%typemap(perl5,in) (const void * nsibuf, size_t nsibuflen) = (const void * mnsbuf, size_t mnsbuflen);

%typemap(perl5,default) (void * itembuf, size_t * itembuflen, size_t itemmaxbuflen) (size_t tmp) %{
	$2 = &tmp;
	$3 = 1024;
%}

%typemap(perl5,default) (void * nsibuf, size_t * nsibuflen, size_t nsimaxbuflen) = (void * itembuf, size_t * itembuflen, size_t itemmaxbuflen);

%typemap(perl5,in) (void * itembuf, size_t * itembuflen, size_t itemmaxbuflen),
                   (void * nsibuf, size_t * nsibuflen, size_t nsimaxbuflen)
%{ $3 = SvUV($input); %}

%typemap(perl5,check) (void * itembuf, size_t * itembuflen, size_t itemmaxbuflen),
                      (void * nsibuf, size_t * nsibuflen, size_t nsimaxbuflen) %{
	Newz((I32)"$1_name", $1, $3, unsigned char);
%}

%typemap(perl5,argout) (void * itembuf, size_t * itembuflen, size_t itemmaxbuflen),
                       (void * nsibuf, size_t * nsibuflen, size_t nsimaxbuflen) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn($1, *$2));
	}
	argvi++;
%}

%typemap(perl5,freearg) (void * itembuf, size_t * itembuflen, size_t itemmaxbuflen),
                        (void * nsibuf, size_t * nsibuflen, size_t nsimaxbuflen)
%{ Safefree($1); %}

/* ncp_get_file_size */
%typemap(perl5,in,numinputs=0) NVBASED * OUTPUT ($*1_type temp)
%{ $1 = &temp; %}

%typemap(perl5,argout) NVBASED * OUTPUT %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVnv(temp));
	}
	argvi++;
%}

#define NV_BASED_OUTPUT(x)			\
%typemap(perl5,in,numinputs=0) x = NVBASED * OUTPUT;	\
%typemap(perl5,argout) x = NVBASED * OUTPUT;

NV_BASED_OUTPUT(ncp_off64_t * fileSize);
/* end of ncp_get_file_size */

%typemap(perl5,in) const char * 
%{ $1 = (char *) swig_PV_maynull_nolen($input); %}

%typemap(perl5,out) const char * 
%{ $result = sv_2mortal(newSVpv($1, 0)); argvi++; %}

%typemap(perl5,in) const char * __objectname = char * INPUT;

IV_BASED_OUTPUT(NWObjectType * __objecttype);
IV_BASED_OUTPUT(NWObjectID   * __objectid);
IV_BASED_OUTPUT(nuint8       * __rights);
IV_BASED_OUTPUT(nuint32      * __volnumber);
IV_BASED_OUTPUT(NWCONN_NUM   * __connnum);
IV_BASED_OUTPUT(u_int16_t    * version);
%typemap(perl5,in) const char * __objectpassword = char * INPUT;
%typemap(perl5,in) const u_int16_t = UVBASED input;
%typemap(perl5,in) nint16 = IVBASED input;

ASCIIZ_CHAR_ARRAY(char * __objectname, 48);
ASCIIZ_CHAR_ARRAY(char * __server, 48);
ASCIIZ_CHAR_ARRAY(char * __volume, 4096);
ASCIIZ_CHAR_ARRAY(char * __volpath, 4096);

RETPTR_AND_MAXLEN((char * __name, size_t __maxlen), 256);

FIXEDARRAY_IN(const char fileHandle[6]);
FIXEDARRAY_OUTPUT(char fileHandle[6]);

/* NWGetObjectConnectionNumbers */
%typemap(perl5,default) (size_t * noOfReturnedConns, NWCONN_NUM * conns, size_t maxConns)
%{ $3 = 256; %}

%typemap(perl5,in) (size_t * noOfReturnedConns, NWCONN_NUM * conns, size_t maxConns)
%{ $3 = SvUV($input); %}

%typemap(perl5,check) (size_t * noOfReturnedConns, NWCONN_NUM * conns, size_t maxConns) %{
	New((I32)"$2_name", $2, $3, NWCONN_NUM);
	$1 = &$3;
%}

%typemap(perl5,argout) (size_t * noOfReturnedConns, NWCONN_NUM * conns, size_t maxConns) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = buildArrayReferenceFromNWConnArray(*$1,$2);
	}
	argvi++;
%}

%typemap(perl5,freearg) (size_t * noOfReturnedConns, NWCONN_NUM * conns, size_t maxConns)
%{ Safefree($2); %}
/* end of NWGetObjectConnectionNumbers */

/* NWGetConnListFromObject */
%typemap(perl5,in,numinputs=0) (size_t * noOfReturnedConns, NWCONN_NUM * conns125) (size_t temp) %{
	$1 = &temp;
	New((I32)"$2_name", $2, 125, NWCONN_NUM); 
%}

%typemap(perl5,argout) (size_t * noOfReturnedConns, NWCONN_NUM * conns125) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = buildArrayReferenceFromNWConnArray(*$1,$2);
	}
	argvi++;
%}

%typemap(perl5,freearg) (size_t * noOfReturnedConns, NWCONN_NUM * conns125)
%{ Safefree($2); %}
/* end of NWGetConnListFromObject */

/* NWSendBroadcastMessage */
%typemap(perl5,in) (size_t conns, NWCONN_NUM * connArray, nuint8 * deliveryStatus) {
	const char *msg;
	
	msg = getNWConnArray($input, &$1, &$2, 65536);
	if (msg) {
		SWIG_croak("Type error in argument %u of %s. %s\n", $argnum, "$symname", msg);
		XSRETURN(1);
	}
	New((I32)"$3_name", $3, $1, nuint8);
}

%typemap(perl5,argout) (size_t conns, NWCONN_NUM * connArray, nuint8 * deliveryStatus) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = buildArrayReferenceFromByteArray($1,$3);
	}
	argvi++;
%}

%typemap(perl5,freearg) (size_t conns, NWCONN_NUM * connArray, nuint8 * deliveryStatus)
%{ freeNWConnArray($2);
   Safefree($3); %}
/* end of NWSendBroadcastMessage */

/* NWGetNumberNCPExtensions */
IV_BASED_OUTPUT(nuint * __exts);
/* end of NWGetNumberNCPExtensions */

/* NWScanNCPExtensions */
IV_BASED_BOTH(nuint32 * __iter);
ASCIIZ_CHAR_ARRAY(char * __extname, 32);
IV_BASED_OUTPUT(nuint8 * __majorVersion);
IV_BASED_OUTPUT(nuint8 * __minorVersion);
IV_BASED_OUTPUT(nuint8 * __revision);
FIXEDARRAY_OUTPUT(nuint8 __queryData[32]);
/* end of NWScanNCPExtensions */

/* NWGetBroadcastMode */
IV_BASED_OUTPUT(nuint16 * __bcstmode);
/* end of NWGetBroadcastMode */

/* NWGetObjDiskRestrictions */
IV_BASED_OUTPUT(nuint32 * restriction);
IV_BASED_OUTPUT(nuint32 * inUse);
/* end of NWGetObjDiskRestrictions */

IV_BASED_BOTH(nuint32 * iterhandle);

/* NWScanVolDiskRestrictions */
PV_BASED_OUTPUT(NWVolumeRestrictions * volInfo);
/* end of NWScanVolDiskRestrictions */

/* NWScanVolDiskRestrictions2 */
PV_BASED_OUTPUT(NWVOL_RESTRICTIONS * volInfo);
/* end of NWScanVolDiskRestrictions2 */

/* NWGetDirSpaceLimitList */
FIXEDARRAYLEN_OUTPUT(nuint8 * __buffer512, 512);
/* end of NWGetDirSpaceLimitList */

/* NWGetDirSpaceLimitList2 */
PV_BASED_OUTPUT(NW_LIMIT_LIST * limitlist);
/* end of NWGetDirSpaceLimitList2 */

/* ncp_get_directory_info */
PV_BASED_OUTPUT(DIR_SPACE_INFO * target);
/* end of ncp_get_directory_info */

/* NWOpenSemaphore */
IV_BASED_OUTPUT(nuint32 * semHandle);
IV_BASED_OUTPUT(nuint16 * semOpenCount);
/* end of NWOpenSemaphore */

/* NWExamineSemaphore */
IV_BASED_OUTPUT(nint16 * semValue);
/* nuint16 * semOpenCount */
/* end of NWExamineSemaphore */

/* NWCCGetConnAddressLength */
IV_BASED_OUTPUT(nuint32 * bufLen);
/* end of NWCCGetConnAddressLength */

/* NWCCGetConnAddress */
%typemap(perl5,out) NWCCTranAddr_buffer buffer[32] %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	$result = sv_2mortal(newSVpvn($1, arg1->len));
	argvi++;
%}
%typemap(perl5,in) NWCCTranAddr_buffer buffer [32] (STRLEN temp) %{
	SvGETMAGIC($input);
 	if (!SvOK($input)) {
		$1 = NULL;
	} else {
		$1 = SvPV($input, temp);
	}
%}
%typemap(perl5,memberin) NWCCTranAddr_buffer buffer[32] %{
	if ($input) {
		if (temp2 > $1_dim0) {
			temp2 = $1_dim0;
		}
		memcpy($1, $input, temp2);
		arg1->len = temp2;
	}
%}

%typemap(perl5,in,numinputs=0) (nuint32 bufLen, NWCCTranAddr * tranAddr) (NWCCTranAddr temp)
%{ $1 = sizeof(temp.bufferdata);
   temp.buffer = temp.bufferdata;
   $2 = &temp; %}
%typemap(perl5,argout) (nuint32 bufLen, NWCCTranAddr * tranAddr) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		SV* sv;
		
		sv = newSVpvn((const char*)$2, sizeof(*$2));
		$result = createTiedHashReference(sv, $2_descriptor);
	}
	argvi++;
%}
/* end of NWCCGetConnAddress */

%typemap(perl5,in) const struct sockaddr* addr = char * INPUT;

/* NWCCGetConnInfo */
%typemap(perl5,in) (nuint info, size_t conninfolen, void * conninfoaddr) 
(int type, swig_type_info * tp, 
char achar[512],
UV aUV,
NWCCVersion aserver_version,
NWCCTranAddr atran_addr,
struct NWCCRootEntry aroot_entry
) %{
	tp = NULL;
	type = 0;
	$1 = SvUV($input);
	switch ($1) {
		case NWCC_INFO_BCAST_STATE:
		case NWCC_INFO_MOUNT_UID:
		case NWCC_INFO_SECURITY:
		case NWCC_INFO_MAX_PACKET_SIZE:
		case NWCC_INFO_CONN_NUMBER:
		case NWCC_INFO_USER_ID:
			$2 = sizeof(aUV);
			$3 = &aUV;
			break;
		case NWCC_INFO_SERVER_NAME:
		case NWCC_INFO_USER_NAME:
		case NWCC_INFO_TREE_NAME:
			$2 = sizeof(achar);
			$3 = achar;
			type = 1;
			break;
		case NWCC_INFO_SERVER_VERSION:
			$2 = sizeof(aserver_version);
			$3 = &aserver_version;
			type = 2;
			tp = SWIGTYPE_p_NWCCVersion;
			break;
		case NWCC_INFO_TRAN_ADDR:
			$2 = sizeof(atran_addr);
			$3 = &atran_addr;
			atran_addr.buffer = atran_addr.bufferdata;
			atran_addr.len = sizeof(atran_addr.bufferdata);
			type = 2;
			tp = SWIGTYPE_p_NWCCTranAddr;
			break;
		case NWCC_INFO_ROOT_ENTRY:
			$2 = sizeof(aroot_entry);
			$3 = &aroot_entry;
			type = 2;
			tp = SWIGTYPE_p_NWCCRootEntry;
			break;
		case NWCC_INFO_AUTHENT_STATE:
		case NWCC_INFO_MOUNT_POINT:
		default:
			$2 = 0;
			$3 = NULL;
			break;
	}
%}

%typemap(perl5,argout) (nuint info, size_t conninfolen, void * conninfoaddr) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		switch (type) {
			case 0:
				$result = sv_2mortal(newSVuv(aUV));
				break;
			case 1:
				$result = sv_2mortal(newSVpv(achar, 0));
				break;
			case 2:
				{
					SV * sv;
					
					sv = newSVpvn($3, $2);
					$result = createTiedHashReference(sv, tp);
				}
				break;
			default:
				SWIG_croak("Undefined case in NWCCopts parsing\n");
				$result = &PL_sv_undef;
		}
	}
	argvi++;
%}
/* end of NWCCGetConnInfo */

/* ncp_ea_ */
%typemap(perl5,in) (const void * key, size_t keyLen)
%{ FETCHPV($input, $1, $2); %}

%typemap(perl5,default) (void * data, size_t datalen, size_t * rdatalen) %{
	$2 = 1024;
%}
%typemap(perl5,in) (void * data, size_t datalen, size_t * rdatalen) %{
	$2 = SvUV($input);
%}
%typemap(perl5,check) (void * data, size_t datalen, size_t * rdatalen) %{
	New((I32)"$1_name", $1, $2, unsigned char);
	$3 = &$2;
%}
%typemap(perl5,argout) (void * data, size_t datalen, size_t * rdatalen) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn($1, $2));
	}
	argvi++;
%}
%typemap(perl5,freearg) (void * data, size_t datalen, size_t * rdatalen) %{
	Safefree($1);
%}

%typemap(perl5,in) (const unsigned char * buffer, const unsigned char * endbuf) {
	STRLEN tmp;
	
	$1 = SvPV($input, tmp);
	$2 = $1 + tmp;
	arg6 = &$1;
}
%typemap(perl5,argout) (const unsigned char * buffer, const unsigned char * endbuf) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn($1, $2 - $1));
	}
	argvi++;
%}

%typemap(perl5,default) (struct ncp_ea_info_level1 * info, size_t maxsize,
		size_t * needsize),
			(struct ncp_ea_info_level6 * info, size_t maxsize,
		size_t * needsize),
			(char * key, size_t maxsize, size_t * needsize) %{
	$2 = 512;
%}
%typemap(perl5,in) (struct ncp_ea_info_level1 * info, size_t maxsize,
		size_t * needsize),
		   (struct ncp_ea_info_level6 * info, size_t maxsize,
		size_t * needsize),
		   (char * key, size_t maxsize, size_t * needsize) %{
	$2 = SvUV($input);
%}
%typemap(perl5,check) (struct ncp_ea_info_level1 * info, size_t maxsize,
		size_t * needsize),
		      (struct ncp_ea_info_level6 * info, size_t maxsize,
		size_t * needsize),
		      (char * key, size_t maxsize, size_t * needsize) %{
	Newc((I32)"$1_name", $1, $2, unsigned char, $*1_type);
	$3 = &$2;
%}
%typemap(perl5,argout) (struct ncp_ea_info_level1 * info, size_t maxsize,
		size_t * needsize) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		SV * sv;
		
		sv = newSVpvn((char*)$1, $2);
		$result = createTiedHashReference(sv, $1_descriptor);
	}
	argvi++;
%}
%typemap(perl5,argout) (struct ncp_ea_info_level6 * info, size_t maxsize,
		size_t * needsize) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		SV * sv;
		
		sv = newSVpvn((char*)$1, $2);
		$result = createTiedHashReference(sv, $1_descriptor);
	}
	argvi++;
%}
%typemap(perl5,argout) (char * key, size_t maxsize, size_t * needsize) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result || !$2) {
		$result = &PL_sv_undef;
	} else {
		$result = sv_2mortal(newSVpvn($1, $2 - 1));
	}
	argvi++;
%}
%typemap(perl5,freearg) (struct ncp_ea_info_level1 * info, size_t maxsize,
		size_t * needsize),
		        (struct ncp_ea_info_level6 * info, size_t maxsize,
		size_t * needsize),
		        (char * key, size_t maxsize, size_t * needsize)
%{ Safefree($1); %}
%typemap(perl5,in,numinputs=0) const unsigned char ** next "";
/* end of ncp_ea_ */

/* ncp_ea_read */
PV_BASED_OUTPUT(struct ncp_ea_read_info * info);
/* end of ncp_ea_read */

/* ncp_ea_write */
PV_BASED_OUTPUT(struct ncp_ea_write_info * info);
%typemap(perl5,in) (const void * data, size_t datalen)
%{ FETCHPV($input, $1, $2); %}
/* end of ncp_ea_write */

/* ncp_ea_duplicate */
UV_BASED_OUTPUT(u_int32_t * duplicateCount);
UV_BASED_OUTPUT(u_int32_t * dataSizeDuplicated);
UV_BASED_OUTPUT(u_int32_t * keySizeDuplicated);
/* end of ncp_ea_duplicate */

/* nwnet.h begins here *********************************************** */

%typemap(perl5,in,numinputs=0) NWDSContextHandle * = PTRBASED *;
%typemap(perl5,argout) NWDSContextHandle * = PTRBASED *;
%typemap(perl5,perl5out) NWDSContextHandle "";
%typemap(perl5,perl5out) struct __NWDSContextHandle   "";
%extend __NWDSContextHandle {
	__NWDSContextHandle(NWDSContextHandle ctxHandleForCopy) {
		NWDSContextHandle ctx;
		NWDSCCODE dserr;
		
		if (ctxHandleForCopy) {
			dserr = NWDSDuplicateContextHandle(ctxHandleForCopy, &ctx);
		} else {
			dserr = NWDSCreateContextHandle(&ctx);
		}
		if (dserr) {
			return (NWDSContextHandle)ERR_CONTEXT_CREATION;
		}
		return ctx;
	}
	~__NWDSContextHandle() {
		if (self) {
			NWDSFreeContext(self);
		}
	}
}
%typemap(perl5,default) NWDSContextHandle ctxHandleForCopy
%{ $1 = NULL; %}
%typemap(perl5,out) struct __NWDSContextHandle * %{
	if (argvi >= items) {
  		EXTEND(sp, 1);
	}
	$result = sv_newmortal();
	if (result != (NWDSContextHandle)ERR_CONTEXT_CREATION) {
		SWIG_MakePtr($result, $1, $1_descriptor, 0);
	}
	argvi++;
%}
%typemap(perl5,in) NWDSContextHandle %{
        if (!SwigConvertPtr($input, (void **) &$1, $1_descriptor)) {
		SWIG_croak("Type error in argument %u of %s. Expected %s", $argnum, "$symname", $1_descriptor->name);
	}
%}
%typemap(perl5,in) NWDSContextHandle ctxToFree {
	SV* inpsv;

        inpsv = SwigConvertPtr($input, (void **) &$1, $1_descriptor);
	if (!inpsv) {
		SWIG_croak("Type error in argument %u of %s. Expected %s", $argnum, "$symname", $1_descriptor->name);
	}
	sv_setiv(inpsv, 0);
}
/* Make sure that real definition comes AFTER all related typemaps... Otherwise some typemaps do not work */
struct __NWDSContextHandle { };


PTR_BASED_OUTPUT(pBuf_T);
%typemap(perl5,default) size_t Buf_T_size
%{	$1 = DEFAULT_MESSAGE_LEN;	%}
%typemap(perl5,perl5out) struct tagBuf_T "";
%extend tagBuf_T {
	tagBuf_T(size_t Buf_T_size) {
		Buf_T* buf;
		NWDSCCODE dserr;
		
		dserr = NWDSAllocBuf(Buf_T_size, &buf);
		if (dserr) {
			buf = NULL;
		}
		return buf; 
	};
	~tagBuf_T() {
		if (self) {
			NWDSFreeBuf(self);
		}
	}
}
%typemap(perl5,default) BUFT * OUTPUT (size_t buflen, int alloced, SV* inpsv)
%{	buflen = DEFAULT_MESSAGE_LEN; 
	alloced = 0;
	inpsv = NULL;
	$1 = NULL;
%}
%typemap(perl5,in) BUFT * OUTPUT
%{
        inpsv = SwigConvertPtr($input, (void **) &$1, $1_descriptor);
	if (!inpsv) {
		if (!SvIOK($input)) {
			SWIG_croak("Type error in argument %u of %s. Expected %s or buffer length", $argnum, "$symname", $1_descriptor->name);
		}
		buflen = SvUV($input);
        } else {
		alloced = 1;
	}
%}
%typemap(perl5,check) BUFT * OUTPUT %{
	if (!alloced) {
		NWDSCCODE result;

		if (G_ARRAY != GIMME_V) {
			SWIG_croak("You must use array return value if you are passing only buffer size into %s", "$symname");
		}			
		result = NWDSAllocBuf(buflen, &$1);
		if (result) {
   			ST(0) = sv_2mortal(newSViv(result));
			XSRETURN(1);
		}
	}
%}
%typemap(perl5,argout) BUFT * OUTPUT %{
	if (G_ARRAY == GIMME_V) {
		if (argvi >= items) {
			EXTEND(sp, 1);
		}
		if (result) {
   			$result = &PL_sv_undef;
			if (!alloced) {
				NWDSFreeBuf($1);
			}
		} else if (inpsv) {
			$result = sv_2mortal(newRV_inc(inpsv));
		} else {
			$result = sv_newmortal();
			SWIG_MakePtr($result, $1, $1_descriptor, 0);
		}
		argvi++;
	}
%}
%typemap(perl5,in) pBuf_T freebuf {
	SV* inpsv;

        inpsv = SwigConvertPtr($input, (void **) &$1, $1_descriptor);
	if (!inpsv) {
		SWIG_croak("Type error in argument %u of %s. Expected %s", $argnum, "$symname", $1_descriptor->name);
	}
	sv_setiv(inpsv, 0);
}

#define BUFT_OUTPUT(x) \
%typemap(perl5,default) Buf_T * x = BUFT * OUTPUT;	\
%typemap(perl5,in)	Buf_T * x = BUFT * OUTPUT;	\
%typemap(perl5,check)	Buf_T * x = BUFT * OUTPUT;	\
%typemap(perl5,argout)	Buf_T * x = BUFT * OUTPUT;

IV_BASED_BOTH_MN(nuint32 * iterHandle);

UV_BASED_OUTPUT(size_t * size);
UV_BASED_OUTPUT(NWObjectCount * count);
UV_BASED_OUTPUT(NWObjectCount * valcount);
UV_BASED_OUTPUT(NWObjectCount * attrs);
UV_BASED_OUTPUT(NWObjectCount * addrcnt);
UV_BASED_OUTPUT(NWObjectCount * countObjectsSearched);	/* NWDSSearch */
UV_BASED_OUTPUT(NWObjectCount * syntaxCount);	/* NWDSGetSyntaxCount */
UV_BASED_OUTPUT(enum SYNTAX * syntaxID);
UV_BASED_OUTPUT(nuint32 * flags);	/* NWDSGetDSVerInfo, ... */
UV_BASED_OUTPUT(nuint32 * replicaType);
UV_BASED_OUTPUT(nuint32 * timev);
UV_BASED_OUTPUT(nuint32 * dsVersion);	/* NWDSGetDSVerInfo */
UV_BASED_OUTPUT(nuint32 * rootMostEntryDepth); /* NWDSGetDSVerInfo */
UV_BASED_OUTPUT(nuint32 * privileges);	/* NWDSGetEffectiveRights */
UV_BASED_OUTPUT(nuint32 * numberOfTrees);	/* NWDSReturnBlockOfAvailableTrees */
UV_BASED_OUTPUT(nuint32 * totalUniqueTrees);	/* NWDSReturnBlockOfAvailableTrees */
UV_BASED_OUTPUT(nuint32 * OUTPUT);	/* __NWGetFileServerUTCTime */
UV_BASED_OUTPUT(nuint32 * OUTPUT2);	/* __NWGetFileServerUTCTime */
UV_BASED_OUTPUT(nuint32 * OUTPUT3);	/* __NWGetFileServerUTCTime */
UV_BASED_OUTPUT(nuint32 * OUTPUT4);	/* __NWGetFileServerUTCTime */
UV_BASED_OUTPUT(nuint32 * OUTPUT5);	/* __NWGetFileServerUTCTime */
UV_BASED_OUTPUT(nbool8 * matched);	/* __NWDSCompare, NWDSCompare */

IV_BASED_OUTPUT(NWObjectID * ID);	/* NWDSMapNameToID, NWDSResolveName */

IV_BASED_BOTH(nint32 * scanIndex);	/* NWDSScanForAvailableTrees */

PV_BASED(Attr_Info_T);
PV_BASED_OUTPUT(Attr_Info_T * attrInfo);

PV_BASED(Class_Info_T);
PV_BASED_OUTPUT(Class_Info_T * classInfo);

PV_BASED(Object_Info_T);
PV_BASED_OUTPUT(Object_Info_T * oit);

PV_BASED(Syntax_Info_T);
PV_BASED_OUTPUT(Syntax_Info_T * syntaxDef);

PV_BASED_STRUCT(tagAsn1ID_T);
PV_BASED_MEMBER(Attr_Info_T, asn1ID, Asn1ID_T);
PV_BASED_MEMBER(Class_Info_T, asn1ID, Asn1ID_T);

PV_BASED(TimeStamp_T);

BUFT_OUTPUT(serverAddresses);
BUFT_OUTPUT(addrBuf);
BUFT_OUTPUT(objectList);
BUFT_OUTPUT(objectInfo);
BUFT_OUTPUT(privilegeInfo);
BUFT_OUTPUT(partitions);
BUFT_OUTPUT(containableClasses);
BUFT_OUTPUT(attrDefs);
BUFT_OUTPUT(classDefs);
BUFT_OUTPUT(syntaxDefs);
BUFT_OUTPUT(initBuf);

%runtime %{
_define nwdschar2mortalsv(str) _nwdschar2mortalsv(aTHX_ str)
static SV * _nwdschar2mortalsv(pTHX_ NWDSChar * str) {
	SV * tmp;
	
	tmp = sv_2mortal(newSVpv(str, 0));
	SvUTF8_on(tmp);
	return tmp;
}
%}

%typemap(perl5,in,numinputs=0) NWDSChar * OUTPUT ( NWDSChar tempstr[MAX_DN_BYTES] )
%{ $1 = tempstr; %}
%typemap(perl5,argout) NWDSChar * OUTPUT %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		$result = nwdschar2mortalsv($1);
	}
	argvi++;
%}
%typemap(perl5,in) const NWDSChar *
%{ $1 = swig_PV_maynull_nolen($input); %}

#define NWDSCHAR_OUTPUT(x)					\
%typemap(perl5,in,numinputs=0) NWDSChar * x = NWDSChar * OUTPUT;	\
%typemap(perl5,argout) NWDSChar * x = NWDSChar * OUTPUT;

NWDSCHAR_OUTPUT(attrName);
NWDSCHAR_OUTPUT(binderyEmulationContext);
NWDSCHAR_OUTPUT(className);
NWDSCHAR_OUTPUT(distName);
NWDSCHAR_OUTPUT(dst);
NWDSCHAR_OUTPUT(item);
NWDSCHAR_OUTPUT(myDN);
NWDSCHAR_OUTPUT(name);
NWDSCHAR_OUTPUT(partitionRoot);
NWDSCHAR_OUTPUT(serverDN);
NWDSCHAR_OUTPUT(syntaxName);
NWDSCHAR_OUTPUT(treeName);

/* NWIsDSServer */
%typemap(perl5,in,numinputs=0) char * treename (char treename[MAX_TREE_NAME_BYTES]) %{
	treename[0] = 0;
	$1 = treename;
%}
%typemap(perl5,argout) char * treename %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	$result = sv_2mortal(newSVpv($1, 0));
	argvi++;
%}
/* end of NWIsDSServer */

/* NWDSGetNDSStatistics */
PV_BASED(NDSStatsInfo_T);
%typemap(perl5,in,numinputs=0) (size_t statsInfoLen, NDSStatsInfo_T* statsInfo) (NDSStatsInfo_T statbuf) %{
	$2 = &statbuf;
	$1 = sizeof(statbuf);
%}
%typemap(perl5,argout) (size_t statsInfoLen, NDSStatsInfo_T* statsInfo) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result) {
		$result = &PL_sv_undef;
	} else {
		SV* s;

		s = newSVpvn((char*)$2, sizeof(*$2));
		$result = createTiedHashReference(s, $2_descriptor);
	}
	argvi++;
%}
/* end of NWDSGetNDSStatistics */

/* NWDSGetDSVerInfo */
ASCIIZ_CHAR_ARRAY(char * sapName, 48);
WCHART_ARRAY(wchar_t * treeName, 48);
/* end of NWDSGetDSVerInfo */

/* NWDSGetAttrVal */
%typemap(perl5,default) (enum SYNTAX synt, void * val) (int syntok)
%{	syntok = 0;	%}
%typemap(perl5,in) (enum SYNTAX synt, void * val) %{
	SvGETMAGIC($input);
	if (SvOK($input)) {
		$1 = SvUV($input);
		syntok = 1;
	}
%}
%typemap(perl5,check) (enum SYNTAX synt, void * val) %{
	if (syntok) {
		size_t len;
		result = NWDSComputeAttrValSize(arg1, arg2, $1, &len);
		if (result) {
   			ST(0) = sv_2mortal(newSViv(result));
			XSRETURN(1);
		}
		New((I32)"NWDSGetAttrVal_data", $2, len, unsigned char);
	} else {
		$1 = SYN_OCTET_STRING;
		$2 = NULL;
	}
%}
%typemap(perl5,argout) (enum SYNTAX synt, void * val) %{
	if (argvi >= items) {
		EXTEND(sp, 1);
	}
	if (result || !syntok) {
		$result = &PL_sv_undef;
	} else {
		switch ($1) {
			case SYN_DIST_NAME:
			case SYN_CE_STRING:
			case SYN_CI_STRING:
			case SYN_PR_STRING:
			case SYN_NU_STRING:
			case SYN_TEL_NUMBER:
			case SYN_CLASS_NAME:
				$result = sv_2mortal(newSVpv($2, 0));
				break;
			case SYN_BOOLEAN:
				$result = sv_2mortal(newSVuv(*(Boolean_T*)$2));
				break;
			case SYN_INTEGER:
			case SYN_INTERVAL:
				$result = sv_2mortal(newSViv(*(Integer_T*)$2));
				break;
			case SYN_OCTET_STRING:
			case SYN_STREAM:
				$result = sv_2mortal(newSVpvn(((Octet_String_T*)$2)->data, ((Octet_String_T*)$2)->length));
				break;
			case SYN_COUNTER:
				$result = sv_2mortal(newSViv(*(Counter_T*)$2));
				break;
			case SYN_TIME:
				$result = sv_2mortal(newSViv(*(Time_T*)$2));
				break;
			case SYN_OBJECT_ACL:
				{
					Object_ACL_T* acl = $2;
					HV* hv = newHV();
					newHVitemPV(hv, "protectedAttrName", acl->protectedAttrName);
					newHVitemPV(hv, "subjectName", acl->subjectName);
					newHVitemUV(hv, "privileges", acl->privileges);
					$result = sv_2mortal(newRV_noinc((SV*)hv));
				}
				break;
			case SYN_BACK_LINK:
				{
					Back_Link_T* bl = $2;
					HV* hv = newHV();
					newHVitemPV(hv, "objectName", bl->objectName);
					newHVitemUV(hv, "remoteID", bl->remoteID);
					$result = sv_2mortal(newRV_noinc((SV*)hv));
				}
				break;
			case SYN_HOLD:
				{
					Hold_T* h = $2;
					HV* hv = newHV();
					newHVitemPV(hv, "objectName", h->objectName);
					newHVitemUV(hv, "amount", h->amount);
					$result = sv_2mortal(newRV_noinc((SV*)hv));
				}
				break;
			case SYN_TYPED_NAME:
				{
					Typed_Name_T* tn = $2;
					HV* hv = newHV();
					newHVitemPV(hv, "objectName", tn->objectName);
					newHVitemUV(hv, "level", tn->level);
					newHVitemUV(hv, "interval", tn->interval);
					$result = sv_2mortal(newRV_noinc((SV*)hv));
				}
				break;
			case SYN_TIMESTAMP:
				$result = createTiedHashReference(newSVpvn($2, sizeof(TimeStamp_T)), SWIGTYPE_p_TimeStamp_T);
				break;
			case SYN_PATH:
				{
					Path_T* p = $2;
					HV* hv = newHV();
					newHVitemUV(hv, "nameSpaceType", p->nameSpaceType);
					newHVitemPV(hv, "volumeName", p->volumeName);
					newHVitemPV(hv, "path", p->path);
					$result = sv_2mortal(newRV_noinc((SV*)hv));
				}
				break;
			case SYN_EMAIL_ADDRESS:
				{
					EMail_Address_T* ea = $2;
					HV* hv = newHV();
					newHVitemUV(hv, "type", ea->type);
					newHVitemPV(hv, "address", ea->address);
					$result = sv_2mortal(newRV_noinc((SV*)hv));
				}
				break;
			case SYN_PO_ADDRESS:
				{
					NWDSChar** po = $2;
					AV* av = newAV();
					int i;
					
					for (i = 0; i < 6; i++) {
						newAVitemPV(av, *po);
					}
					$result = sv_2mortal(newRV_noinc((SV*)av));
				}
				break;
			case SYN_CI_LIST:
				{
					CI_List_T* ci;
					AV* av = newAV();
					
					for (ci = $2; ci; ci = ci->next) {
						newAVitemPV(av, ci->s);
					}
					$result = sv_2mortal(newRV_noinc((SV*)av));
				}
				break;
			case SYN_OCTET_LIST:
				{
					Octet_List_T* ol;
					AV* av = newAV();
					
					for (ol = $2; ol; ol = ol->next) {
						av_push(av, newSVpvn(ol->data, ol->length));
					}
					$result = sv_2mortal(newRV_noinc((SV*)av));
				}
				break;
			case SYN_NET_ADDRESS:
				{
					Net_Address_T* na = $2;
					HV* hv = newHV();
					newHVitemUV(hv, "addressType", na->addressType);
					hv_safe_store(hv, "address", newSVpvn(na->address, na->addressLength));
					$result = sv_2mortal(newRV_noinc((SV*)hv));
				}
				break;
			case SYN_FAX_NUMBER:
				{
					Fax_Number_T* fn = $2;
					HV* hv = newHV();
					HV* hv_params = newHV();
					newHVitemPV(hv, "telephoneNumber", fn->telephoneNumber);
					newHVitemUV(hv_params, "numOfBits", fn->parameters.numOfBits);
					hv_safe_store(hv_params, "data", newSVpvn(fn->parameters.data, (fn->parameters.numOfBits + 7) >> 3));
					hv_safe_store(hv, "parameters", newRV_noinc((SV*)hv_params));
					$result = sv_2mortal(newRV_noinc((SV*)hv));
				}
				break;
			case SYN_REPLICA_POINTER:
				{
					Replica_Pointer_T* rp = $2;
					HV* hv = newHV();
					AV* av = newAV();
					UV idx;
					Net_Address_T* na;
					newHVitemPV(hv, "serverName", rp->serverName);
					newHVitemUV(hv, "replicaType", rp->replicaType);
					newHVitemUV(hv, "replicaNumber", rp->replicaNumber);
					hv_safe_store(hv, "replicaAddressHint", newRV_noinc((SV*)av));
					$result = sv_2mortal(newRV_noinc((SV*)hv));
					na = rp->replicaAddressHint;
					for (idx = rp->count; idx--; na++) {
						HV* hv2 = newHV();
						newHVitemUV(hv2, "addressType", na->addressType);
						hv_safe_store(hv2, "address", newSVpvn(na->address, na->addressLength));
						av_push(av, newRV_noinc((SV*)hv2));
					}
				}
				break;
			default:
				$result = &PL_sv_undef;
				break;
		}
	}
	argvi++;
%}
%typemap(perl5,freearg) (enum SYNTAX synt, void * val) %{
	Safefree($2);
%}
/* end of NWDSGetAttrVal */

/* NWDSGetAttrValModTime */
PV_BASED_OUTPUT(TimeStamp_T * stamp);
/* end of NWDSGetAttrValModTime */

/* NWDSPutAttrVal, NWDSPutAttrNameAndVal, NWDSPutAttrChangeAndVal */
%wrapper %{
_define getHV(inp) _getHV(aTHX_ inp)
static HV * _getHV(pTHX_ SV* inp) {
	SV* r;
	
	SvGETMAGIC(inp);
	if (!SvROK(inp)) {
		Perl_croak(aTHX_ "Expected reference, but found something else");
		return NULL;
	}
	r = SvRV(inp);
	if (SvTYPE(r) != SVt_PVHV) {
		Perl_croak(aTHX_ "Expected reference to hash, but found reference to something else");
		return NULL;
	}
	return (HV*)r;
}

_define checkAttrVal(inp,synt,nfree) _checkAttrVal(aTHX_ inp, synt, nfree)
static void * _checkAttrVal(pTHX_ SV* inp, enum SYNTAX synt, int * nfree) {
	*nfree = 1;
	switch (synt) {
		case SYN_DIST_NAME:
		case SYN_CE_STRING:
		case SYN_CI_STRING:
		case SYN_PR_STRING:
		case SYN_NU_STRING:
		case SYN_TEL_NUMBER:
		case SYN_CLASS_NAME:
			*nfree = 0;
			return swig_PV_maynull_nolen(inp);
		case SYN_BOOLEAN:
			{
				Boolean_T* b;
				
				New((I32)"checkAttrVal_Boolean_T", b, 1, Boolean_T);
				*b = SvUV(inp);
				return b;
			}
		case SYN_INTEGER:
		case SYN_INTERVAL:
			{
				Integer_T* i;
				
				New((I32)"checkAttrVal_Integer_T", i, 1, Integer_T);
				*i = SvIV(inp);
				return i;
			}
		case SYN_OCTET_STRING:
		case SYN_STREAM:
			{
				Octet_String_T* os;
				STRLEN len;
				
				New((I32)"checkAttrVal_Octet_String_T", os, 1, Octet_String_T);
				os->data = SvPV(inp, len);
				os->length = len;
				return os;
			}
		case SYN_COUNTER:
			{
				Counter_T* c;
				
				New((I32)"checkAttrVal_Counter_T", c, 1, Counter_T);
				*c = SvIV(inp);
				return c;
			}
		case SYN_TIME:
			{
				Time_T* t;
				
				New((I32)"checkAttrVal_Time_T", t, 1, Time_T);
				*t = SvIV(inp);
				return t;
			}
		case SYN_OBJECT_ACL:
			{
				Object_ACL_T* acl;
				HV* hv;
				
				hv = getHV(inp);
				New((I32)"checkAttrVal_Object_ACL_T", acl, 1, Object_ACL_T);
				acl->protectedAttrName = fetchHVitemPV_nolen(hv, "protectedAttrName");
				acl->subjectName = fetchHVitemPV_nolen(hv, "subjectName");
				acl->privileges = fetchHVitemUV(hv, "privileges");
				return acl;
			}
		case SYN_BACK_LINK:
			{
				Back_Link_T* bl;
				HV* hv;
				
				hv = getHV(inp);
				New((I32)"checkAttrVal_Back_Link_T", bl, 1, Back_Link_T);
				bl->objectName = fetchHVitemPV_nolen(hv, "objectName");
				bl->remoteID = fetchHVitemUV(hv, "remoteID");
				return bl;
			}
		case SYN_HOLD:
			{
				Hold_T* h;
				HV* hv;
				
				hv = getHV(inp);
				New((I32)"checkAttrVal_Hold_T", h, 1, Hold_T);
				h->objectName = fetchHVitemPV_nolen(hv, "objectName");
				h->amount = fetchHVitemUV(hv, "amount");
				return h;
			}
		case SYN_TYPED_NAME:
			{
				Typed_Name_T* tn;
				HV* hv;
				
				hv = getHV(inp);
				New((I32)"checkAttrVal_Typed_Name_T", tn, 1, Typed_Name_T);
				tn->objectName = fetchHVitemPV_nolen(hv, "objectName");
				tn->level = fetchHVitemUV(hv, "level");
				tn->interval = fetchHVitemUV(hv, "interval");
				return tn;
			}
		case SYN_TIMESTAMP:
			{
				TimeStamp_T* ts;
				HV* hv;
				
				hv = getHV(inp);
				New((I32)"checkAttrVal_TimeStamp_T", ts, 1, TimeStamp_T);
				ts->wholeSeconds = fetchHVitemUV(hv, "wholeSeconds");
				ts->replicaNum = fetchHVitemUV(hv, "replicaNum");
				ts->eventID = fetchHVitemUV(hv, "eventID");
				return ts;
			}
		case SYN_PATH:
			{
				Path_T* p;
				HV* hv;
				
				hv = getHV(inp);
				New((I32)"checkAttrVal_Path_T", p, 1, Path_T);
				p->nameSpaceType = fetchHVitemUV(hv, "nameSpaceType");
				p->volumeName = fetchHVitemPV_nolen(hv, "volumeName");
				p->path = fetchHVitemPV_nolen(hv, "path");
				return p;
			}
		case SYN_EMAIL_ADDRESS:
			{
				EMail_Address_T* ea ;
				HV* hv;
				
				hv = getHV(inp);
				New((I32)"checkAttrVal_EMail_Address_T", ea, 1, EMail_Address_T);
				ea->type = fetchHVitemUV(hv, "type");
				ea->address = fetchHVitemPV_nolen(hv, "address");
				return ea;
			}
#ifdef NOTDEF			
		case SYN_PO_ADDRESS:
			{
				NWDSChar** po = $2;
				AV* av = newAV();
				int i;
				
				for (i = 0; i < 6; i++) {
					newAVitemPV(av, *po);
				}
				$result = sv_2mortal(newRV_noinc((SV*)av));
			}
			break;
		case SYN_CI_LIST:
			{
				CI_List_T* ci;
				AV* av = newAV();
				
				for (ci = $2; ci; ci = ci->next) {
					newAVitemPV(av, ci->s);
				}
				$result = sv_2mortal(newRV_noinc((SV*)av));
			}
			break;
		case SYN_OCTET_LIST:
			{
				Octet_List_T* ol;
				AV* av = newAV();
				
				for (ol = $2; ol; ol = ol->next) {
					av_push(av, newSVpvn(ol->data, ol->length));
				}
				$result = sv_2mortal(newRV_noinc((SV*)av));
			}
			break;
		case SYN_NET_ADDRESS:
			{
				Net_Address_T* na = $2;
				HV* hv = newHV();
				newHVitemUV(hv, "addressType", na->addressType);
				hv_safe_store(hv, "address", newSVpvn(na->address, na->addressLength));
				$result = sv_2mortal(newRV_noinc((SV*)hv));
			}
			break;
		case SYN_FAX_NUMBER:
			{
				Fax_Number_T* fn = $2;
				HV* hv = newHV();
				HV* hv_params = newHV();
				newHVitemPV(hv, "telephoneNumber", fn->telephoneNumber);
				newHVitemUV(hv_params, "numOfBits", fn->parameters.numOfBits);
				hv_safe_store(hv_params, "data", newSVpvn(fn->parameters.data, (fn->parameters.numOfBits + 7) >> 3));
				hv_safe_store(hv, "parameters", newRV_noinc((SV*)hv_params));
				$result = sv_2mortal(newRV_noinc((SV*)hv));
			}
			break;
		case SYN_REPLICA_POINTER:
			{
				Replica_Pointer_T* rp = $2;
				HV* hv = newHV();
				AV* av = newAV();
				UV idx;
				Net_Address_T* na;
				newHVitemPV(hv, "serverName", rp->serverName);
				newHVitemUV(hv, "replicaType", rp->replicaType);
				newHVitemUV(hv, "replicaNumber", rp->replicaNumber);
				hv_safe_store(hv, "replicaAddressHint", newRV_noinc((SV*)av));
				$result = sv_2mortal(newRV_noinc((SV*)hv));
				na = rp->replicaAddressHint;
				for (idx = rp->count; idx--; na++) {
					HV* hv2 = newHV();
					newHVitemUV(hv2, "addressType", na->addressType);
					hv_safe_store(hv2, "address", newSVpvn(na->address, na->addressLength));
					av_push(av, newRV_noinc((SV*)hv2));
				}
			}
			break;
#endif				
		default:
			*nfree = 0;
			Perl_croak(aTHX_ "Syntax #%u is not supported", synt);
			return NULL;
	}
}
%}

%typemap(perl5,in) const void * attrVal_syn3 (SV* sv, int needfree),
		   const void * attrVal_syn4 (SV* sv, int needfree),
		   const void * attrVal_syn5 (SV* sv, int needfree)
%{	needfree = 0;
	sv = $input; %}
%typemap(perl5,check) const void * attrVal_syn3
%{	$1 = checkAttrVal(sv, arg3, &needfree); %}
%typemap(perl5,check) const void * attrVal_syn4
%{	$1 = checkAttrVal(sv, arg4, &needfree); %}
%typemap(perl5,check) const void * attrVal_syn5
%{	$1 = checkAttrVal(sv, arg5, &needfree); %}
%typemap(perl5,freearg) const void * attrVal_syn3,
			const void * attrVal_syn4,
			const void * attrVal_syn5
%{	if (needfree) Safefree($1); %}

%include ncp/kernel/ncp.h
//%include ncp/kernel/ncp_fs.h
%include ncp/ncp.h
//%include ncp/ipxlib.h
%include ncp/ncplib.h
%include ncp/nwcalls.h
%include ncp/eas.h
%include ncp/ndslib.h
%include ncp/nwnet.h
//%include ncp/nwclient.h
