/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		functions.h
 *	DESCRIPTION:	Definition of built-in functions
 *
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov and all contributors
 *  signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

// Usage:
// FUNCTION(<routine>, "<function_name>", "<module_name>", "<entrypoint>", <return_argument>)
// FUNCTION_ARGUMENT(<mechanism>, <type>, <scale>, <length>, <sub_type>, <charset>, <precision>, <char_length>)

// Uncomment this to have the two example functions registered automatically in system tables.
//#define DECLARE_EXAMPLE_IUDF_AUTOMATICALLY
// CVC: Starting demonstration code to register IUDF automatically.
#ifdef DECLARE_EXAMPLE_IUDF_AUTOMATICALLY
FUNCTION(byteLen, "SYS_BYTE_LEN", "test_module", "byte_len", 0)
  FUNCTION_ARGUMENT(-FUN_reference, (int) blr_long, 0, 4, 0, 0, 0, 0)
  FUNCTION_ARGUMENT(FUN_descriptor, (int) blr_long, 0, 4, 0, 0, 0, 0)
END_FUNCTION

FUNCTION(test, "TEST", "test_module", "test_function", 2)
  FUNCTION_ARGUMENT(FUN_ref_with_null, (int) blr_long, 0, 4, 0, 0, 0, 0)
  FUNCTION_ARGUMENT(FUN_reference, (int) blr_text, 0, 20, 0, 0, 0, 20)
#endif
// CVC: Finishing demonstration code to register IUDF automatically.


FUNCTION(get_context, "RDB$GET_CONTEXT", "system_module", "get_context", 0)
  // Result, variable value
  FUNCTION_ARGUMENT(-FUN_reference, blr_varying, 0, 255, 0, 0, 0, 255)
  // Namespace
  FUNCTION_ARGUMENT(FUN_ref_with_null, blr_varying, 0, 80, 0, 0, 0, 80)
  // Variable name
  FUNCTION_ARGUMENT(FUN_ref_with_null, blr_varying, 0, 80, 0, 0, 0, 80)
END_FUNCTION

FUNCTION(set_context, "RDB$SET_CONTEXT", "system_module", "set_context", 0)
  // Result
  FUNCTION_ARGUMENT(FUN_value, blr_long, 0, 4, 0, 0, 0, 0)
  // Namespace
  FUNCTION_ARGUMENT(FUN_ref_with_null, blr_varying, 0, 80, 0, 0, 0, 80)
  // Variable name
  FUNCTION_ARGUMENT(FUN_ref_with_null, blr_varying, 0, 80, 0, 0, 0, 80)
  // Variable, value
  FUNCTION_ARGUMENT(FUN_ref_with_null, blr_varying, 0, 255, 0, 0, 0, 255)
END_FUNCTION

