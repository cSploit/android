
insert into ldap_oc_mappings 
(id,name,          keytbl,   keycol, create_proc,
delete_proc,expect_return)
values 
(1,'inetOrgPerson','persons','id',   'insert into persons (name) values ('');\n select last_insert_id();',
NULL,0);

insert into ldap_oc_mappings 
(id, name,      keytbl,     keycol,create_proc,delete_proc,expect_return)
values 
(2,  'document','documents','id',  NULL,       NULL,       0);

insert into ldap_oc_mappings 
(id,name,         keytbl,      keycol,create_proc,delete_proc,expect_return)
values 
(3,'organization','institutes','id',  NULL,       NULL,       0);


insert into ldap_attr_mappings 
(id, oc_map_id, name,  sel_expr,       sel_expr_u,      from_tbls,
join_where,add_proc, delete_proc,param_order,expect_return)
values 
(1,  1,         'cn',  'persons.name', 'persons.name_u','persons',
NULL,      NULL,     NULL,       3,          0);

insert into ldap_attr_mappings 
(id, oc_map_id, name,     sel_expr,        sel_expr_u, from_tbls,join_where,
add_proc, delete_proc,param_order,expect_return)
values 
(10, 1,         'title',  'persons.title', 'persons.title_u', 'persons',NULL,      NULL,     
NULL,       3,          0);

insert into ldap_attr_mappings 
(id, oc_map_id,name,             sel_expr,      from_tbls,
join_where,                  add_proc,delete_proc,param_order,expect_return)
values 
(2,  1,        'telephoneNumber','phones.phone','persons,phones',
'phones.pers_id=persons.id', NULL,    NULL,       3,          0);

insert into ldap_attr_mappings 
(id,oc_map_id, name, sel_expr,      from_tbls, join_where,add_proc,
delete_proc,param_order,expect_return)
values 
(3, 1,         'sn', 'persons.name','persons', NULL,      NULL, 
NULL,       3,          0);

insert into ldap_attr_mappings 
(id, oc_map_id, name, sel_expr,              from_tbls, join_where,add_proc,
delete_proc,param_order,expect_return)
values 
(30, 1,         'ou', 'persons.organization','persons', NULL,      NULL, 
NULL,       3,          0);

insert into ldap_attr_mappings 
(id, oc_map_id, name,          sel_expr,            from_tbls,  join_where,
add_proc,delete_proc,param_order,expect_return)
values 
(4,  2,         'description', 'documents.abstract','documents', NULL,
NULL,    NULL,       3,          0);

insert into ldap_attr_mappings 
(id, oc_map_id, name,           sel_expr,         from_tbls,  join_where,
add_proc,delete_proc,param_order,expect_return)
values 
(5,  2,         'documentTitle','documents.title','documents',NULL,
NULL,    NULL,       3,          0);

-- insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
-- values (6,2,'documentAuthor','persons.name','persons,documents,authors_docs',
--         'persons.id=authors_docs.pers_id AND documents.id=authors_docs.doc_id',
-- 	NULL,NULL,3,0);

insert into ldap_attr_mappings 
(id, oc_map_id, name, sel_expr,          from_tbls,    join_where,add_proc,
delete_proc,param_order,expect_return)
values 
(7,  3,         'o',  'institutes.name', 'institutes', NULL,      NULL,    
NULL,       3,          0);

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (8,1,'documentDN','ldap_entries.dn','ldap_entries,documents,authors_docs,persons',
        'ldap_entries.keyval=documents.id AND ldap_entries.oc_map_id=2 AND authors_docs.doc_id=documents.id AND authors_docs.pers_id=persons.id',
	NULL,NULL,3,0);

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (9,2,'documentAuthor','ldap_entries.dn','ldap_entries,documents,authors_docs,persons',
        'ldap_entries.keyval=persons.id AND ldap_entries.oc_map_id=1 AND authors_docs.doc_id=documents.id AND authors_docs.pers_id=persons.id',
	NULL,NULL,3,0);
	
-- entries
	
insert into ldap_entries 
(id, dn,           oc_map_id, parent, keyval)
values 
(1,  'o=sql,c=RU', 3,         0,      1);

insert into ldap_entries 
(id, dn,                            oc_map_id, parent, keyval)
values 
(2,  'cn=Mitya Kovalev,o=sql,c=RU', 1,         1,      1);

insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (3,'cn=Torvlobnor Puzdoy,o=sql,c=RU',1,1,2);

insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (4,'cn=Akakiy Zinberstein,o=sql,c=RU',1,1,3);

insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (5,'documentTitle=book1,o=sql,c=RU',2,1,1);

insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (6,'documentTitle=book2,o=sql,c=RU',2,1,2);
	
	
-- referrals

insert into ldap_entry_objclasses (entry_id,oc_name)
values (4,'referral');

insert into ldap_referrals (entry_id,url)
values (4,'http://localhost');
