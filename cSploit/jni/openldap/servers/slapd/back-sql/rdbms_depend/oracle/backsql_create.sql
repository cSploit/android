create table ldap_oc_mappings (
	id number not null ,
	name varchar2(64) not null ,
	keytbl varchar2(64) not null ,
	keycol varchar2(64) not null ,
	create_proc varchar2(255),
	delete_proc varchar2(255),
	expect_return number not null
);

alter table ldap_oc_mappings add 
	constraint PK_ldap_oc_mappings primary key
	(
		id
	); 

alter table ldap_oc_mappings add 
	constraint unq_ldap_oc_mappings unique
	(
		name
	);  

create table ldap_attr_mappings (
	id number not null,
	oc_map_id number not null references ldap_oc_mappings(id),
	name varchar2(255) not null,
	sel_expr varchar2(255) not null,
	sel_expr_u varchar2(255),
	from_tbls varchar2(255) not null,
	join_where varchar2(255),
	add_proc varchar2(255),
	delete_proc varchar2(255),
	param_order number not null,
	expect_return number not null
);

alter table ldap_attr_mappings add 
	constraint pk_ldap_attr_mappings primary key
	(
		id
	); 


create table ldap_entries (
	id number not null ,
	dn varchar2(255) not null ,
	dn_ru varchar2(255),
	oc_map_id number not null references ldap_oc_mappings(id),
	parent number not null ,
	keyval number not null 
);

alter table ldap_entries add 
	constraint PK_ldap_entries primary key
	(
		id
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

create sequence ldap_objclass_ids start with 1 increment by 1;

create sequence ldap_attr_ids start with 1 increment by 1;

create sequence ldap_entry_ids start with 1 increment by 1;

create table ldap_referrals
 (
	entry_id number not null references ldap_entries(id),
	url varchar(1023) not null
);

create table ldap_entry_objclasses
 (
	entry_id number not null references ldap_entries(id),
	oc_name varchar(64)
 );

quit
