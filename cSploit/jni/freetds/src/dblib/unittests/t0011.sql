drop table #dblib0011
go
create table #dblib0011 (i int not null, c1 char(200) not null, c2 char(200) null, vc varchar(200) null)
go
insert into #dblib0011 values (1, 'This is a really long column to ensure that the next row ends properly.','This is a really long column to ensure that the next row ends properly.','This is a really long column to ensure that the next row ends properly.')
go
insert into #dblib0011 values (2, 'Short column','Short column','Short column')
go
insert into #dblib0011 values (3, 'Short column',NULL,NULL)
go
select * from #dblib0011 order by i
go
