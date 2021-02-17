/***
 *
 *  Usage example
 *  -------------
 *
 *  input 'intl.sql';
 *
 *  execute procedure sp_register_character_set ('CHARSET_NAME', 1);
 *  commit;
 *
 *  execute procedure sp_unregister_character_set ('CHARSET_NAME');
 *  commit;
 *
 */


set term !;


create or alter procedure sp_register_character_set
(
	name char(31) character set unicode_fss,
	max_bytes_per_character smallint
)
as
	declare variable id smallint;
	declare variable temp_id smallint;
begin
	name = upper(name);
	id = 255;

	for select rdb$character_set_id
			from rdb$character_sets
			order by rdb$character_set_id desc
		into :temp_id do
	begin
		if (temp_id = id) then
		begin
			id = id - 1;
			if (id = 127) then
				id = 126;
		end
		else
			break;
	end

	if (id > 0) then
	begin
		insert into rdb$types
			(rdb$field_name, rdb$type, rdb$type_name, rdb$system_flag)
			values ('RDB$CHARACTER_SET_NAME', :id, :name, 0);

		insert into rdb$character_sets
			(rdb$character_set_name, rdb$character_set_id, rdb$system_flag, rdb$bytes_per_character, rdb$default_collate_name)
			values (:name, :id, 0, :max_bytes_per_character, :name);

		insert into rdb$collations
			(rdb$collation_name, rdb$collation_id, rdb$character_set_id, rdb$system_flag)
			values (:name, 0, :id, 0);
	end
end!


create or alter procedure sp_unregister_character_set
(
	name char(31) character set unicode_fss
)
as
	declare variable id smallint;
begin
	name = upper(name);

	select rdb$character_set_id
		from rdb$character_sets
		where rdb$character_set_name = :name
		into :id;

	delete from rdb$collations
		where rdb$character_set_id = :id;

	delete from rdb$character_sets
		where rdb$character_set_id = :id;

	delete from rdb$types
		where rdb$field_name = 'RDB$CHARACTER_SET_NAME' and rdb$type_name = :name;
end!


set term ;!
commit;
