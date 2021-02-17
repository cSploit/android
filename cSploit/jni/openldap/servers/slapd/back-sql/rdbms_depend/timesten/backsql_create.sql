
create table ldap_oc_mappings
 (
	id integer not null primary key,
	name varchar(64) not null,
	keytbl varchar(64) not null,
	keycol varchar(64) not null,
	create_proc varchar(255),
	delete_proc varchar(255),
	expect_return tinyint not null
);

create table ldap_attr_mappings
 (
	id integer not null primary key,
	oc_map_id integer not null,
	name varchar(255) not null,
	sel_expr varchar(255) not null,
	sel_expr_u varchar(255),
	from_tbls varchar(255) not null,
	join_where varchar(255),
	add_proc varchar(255),
	delete_proc varchar(255),
	param_order tinyint not null,
	expect_return tinyint not null,
	foreign key (oc_map_id) references ldap_oc_mappings(id)
);

create table ldap_entries
 (
	id integer not null primary key,
	dn varchar(255) not null,
        dn_ru varchar(255),
	oc_map_id integer not null,
	parent int NOT NULL ,
	keyval int NOT NULL,
	foreign key (oc_map_id) references ldap_oc_mappings(id)
);

create index ldap_entriesx1 on ldap_entries(dn_ru);

create unique index unq1_ldap_entries on ldap_entries
	(
		oc_map_id,
		keyval
	);  

create unique index unq2_ldap_entries on ldap_entries
	(
		dn
	);  

create table ldap_referrals
 (
	entry_id integer not null,
	url varchar(4096) not null,
	foreign key (entry_id) references ldap_entries(id)
);

create table ldap_entry_objclasses
 (
	entry_id integer not null,
	oc_name varchar(64),
	foreign key (entry_id) references ldap_entries(id)
 );

