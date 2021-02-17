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

/*
 * This example demonstrate packages creating and using FB$OUT, a package that write and read
 * messages in a session and is suitable for debug PSQL code.
 */

set names utf8;

create database 'test.fdb';

input 'fbout-header.sql';
input 'fbout-body.sql';

set term !;

create procedure test
as
	declare i integer;
begin
	execute procedure fb$out.put_line ('um');
	execute procedure fb$out.put_line ('dois');
	execute procedure fb$out.enable;
	execute procedure fb$out.put_line ('três');
	execute procedure fb$out.put_line ('quatro');
	execute procedure fb$out.disable;
	execute procedure fb$out.put_line ('cinco');
	execute procedure fb$out.enable;
	execute procedure fb$out.put_line ('seis');
	begin
		i = 'a';
		execute procedure fb$out.put_line ('sete');
	when any do
		begin
		end
	end
end!

set term ;!

grant execute on procedure test to public;


execute procedure test;

execute procedure fb$out.get_lines;
execute procedure fb$out.get_lines;
