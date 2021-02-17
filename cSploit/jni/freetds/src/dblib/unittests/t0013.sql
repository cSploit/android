if object_id('freetds_dblib_t0013') is not null drop table freetds_dblib_t0013
go
create table freetds_dblib_t0013 (i int not null, PigTure image not null)
go
insert into freetds_dblib_t0013 values (1, '')
go
SELECT PigTure FROM freetds_dblib_t0013 WHERE i = 1
go
select i, PigTure from freetds_dblib_t0013 order by i
go
SET TEXTSIZE 2147483647
go
SELECT PigTure FROM freetds_dblib_t0013 WHERE i = 1
go
drop table freetds_dblib_t0013
go
