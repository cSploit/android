create table #dblib0009 (i int not null, s char(10) not null)
go
insert into #dblib0009 values (1, 'abcdef')
go
insert into #dblib0009 values (2, 'abc')
go
select * from #dblib0009 order by i
go
