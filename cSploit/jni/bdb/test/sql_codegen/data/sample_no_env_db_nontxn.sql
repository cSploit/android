CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR2(20),
                       value NUMERIC(8,2), -- just a comment here
                       mintage_year INT(8),
                       mint_id INT(8),
                       CONSTRAINT mint_id_fk FOREIGN KEY(mint_id)
                           REFERENCES mint(mid)); --+ MODE = NONTRANSACTIONAL

CREATE INDEX unit_index ON coin(unit); 
