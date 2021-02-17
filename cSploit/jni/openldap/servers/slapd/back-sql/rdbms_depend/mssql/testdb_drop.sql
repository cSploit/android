drop procedure create_person
drop procedure set_person_name
drop procedure delete_phone
drop procedure add_phone
drop procedure make_doc_link
drop procedure del_doc_link
drop procedure delete_person

drop procedure create_org
drop procedure set_org_name
drop procedure delete_org

drop procedure create_document
drop procedure set_doc_title
drop procedure set_doc_abstract
drop procedure make_author_link
drop procedure del_author_link
drop procedure delete_document

if exists (select * from sysobjects where id = object_id(N'authors_docs') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table authors_docs
GO

if exists (select * from sysobjects where id = object_id(N'documents') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table documents
GO

if exists (select * from sysobjects where id = object_id(N'institutes') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table institutes
GO

if exists (select * from sysobjects where id = object_id(N'persons') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table persons
GO

if exists (select * from sysobjects where id = object_id(N'phones') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table phones
GO

