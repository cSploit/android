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

set term !;


recreate package body fb$out
as
begin
	procedure enable
	as
	begin
		rdb$set_context('USER_SESSION', 'fb$out.enabled', '1');
	end

	procedure disable
	as
	begin
		rdb$set_context('USER_SESSION', 'fb$out.enabled', null);
	end

	procedure put_line (line fb$out_type)
	as
	begin
		if (rdb$get_context('USER_SESSION', 'fb$out.enabled') = '1') then
		begin
			in autonomous transaction do
			begin
				insert into fb$out_table (line_num, content)
					values (next value for fb$out_seq, :line);
			end
		end
	end

	procedure clear
	as
	begin
		in autonomous transaction do
			delete from fb$out_table;
	end

	procedure get_lines returns (lines fb$out_type)
	as
		declare line fb$out_type;
	begin
		lines = '';

		in autonomous transaction do
		begin
			for select content from fb$out_table order by line_num into line
			do
			begin
				if (octet_length(lines) > 0) then
					lines = lines || ascii_char(13) || ascii_char(10);

				lines = lines || :line;
			end
		end

		execute procedure clear;
	end
end!


set term ;!
