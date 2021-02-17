if object_id('dblib0014') is not null drop table dblib0014
go
create table dblib0014 (i int not null, PigTure image not null)
go
insert into dblib0014 values (0, '')
go
insert into dblib0014 values (1, '')
go
insert into dblib0014 values (2, '')
go
SELECT PigTure FROM dblib0014 WHERE i = 0
go
SELECT PigTure FROM dblib0014 WHERE i = 1
go
SELECT PigTure FROM dblib0014 WHERE i = 2
go
select * from dblib0014 order by i
go
drop table dblib0014
go
