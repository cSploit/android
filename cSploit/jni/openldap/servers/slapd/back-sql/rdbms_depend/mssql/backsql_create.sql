create table ldap_oc_mappings (
	id int identity (1, 1) not null ,
	name varchar (64) not null ,
	keytbl varchar (64) not null ,
	keycol varchar (64) not null ,
	create_proc varchar (255) NULL ,
	delete_proc varchar (255) NULL,
	expect_return int not null
)
GO

alter table ldap_oc_mappings add 
	constraint pk_ldap_oc_mappings primary key  
	(
		id
	)  
GO


alter table ldap_oc_mappings add 
	constraint unq1_ldap_oc_mappings unique
	(
		name
	)  
GO


create table ldap_attr_mappings (
	id int identity (1, 1) not null ,
	oc_map_id int not null references ldap_oc_mappings(id),
	name varchar (255) not null ,
	sel_expr varchar (255) not null ,
	sel_expr_u varchar(255),
	from_tbls varchar (255) not null ,
	join_where varchar (255) NULL ,
	add_proc varchar (255) NULL ,
	delete_proc varchar (255) NULL ,
	param_order int not null,
	expect_return int not null
)
GO

alter table ldap_attr_mappings  add 
	constraint pk_ldap_attr_mappings primary key  
	(
		id
	)  
GO


create table ldap_entries (
	id int identity (1, 1) not null ,
	dn varchar (255) not null ,
	oc_map_id int not null references ldap_oc_mappings(id),
	parent int not null ,
	keyval int not null 
)
GO


alter table ldap_entries add 
	constraint pk_ldap_entries primary key  
	(
		id
	)  
GO

alter table ldap_entries add 
	constraint unq1_ldap_entries unique
	(
		oc_map_id,
		keyval
	)  
GO

alter table ldap_entries add 
	constraint unq2_ldap_entries unique
	(
		dn
	)  
GO


create table ldap_referrals
 (
	entry_id int not null references ldap_entries(id),
	url text not null
)
GO

create index entry_idx on ldap_referrals(entry_id);

create table ldap_entry_objclasses
 (
	entry_id int not null references ldap_entries(id),
	oc_name varchar(64)
 )
GO

create index entry_idx on ldap_entry_objclasses(entry_id);
