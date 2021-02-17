
CREATE TABLE authors_docs (
	pers_id int NOT NULL ,
	doc_id int NOT NULL 
)
GO

CREATE TABLE documents (
	id int IDENTITY (1, 1) NOT NULL ,
	abstract varchar (255) NULL ,
	title varchar (255) NULL ,
	body binary (255) NULL 
)
GO

CREATE TABLE institutes (
	id int IDENTITY (1, 1) NOT NULL ,
	name varchar (255) NOT NULL 
)
GO


CREATE TABLE persons (
	id int IDENTITY (1, 1) NOT NULL ,
	name varchar (255) NULL ,
	surname varchar (255) NULL ,
	password varchar (64) NULL
)
GO

CREATE TABLE phones (
	id int IDENTITY (1, 1) NOT NULL ,
	phone varchar (255) NOT NULL ,
	pers_id int NOT NULL 
)
GO

ALTER TABLE authors_docs WITH NOCHECK ADD 
	CONSTRAINT PK_authors_docs PRIMARY KEY  
	(
		pers_id,
		doc_id
	)  
GO

ALTER TABLE documents WITH NOCHECK ADD 
	CONSTRAINT PK_documents PRIMARY KEY  
	(
		id
	)  
GO

ALTER TABLE institutes WITH NOCHECK ADD 
	CONSTRAINT PK_institutes PRIMARY KEY  
	(
		id
	)  
GO


ALTER TABLE persons WITH NOCHECK ADD 
	CONSTRAINT PK_persons PRIMARY KEY  
	(
		id
	)  
GO

ALTER TABLE phones WITH NOCHECK ADD 
	CONSTRAINT PK_phones PRIMARY KEY  
	(
		id
	)  
GO

