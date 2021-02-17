create table #dblib0007 (i int not null, s char(12) not null)
go
insert into #dblib0007 values (1, 'row 0000001')
go
insert into #dblib0007 values (2, 'row 0000002')
go
insert into #dblib0007 values (3, 'row 0000003')
go
insert into #dblib0007 values (4, 'row 0000004')
go
insert into #dblib0007 values (5, 'row 0000005')
go
insert into #dblib0007 values (6, 'row 0000006')
go
insert into #dblib0007 values (7, 'row 0000007')
go
insert into #dblib0007 values (8, 'row 0000008')
go
insert into #dblib0007 values (9, 'row 0000009')
go
select * from #dblib0007 where i<=5 order by i
go
/*
 * Second select, should fail: expected_error = 20019
 */
select * from #dblib0007 where i>=5 order by i
go
/*
 * Third select for binary bindings
 */
select i, s, i from #dblib0007 x where x.i<=5 order by x.i
go
