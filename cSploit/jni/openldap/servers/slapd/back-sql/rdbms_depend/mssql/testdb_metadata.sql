-- mappings


SET IDENTITY_INSERT ldap_oc_mappings ON
insert into ldap_oc_mappings (id,name,keytbl,keycol,create_proc,delete_proc,expect_return)
values (1,'inetOrgPerson','persons','id','{call create_person(?)}','{call delete_person(?)}',0)

insert into ldap_oc_mappings (id,name,keytbl,keycol,create_proc,delete_proc,expect_return)
values (2,'document','documents','id','{call create_document(?)}','{call delete_document(?)}',0)

insert into ldap_oc_mappings (id,name,keytbl,keycol,create_proc,delete_proc,expect_return)
values (3,'organization','institutes','id','{call create_org(?)}','{call delete_org(?)}',0)
SET IDENTITY_INSERT ldap_oc_mappings OFF


SET IDENTITY_INSERT ldap_attr_mappings ON
insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (1,1,'cn','persons.name+'' ''+persons.surname','persons',NULL,
	NULL,NULL,0,0)

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (2,1,'telephoneNumber','phones.phone','persons,phones',
        'phones.pers_id=persons.id','{call add_phone(?,?)}',
        '{call delete_phone(?,?)}',0,0)

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (3,1,'givenName','persons.name','persons',NULL,
	'{call set_person_name(?,?)}',NULL,0,0)

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (4,1,'sn','persons.surname','persons',NULL,
	'{call set_person_surname(?,?)}',NULL,0,0)

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (5,1,'userPassword','persons.password','persons','persons.password IS NOT NULL',
	'{call set_person_password(?,?)}','call del_person_password(?,?)',0,0)

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (6,1,'seeAlso','seeAlso.dn','ldap_entries AS seeAlso,documents,authors_docs,persons',
	'seeAlso.keyval=documents.id AND seeAlso.oc_map_id=2 AND authors_docs.doc_id=documents.id AND authors_docs.pers_id=persons.id',
	NULL,NULL,0,0);

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (7,2,'description','documents.abstract','documents',NULL,'{call set_doc_abstract(?,?)}',
	NULL,0,0);

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (8,2,'documentTitle','documents.title','documents',NULL, '{call set_doc_title(?,?)}',
        NULL,0,0)

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (9,2,'documentAuthor','documentAuthor.dn','ldap_entries AS documentAuthor,documents,authors_docs,persons',
	'documentAuthor.keyval=persons.id AND documentAuthor.oc_map_id=1 AND authors_docs.doc_id=documents.id AND authors_docs.pers_id=persons.id',
	'INSERT INTO authors_docs (pers_id,doc_id) VALUES ((SELECT ldap_entries.keyval FROM ldap_entries WHERE upper(?)=upper(ldap_entries.dn)),?)',
	'DELETE FROM authors_docs WHERE authors_docs.pers_id=(SELECT ldap_entries.keyval FROM ldap_entries WHERE upper(?)=upper(ldap_entries.dn)) AND authors_docs.doc_id=?',3,0);

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (10,2,'documentIdentifier','''document ''+text(documents.id)','documents',
	NULL,NULL,NULL,0,0);

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (11,3,'o','institutes.name','institutes',NULL,NULL,NULL,3,0);

insert into ldap_attr_mappings (id,oc_map_id,name,sel_expr,from_tbls,join_where,add_proc,delete_proc,param_order,expect_return)
values (12,3,'dc','lower(institutes.name)','institutes,ldap_entries AS dcObject,ldap_entry_objclasses AS auxObjectClass',
	'institutes.id=dcObject.keyval AND dcObject.oc_map_id=3 AND dcObject.id=auxObjectClass.entry_id AND auxObjectClass.oc_name=''dcObject''',
	'{call set_org_name(?,?)}',NULL,3,0);

SET IDENTITY_INSERT ldap_attr_mappings OFF

-- entries

SET IDENTITY_INSERT ldap_entries ON
insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (1,'dc=example,dc=com',3,0,1)

insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (2,'cn=Mitya Kovalev,dc=example,dc=com',1,1,1)

insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (3,'cn=Torvlobnor Puzdoy,dc=example,dc=com',1,1,2)

insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (4,'cn=Akakiy Zinberstein,dc=example,dc=com',1,1,3)

insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (5,'documentTitle=book1,dc=example,dc=com',2,1,1)

insert into ldap_entries (id,dn,oc_map_id,parent,keyval)
values (6,'documentTitle=book2,dc=example,dc=com',2,1,2)

SET IDENTITY_INSERT ldap_entries OFF

-- referrals
insert into ldap_entry_objclasses (entry_id,oc_name)
values (1,'dcObject');

insert into ldap_entry_objclasses (entry_id,oc_name)
values (4,'referral');

insert into ldap_referrals (entry_id,url)
values (4,'ldap://localhost:9012/');

-- support procedures

SET QUOTED_IDENTIFIER  OFF    SET ANSI_NULLS  ON 
GO


CREATE PROCEDURE create_person @@keyval int OUTPUT AS
INSERT INTO example.persons (name) VALUES ('');
set @@keyval=(SELECT MAX(id) FROM example.persons)
GO

CREATE PROCEDURE delete_person @keyval int AS
DELETE FROM example.phones WHERE pers_id=@keyval;
DELETE FROM example.authors_docs WHERE pers_id=@keyval;
DELETE FROM example.persons WHERE id=@keyval;
GO

CREATE PROCEDURE create_org @@keyval int OUTPUT AS
INSERT INTO example.institutes (name) VALUES ('');
set @@keyval=(SELECT MAX(id) FROM example.institutes)
GO

CREATE PROCEDURE delete_org @keyval int AS
DELETE FROM example.institutes WHERE id=@keyval;
GO

CREATE PROCEDURE create_document @@keyval int OUTPUT AS
INSERT INTO example.documents (title) VALUES ('');
set @@keyval=(SELECT MAX(id) FROM example.documents)
GO

CREATE PROCEDURE delete_document @keyval int AS
DELETE FROM example.authors_docs WHERE doc_id=@keyval;
DELETE FROM example.documents WHERE id=@keyval;
GO

CREATE PROCEDURE add_phone @pers_id int, @phone varchar(255) AS
INSERT INTO example.phones (pers_id,phone) VALUES (@pers_id,@phone)
GO

CREATE PROCEDURE delete_phone @keyval int,@phone varchar(64) AS
DELETE FROM example.phones WHERE pers_id=@keyval AND phone=@phone;
GO

CREATE PROCEDURE set_person_name @keyval int, @new_name varchar(255)  AS
UPDATE example.persons SET name=@new_name WHERE id=@keyval;
GO

CREATE PROCEDURE set_person_surname @keyval int, @new_surname varchar(255)  AS
UPDATE example.persons SET surname=@new_surname WHERE id=@keyval;
GO

CREATE PROCEDURE set_org_name @keyval int, @new_name varchar(255)  AS
UPDATE example.institutes SET name=@new_name WHERE id=@keyval;
GO

CREATE PROCEDURE set_doc_title @keyval int, @new_title varchar(255)  AS
UPDATE example.documents SET title=@new_title WHERE id=@keyval;
GO

CREATE PROCEDURE set_doc_abstract @keyval int, @new_abstract varchar(255)  AS
UPDATE example.documents SET abstract=@new_abstract WHERE id=@keyval;
GO

CREATE PROCEDURE make_author_link @keyval int, @author_dn varchar(255)  AS
DECLARE @per_id int;
SET @per_id=(SELECT keyval FROM example.ldap_entries 
	   WHERE oc_map_id=1 AND dn=@author_dn);
IF NOT (@per_id IS NULL)
 INSERT INTO example.authors_docs (doc_id,pers_id) VALUES (@keyval,@per_id);
GO

CREATE PROCEDURE make_doc_link @keyval int, @doc_dn varchar(255)  AS
DECLARE @doc_id int;
SET @doc_id=(SELECT keyval FROM example.ldap_entries 
	   WHERE oc_map_id=2 AND dn=@doc_dn);
IF NOT (@doc_id IS NULL)
 INSERT INTO example.authors_docs (pers_id,doc_id) VALUES (@keyval,@doc_id);
GO

CREATE PROCEDURE del_doc_link @keyval int, @doc_dn varchar(255)  AS
DECLARE @doc_id int;
SET @doc_id=(SELECT keyval FROM example.ldap_entries 
	   WHERE oc_map_id=2 AND dn=@doc_dn);
IF NOT (@doc_id IS NULL)
DELETE FROM example.authors_docs WHERE pers_id=@keyval AND doc_id=@doc_id;
GO

CREATE PROCEDURE del_author_link @keyval int, @author_dn varchar(255)  AS
DECLARE @per_id int;
SET @per_id=(SELECT keyval FROM example.ldap_entries 
	   WHERE oc_map_id=1 AND dn=@author_dn);
IF NOT (@per_id IS NULL)
 DELETE FROM example.authors_docs WHERE doc_id=@keyval AND pers_id=@per_id;
GO
