create table #dblib0024 (i int not null, s char(10) not null)
go
insert into #dblib0024 values (0, 'row 000')
go
insert into #dblib0024 values (1, 'row 001')
go
insert into #dblib0024 values (2, 'row 002')
go
insert into #dblib0024 values (3, 'row 003')
go
insert into #dblib0024 values (4, 'row 004')
go
insert into #dblib0024 values (5, 'row 005')
go
insert into #dblib0024 values (6, 'row 006')
go
insert into #dblib0024 values (7, 'row 007')
go
insert into #dblib0024 values (8, 'row 008')
go
insert into #dblib0024 values (9, 'row 009')
go
select count(*) from #dblib0024 -- order by i
go
select count(*) from sysusers
select name from sysobjects compute count(name)
go
