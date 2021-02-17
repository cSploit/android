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
 *  Copyright (c) 2007 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

set term !;

create or alter procedure rdb$fix_metadata
    (charset varchar(31) character set ascii)
returns
    (table_name char(31) character set unicode_fss,
     field_name char(31) character set unicode_fss,
     name1 char(31) character set unicode_fss,
     name2 char(31) character set unicode_fss)
as
    declare variable system integer;
    declare variable field1 char(31) character set unicode_fss;
    declare variable field2 char(31) character set unicode_fss;
    declare variable has_records integer;
begin
    for select rf.rdb$relation_name, rf.rdb$field_name,
               (select 1 from rdb$relation_fields
                   where rdb$relation_name = rf.rdb$relation_name and
                         rdb$field_name = 'RDB$SYSTEM_FLAG'),
               case rdb$relation_name
                   when 'RDB$CHARACTER_SETS' then 'RDB$CHARACTER_SET_NAME'
                   when 'RDB$COLLATIONS' then 'RDB$COLLATION_NAME'
                   when 'RDB$EXCEPTIONS' then 'RDB$EXCEPTION_NAME'
                   when 'RDB$FIELDS' then 'RDB$FIELD_NAME'
                   when 'RDB$FILTERS' then 'RDB$INPUT_SUB_TYPE'
                   when 'RDB$FUNCTIONS' then 'RDB$FUNCTION_NAME'
                   when 'RDB$GENERATORS' then 'RDB$GENERATOR_NAME'
                   when 'RDB$INDICES' then 'RDB$INDEX_NAME'
                   when 'RDB$PROCEDURES' then 'RDB$PROCEDURE_NAME'
                   when 'RDB$PROCEDURE_PARAMETERS' then 'RDB$PROCEDURE_NAME'
                   when 'RDB$RELATIONS' then 'RDB$RELATION_NAME'
                   when 'RDB$RELATION_FIELDS' then 'RDB$RELATION_NAME'
                   when 'RDB$ROLES' then 'RDB$ROLE_NAME'
                   when 'RDB$TRIGGERS' then 'RDB$TRIGGER_NAME'
                   else NULL
               end,
               case rdb$relation_name
                   when 'RDB$FILTERS' then 'RDB$OUTPUT_SUB_TYPE'
                   when 'RDB$PROCEDURE_PARAMETERS' then 'RDB$PARAMETER_NAME'
                   when 'RDB$RELATION_FIELDS' then 'RDB$FIELD_NAME'
                   else NULL
               end
            from rdb$relation_fields rf
            join rdb$fields f
                on (rf.rdb$field_source = f.rdb$field_name)
            where f.rdb$field_type = 261 and f.rdb$field_sub_type = 1 and
                  f.rdb$field_name <> 'RDB$SPECIFIC_ATTRIBUTES' and
                  rf.rdb$relation_name starting with 'RDB$'
            order by rf.rdb$relation_name
        into :table_name, :field_name, :system, :field1, :field2
    do
    begin
        name1 = null;
        name2 = null;

        if (field1 is null and field2 is null) then
        begin
            has_records = null;

            execute statement
                'select first 1 1 from ' || table_name ||
                '    where ' || field_name || ' is not null' ||
                iif(system = 1, ' and coalesce(rdb$system_flag, 0) in (0, 3)', '')
                into :has_records;

            if (has_records = 1) then
            begin
                suspend;

                execute statement
                    'update ' || table_name || ' set ' || field_name || ' = ' ||
                    '    cast(cast(' || field_name || ' as blob sub_type text character set none) as ' ||
                    '        blob sub_type text character set ' || charset || ') ' ||
                    iif(system = 1, 'where coalesce(rdb$system_flag, 0) in (0, 3)', '');
            end
        end
        else
        begin
            for execute statement
                    'select ' || field1 || ', ' || coalesce(field2, ' null') || ' from ' || table_name ||
                    '    where ' || field_name || ' is not null' ||
                    iif(system = 1, ' and coalesce(rdb$system_flag, 0) in (0, 3)', '')
                into :name1, :name2
            do
            begin
                suspend;

                execute statement
                    'update ' || table_name || ' set ' || field_name || ' = ' ||
                    '    cast(cast(' || field_name || ' as blob sub_type text character set none) as ' ||
                    '        blob sub_type text character set ' || charset || ') ' ||
                    '    where ' || field1 || ' = ''' || name1 || '''' ||
                    iif(name2 is null, '', ' and ' || field2 || ' = ''' || name2 || '''');
            end
        end
    end
end!

commit!

create or alter procedure rdb$check_metadata
returns
    (table_name char(31) character set unicode_fss,
     field_name char(31) character set unicode_fss,
     name1 char(31) character set unicode_fss,
     name2 char(31) character set unicode_fss)
as
begin
    for select table_name, field_name, name1, name2
            from rdb$fix_metadata ('UTF8')
        into :table_name, :field_name, :name1, :name2
    do
    begin
        suspend;
    end
end!

commit!

set term ;!
