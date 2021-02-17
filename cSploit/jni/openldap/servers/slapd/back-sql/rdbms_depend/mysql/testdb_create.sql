drop table if exists persons;
CREATE TABLE persons (
	id int NOT NULL,
	name varchar(255) NOT NULL,
	surname varchar(255) NOT NULL,
	password varchar(64)
);

drop table if exists institutes;
CREATE TABLE institutes (
	id int NOT NULL,
	name varchar(255)
);

drop table if exists documents;
CREATE TABLE documents (
	id int NOT NULL,
	title varchar(255) NOT NULL,
	abstract varchar(255)
);

drop table if exists authors_docs;
CREATE TABLE authors_docs (
	pers_id int NOT NULL,
	doc_id int NOT NULL
);

drop table if exists phones;
CREATE TABLE phones (
	id int NOT NULL ,
	phone varchar(255) NOT NULL ,
	pers_id int NOT NULL 
);

drop table if exists certs;
CREATE TABLE certs (
	id int NOT NULL ,
	cert LONGBLOB NOT NULL,
	pers_id int NOT NULL 
);

ALTER TABLE authors_docs  ADD 
	CONSTRAINT PK_authors_docs PRIMARY KEY  
	(
		pers_id,
		doc_id
	);

ALTER TABLE documents  ADD 
	CONSTRAINT PK_documents PRIMARY KEY  
	(
		id
	); 

ALTER TABLE institutes  ADD 
	CONSTRAINT PK_institutes PRIMARY KEY  
	(
		id
	);  


ALTER TABLE persons  ADD 
	CONSTRAINT PK_persons PRIMARY KEY  
	(
		id
	); 

ALTER TABLE phones  ADD 
	CONSTRAINT PK_phones PRIMARY KEY  
	(
		id
	); 

ALTER TABLE certs  ADD 
	CONSTRAINT PK_certs PRIMARY KEY  
	(
		id
	); 

drop table if exists referrals;
CREATE TABLE referrals (
	id int NOT NULL,
	name varchar(255) NOT NULL,
	url varchar(255) NOT NULL
);

