CREATE DATABASE numismatics1; /*+ MODE = TRANSACTIONAL */ 

CREATE TABLE coin1 (cid INT(8) PRIMARY KEY,
                       unit VARCHAR2(20),
                       value NUMERIC(8,2), 
                       mintage_year INT(8),
                       mint_id INT(8),
                       CONSTRAINT mint_id_fk FOREIGN KEY(mint_id)
                           REFERENCES mint(mid)); --+ MODE = TRANSACTIONAL

CREATE TABLE mint1 (mid INT(8) PRIMARY KEY,
       country VARCHAR2(20),
       city VARCHAR2(20));   --+ MODE = NONTRANSACTIONAL

CREATE INDEX coin_index1 ON coin1(unit); 

CREATE TABLE random1 (rid VARCHAR2(20) PRIMARY KEY,
       chunk bin(127));     /*+ MODE = NONTRANSACTIONAL */
       
CREATE DATABASE numismatics2; /*+ MODE = TRANSACTIONAL */ 