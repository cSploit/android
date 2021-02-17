CREATE TABLE persons (
	id NUMBER NOT NULL,
	name varchar2(255) NOT NULL,
	surname varchar2(255) NOT NULL,
	password varchar2(64) NOT NULL
);

CREATE TABLE institutes (
	id NUMBER NOT NULL,
	name varchar2(255)
);

CREATE TABLE documents (
	id NUMBER NOT NULL,
	title varchar2(255) NOT NULL,
	abstract varchar2(255)
);

CREATE TABLE authors_docs (
	pers_id NUMBER NOT NULL,
	doc_id NUMBER NOT NULL
);

CREATE TABLE phones (
	id NUMBER NOT NULL ,
	phone varchar2(255) NOT NULL ,
	pers_id NUMBER NOT NULL 
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

CREATE SEQUENCE person_ids START WITH 1 INCREMENT BY 1;

CREATE SEQUENCE document_ids START WITH 1 INCREMENT BY 1;

CREATE SEQUENCE institute_ids START WITH 1 INCREMENT BY 1;

CREATE SEQUENCE phone_ids START WITH 1 INCREMENT BY 1;
