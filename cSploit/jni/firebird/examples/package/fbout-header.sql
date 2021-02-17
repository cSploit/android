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
 *  Copyright (c) 2009 Adriano dos Santos Fernandes <adrianosf@gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

create sequence fb$out_seq;

create domain fb$out_type as blob sub_type text character set utf8 not null;

create global temporary table fb$out_table (
	line_num int,  
	content fb$out_type
) on commit preserve rows;


set term !;


create or alter package fb$out
as
begin
	procedure enable;
	procedure disable;

	procedure put_line (line fb$out_type);
	procedure clear;

	procedure get_lines returns (lines fb$out_type);
end!


set term ;!


grant all on table fb$out_table to package fb$out;

grant execute on package fb$out to public;
