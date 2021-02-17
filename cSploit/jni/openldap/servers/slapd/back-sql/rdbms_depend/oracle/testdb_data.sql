insert into institutes (id,name) values (institute_ids.nextval,'example');

insert into persons (id,name,surname,password) values (person_ids.nextval,'Mitya','Kovalev','mit');

insert into persons (id,name,surname) values (person_ids.nextval,'Torvlobnor','Puzdoy');

insert into persons (id,name,surname) values (person_ids.nextval,'Akakiy','Zinberstein');


insert into phones (id,phone,pers_id) values (phone_ids.nextval,'332-2334',1);

insert into phones (id,phone,pers_id) values (phone_ids.nextval,'222-3234',1);

insert into phones (id,phone,pers_id) values (phone_ids.nextval,'545-4563',2);


insert into documents (id,abstract,title) values (document_ids.nextval,'abstract1','book1');

insert into documents (id,abstract,title) values (document_ids.nextval,'abstract2','book2');


insert into authors_docs (pers_id,doc_id) values (1,1);

insert into authors_docs (pers_id,doc_id) values (1,2);

insert into authors_docs (pers_id,doc_id) values (2,1);

