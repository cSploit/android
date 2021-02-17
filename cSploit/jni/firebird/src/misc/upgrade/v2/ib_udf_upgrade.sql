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
 *  The Original Code was created by Claudio Valderrama on 20-Sept-2004
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

-- Warning: this script should be used only when you are using the new ib_udf
-- library together with Firebird v2 and want to detect NULL correctly.

update rdb$function_arguments
set rdb$mechanism = 5
where rdb$function_name = 'ASCII_CHAR'
and rdb$argument_position = 1;

update rdb$function_arguments
set rdb$mechanism = 5
where rdb$function_name = 'LOWER'
and rdb$argument_position = 1;

update rdb$function_arguments
set rdb$mechanism = 5
where rdb$function_name = 'LPAD'
and rdb$argument_position in (1, 3);

update rdb$function_arguments
set rdb$mechanism = 5
where rdb$function_name = 'LTRIM'
and rdb$argument_position = 1;

update rdb$function_arguments
set rdb$mechanism = 5
where rdb$function_name = 'RPAD'
and rdb$argument_position in (1, 3);

update rdb$function_arguments
set rdb$mechanism = 5
where rdb$function_name = 'RTRIM'
and rdb$argument_position = 1;

update rdb$function_arguments
set rdb$mechanism = 5
where rdb$function_name = 'SUBSTR'
and rdb$argument_position = 1;

update rdb$function_arguments
set rdb$mechanism = 5
where rdb$function_name = 'SUBSTRLEN'
and rdb$argument_position = 1;

