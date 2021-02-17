create table #dblib (i int not null, s text)
go
insert into #dblib values (1, 'ABCDEF')
go
insert into #dblib values (2, 'abc')
go
select * from #dblib order by i
go
