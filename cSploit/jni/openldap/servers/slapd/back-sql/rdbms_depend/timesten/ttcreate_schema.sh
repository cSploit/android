ttIsql -connStr "DSN=ldap_tt;Overwrite=1" -f backsql_create.sql
ttIsql -connStr "DSN=ldap_tt" -f tttestdb_create.sql
ttIsql -connStr "DSN=ldap_tt" -f tttestdb_data.sql
ttIsql -connStr "DSN=ldap_tt" -f tttestdb_metadata.sql
