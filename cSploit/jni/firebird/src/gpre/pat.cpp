//____________________________________________________________
//
//		PROGRAM:	Language Preprocessor
//		MODULE:		pat.cpp
//		DESCRIPTION:	Code generator pattern generator
//
//  The contents of this file are subject to the Interbase Public
//  License Version 1.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy
//  of the License at http://www.Inprise.com/IPL.html
//
//  Software distributed under the License is distributed on an
//  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
//  or implied. See the License for the specific language governing
//  rights and limitations under the License.
//
//  The Original Code was created by Inprise Corporation
//  and its predecessors. Portions created by Inprise Corporation are
//  Copyright (C) Inprise Corporation.
//
//  All Rights Reserved.
//  Contributor(s): ______________________________________.
//
//
//____________________________________________________________
//
//

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../gpre/gpre.h"
#include "../gpre/pat.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/pat_proto.h"
#include "../gpre/lang_proto.h"


enum pat_t {
	NL,
	RH, RL, RT, RI, RS,			// Request handle, level, transaction, ident, length
	DH, DF,						// Database handle, filename
	TH,							// Transaction handle
	BH, BI,						// Blob handle, blob_ident
	FH,							// Form handle
	V1, V2,						// Status vectors
	I1, I2,						// Identifier numbers
	RF, RE,						// OS- and language-dependent REF and REF-end character
	VF, VE,						// OS- and language-dependent VAL and VAL-end character
	S1, S2, S3, S4, S5, S6, S7,
	// Arbitrary strings
	N1, N2, N3, N4,				// Arbitrary number (SSHORT)
	L1, L2,						// Arbitrary number (SLONG)
	PN, PL, PI,					// Port number, port length, port ident
	QN, QL, QI,					// Second port number, port length, port ident
	IF, EL, EN,					// If, else, end
	FR							// Field reference
};

static const struct ops
{
	pat_t ops_type;
	TEXT ops_string[3];
} operators[] =
	{
		{ RH, "RH" },
		{ RL, "RL" },
		{ RT, "RT" },
		{ RI, "RI" },
		{ RS, "RS" },
		{ DH, "DH" },
		{ DF, "DF" },
		{ TH, "TH" },
		{ BH, "BH" },
		{ BI, "BI" },
		{ FH, "FH" },
		{ V1, "V1" },
		{ V2, "V2" },
		{ I1, "I1" },
		{ I2, "I2" },
		{ S1, "S1" },
		{ S2, "S2" },
		{ S3, "S3" },
		{ S4, "S4" },
		{ S5, "S5" },
		{ S6, "S6" },
		{ S7, "S7" },
		{ N1, "N1" },
		{ N2, "N2" },
		{ N3, "N3" },
		{ N4, "N4" },
		{ L1, "L1" },
		{ L2, "L2" },
		{ PN, "PN" },
		{ PL, "PL" },
		{ PI, "PI" },
		{ QN, "QN" },
		{ QL, "QL" },
		{ QI, "QI" },
		{ IF, "IF" },
		{ EL, "EL" },
		{ EN, "EN" },
		{ RF, "RF" },
		{ RE, "RE" },
		{ VF, "VF" },
		{ VE, "VE" },
		{ FR, "FR" },
		{ NL, "" }
	};


//____________________________________________________________
//
//		Expand a pattern.
//

void PATTERN_expand( USHORT column, const TEXT* pattern, PAT* args)
{

	// CVC: kudos to the programmer that had chosen variables with the same
	// names that structs in gpre.h and with type char* if not enough.
	// Renamed ref to lang_ref and val to lang_val.
	const TEXT* lang_ref = "";
	const TEXT* refend = "";
	const TEXT* lang_val = "";
	const TEXT* valend = "";

	if ((gpreGlob.sw_language == lang_c) || (isLangCpp(gpreGlob.sw_language)))
	{
		lang_ref = "&";
		refend = "";
	}
	else if (gpreGlob.sw_language == lang_pascal)
	{
		lang_ref = "%%REF ";
		refend = "";
	}
	else if (gpreGlob.sw_language == lang_cobol)
	{
		lang_ref = "BY REFERENCE ";
		refend = "";
		lang_val = "BY VALUE ";
		valend = "";
	}
	else if (gpreGlob.sw_language == lang_fortran)
	{
#if (defined AIX || defined AIX_PPC)
		lang_ref = "%REF(";
		refend = ")";
		lang_val = "%VAL(";
		valend = ")";
#endif
	}

	TEXT buffer[512];
	TEXT* p = buffer;
	*p++ = '\n';
	bool sw_gen = true;
	for (USHORT n = column; n; --n)
		*p++ = ' ';

	SSHORT value;				// value needs to be signed since some of the
								// values printed out are signed.
	SLONG long_value;
	TEXT c;
	while (c = *pattern++)
	{
		if (c != '%')
		{
			if (sw_gen)
			{
				*p++ = c;
				if ((c == '\n') && (*pattern))
					for (USHORT n = column; n; --n)
						*p++ = ' ';
			}
			continue;
		}
		bool sw_ident = false;
		const TEXT* string = NULL;
		const ref* reference = NULL;
		bool handle_flag = false, long_flag = false;
		const ops* oper_iter;
		for (oper_iter = operators; oper_iter->ops_type != NL; oper_iter++)
		{
			if (oper_iter->ops_string[0] == pattern[0] && oper_iter->ops_string[1] == pattern[1])
			{
				break;
			}
		}
		pattern += 2;

		switch (oper_iter->ops_type)
		{
		case IF:
			sw_gen = args->pat_condition;
			continue;

		case EL:
			sw_gen = !sw_gen;
			continue;

		case EN:
			sw_gen = true;
			continue;

		case RH:
			handle_flag = true;
			string = args->pat_request->req_handle;
			break;

		case RL:
			string = args->pat_request->req_request_level;
			break;

		case RS:
			value = args->pat_request->req_length;
			break;

		case RT:
			handle_flag = true;
			string = args->pat_request->req_trans;
			break;

		case RI:
			long_value = args->pat_request->req_ident;
			long_flag = true;
			sw_ident = true;
			break;

		case DH:
			handle_flag = true;
			string = args->pat_database->dbb_name->sym_string;
			break;

		case DF:
			string = args->pat_database->dbb_filename;
			break;

		case PN:
			value = args->pat_port->por_msg_number;
			break;

		case PL:
			value = args->pat_port->por_length;
			break;

		case PI:
			long_value = args->pat_port->por_ident;
			long_flag = true;
			sw_ident = true;
			break;

		case QN:
			value = args->pat_port2->por_msg_number;
			break;

		case QL:
			value = args->pat_port2->por_length;
			break;

		case QI:
			long_value = args->pat_port2->por_ident;
			long_flag = true;
			sw_ident = true;
			break;

		case BH:
			long_value = args->pat_blob->blb_ident;
			long_flag = true;
			sw_ident = true;
			break;

		case I1:
			long_value = args->pat_ident1;
			long_flag = true;
			sw_ident = true;
			break;

		case I2:
			long_value = args->pat_ident2;
			long_flag = true;
			sw_ident = true;
			break;

		case S1:
			string = args->pat_string1;
			break;

		case S2:
			string = args->pat_string2;
			break;

		case S3:
			string = args->pat_string3;
			break;

		case S4:
			string = args->pat_string4;
			break;

		case S5:
			string = args->pat_string5;
			break;

		case S6:
			string = args->pat_string6;
			break;

		case S7:
			string = args->pat_string7;
			break;

		case V1:
			string = args->pat_vector1;
			break;

		case V2:
			string = args->pat_vector2;
			break;

		case N1:
			value = args->pat_value1;
			break;

		case N2:
			value = args->pat_value2;
			break;

		case N3:
			value = args->pat_value3;
			break;

		case N4:
			value = args->pat_value4;
			break;

		case L1:
			long_value = args->pat_long1;
			long_flag = true;
			break;

		case L2:
			long_value = args->pat_long2;
			long_flag = true;
			break;

		case RF:
			string = lang_ref;
			break;

		case RE:
			string = refend;
			break;

		case VF:
			string = lang_val;
			break;

		case VE:
			string = valend;
			break;

		case FR:
			reference = args->pat_reference;
			break;

		default:
			sprintf(buffer, "Unknown substitution \"%c%c\"", pattern[-2], pattern[-1]);
			CPR_error(buffer);
			continue;
		}

		if (!sw_gen)
			continue;

		if (string)
		{
#ifdef GPRE_ADA
			if (handle_flag && (gpreGlob.sw_language == lang_ada))
			{
				for (const TEXT* q = gpreGlob.ada_package; *q;)
					*p++ = *q++;
			}
#endif
			while (*string)
				*p++ = *string++;
			continue;
		}

		if (sw_ident)
		{
			if (long_flag)
				sprintf(p, gpreGlob.long_ident_pattern, long_value);
			else
				sprintf(p, gpreGlob.ident_pattern, value);
		}
		else if (reference)
		{
			if (!reference->ref_port)
				sprintf(p, gpreGlob.ident_pattern, reference->ref_ident);
			else
			{
				TEXT temp1[16], temp2[16];
				sprintf(temp1, gpreGlob.ident_pattern, reference->ref_port->por_ident);
				sprintf(temp2, gpreGlob.ident_pattern, reference->ref_ident);
				switch (gpreGlob.sw_language)
				{
				case lang_fortran:
				case lang_cobol:
					strcpy(p, temp2);
					break;

				default:
					sprintf(p, "%s.%s", temp1, temp2);
				}
			}
		}
		else if (long_flag) {
			sprintf(p, "%"SLONGFORMAT, long_value);
		}
		else {
			sprintf(p, "%d", value);
		}

		while (*p)
			p++;
	}

	*p = 0;

#if (defined GPRE_ADA || defined GPRE_COBOL || defined GPRE_FORTRAN)
	switch (gpreGlob.sw_language)
	{
#ifdef GPRE_ADA
		// Ada lines can be up to 120 characters long.  ADA_print_buffer
		// handles this problem and ensures that GPRE output is <=120 characters.
	case lang_ada:
		ADA_print_buffer(buffer, 0);
		break;
#endif

#ifdef GPRE_COBOL
		// COBOL lines can be up to 72 characters long.  COB_print_buffer
		// handles this problem and ensures that GPRE output is <= 72 characters.
	case lang_cobol:
		COB_print_buffer(buffer, true);
		break;
#endif

#ifdef GPRE_FORTRAN
		// In FORTRAN, characters beyond column 72 are ignored.  FTN_print_buffer
		// handles this problem and ensures that GPRE output does not exceed this
		// limit.
	case lang_fortran:
		FTN_print_buffer(buffer);
		break;
#endif

	default:
		fprintf(gpreGlob.out_file, "%s", buffer);
		break;
	}
#else
	fprintf(gpreGlob.out_file, "%s", buffer);
#endif

}

