CREATE TABLE persons (
	id int NOT NULL primary key,
	name varchar(255) NOT NULL,
        name_u varchar(255),
 	title varchar(255),
	title_U varchar(255),
        organization varchar(255)
) 
unique hash on (id) pages=100;
create index personsx1 on persons(title_U);
create index personsx2 on persons(name_u);

CREATE TABLE institutes (
	id int NOT NULL primary key,
	name varchar(255)
)
unique hash on (id) pages=100;

CREATE TABLE documents (
	id int NOT NULL primary key,
	title varchar(255) NOT NULL,
	abstract varchar(255)
)
unique hash on (id) pages=100;

CREATE TABLE authors_docs (
	pers_id int NOT NULL,
	doc_id int NOT NULL,
	PRIMARY KEY  
	(
		pers_id,
		doc_id
	)
) unique hash on (pers_id, doc_id) pages=100;

CREATE TABLE phones (
	id int NOT NULL primary key,
	phone varchar(255) NOT NULL ,
	pers_id int NOT NULL 
)
unique hash on (id) pages=100;

