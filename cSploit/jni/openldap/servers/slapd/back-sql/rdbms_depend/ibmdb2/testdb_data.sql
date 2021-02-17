insert into institutes (id,name) values (1,'Example');

insert into persons (id,name,surname,password) values (1,'Mitya','Kovalev','mit');
insert into persons (id,name,surname) values (2,'Torvlobnor','Puzdoy');
insert into persons (id,name,surname) values (3,'Akakiy','Zinberstein');

insert into phones (id,phone,pers_id) values (1,'332-2334',1);
insert into phones (id,phone,pers_id) values (2,'222-3234',1);
insert into phones (id,phone,pers_id) values (3,'545-4563',2);

insert into documents (id,abstract,title) values (1,'abstract1','book1');
insert into documents (id,abstract,title) values (2,'abstract2','book2');

insert into authors_docs (pers_id,doc_id) values (1,1);
insert into authors_docs (pers_id,doc_id) values (1,2);
insert into authors_docs (pers_id,doc_id) values (2,1);

insert into referrals (id,name,url) values (1,'Referral','ldap://localhost:9012/');
