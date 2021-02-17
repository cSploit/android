drop table if exists ldap_oc_mappings;
create table ldap_oc_mappings
 (
	id integer unsigned not null primary key auto_increment,
	name varchar(64) not null,
	keytbl varchar(64) not null,
	keycol varchar(64) not null,
	create_proc varchar(255),
	delete_proc varchar(255),
	expect_return tinyint not null
);

drop table if exists ldap_attr_mappings;
create table ldap_attr_mappings
 (
	id integer unsigned not null primary key auto_increment,
	oc_map_id integer unsigned not null references ldap_oc_mappings(id),
	name varchar(255) not null,
	sel_expr varchar(255) not null,
	sel_expr_u varchar(255),
	from_tbls varchar(255) not null,
	join_where varchar(255),
	add_proc varchar(255),
	delete_proc varchar(255),
	param_order tinyint not null,
	expect_return tinyint not null
);

drop table if exists ldap_entries;
create table ldap_entries
 (
	id integer unsigned not null primary key auto_increment,
	dn varchar(255) not null,
	oc_map_id integer unsigned not null references ldap_oc_mappings(id),
	parent int NOT NULL ,
	keyval int NOT NULL 
);

alter table ldap_entries add 
	constraint unq1_ldap_entries unique
	(
		oc_map_id,
		keyval
	);  

alter table ldap_entries add
	constraint unq2_ldap_entries unique
	(
		dn
	);  

drop table if exists ldap_entry_objclasses;
create table ldap_entry_objclasses
 (
	entry_id integer not null references ldap_entries(id),
	oc_name varchar(64)
 );

