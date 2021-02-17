create table #dblib0023 (col1 int not null,  col2 char(1) not null, col3 datetime not null)
go
insert into #dblib0023 values (1, 'A', 'Jan  1 2002 10:00:00AM')
go
insert into #dblib0023 values (2, 'A', 'Jan  2 2002 10:00:00AM')
go
insert into #dblib0023 values (3, 'A', 'Jan  3 2002 10:00:00AM')
go
insert into #dblib0023 values (8, 'B', 'Jan  4 2002 10:00:00AM')
go
insert into #dblib0023 values (9, 'B', 'Jan  5 2002 10:00:00AM')
go
select col1, col2, col3 from #dblib0023 order by col2 compute sum(col1) by col2 compute max(col3)
go
