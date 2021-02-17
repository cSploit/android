create table #dblib0005 (i int not null, s char(10) not null)
go
insert into #dblib0005 values (1, 'row 0001')
go
insert into #dblib0005 values (2, 'row 0002')
go
insert into #dblib0005 values (3, 'row 0003')
go
insert into #dblib0005 values (4, 'row 0004')
go
insert into #dblib0005 values (5, 'row 0005')
go
insert into #dblib0005 values (6, 'row 0006')
go
insert into #dblib0005 values (7, 'row 0007')
go
insert into #dblib0005 values (8, 'row 0008')
go
insert into #dblib0005 values (9, 'row 0009')
go
insert into #dblib0005 values (10, 'row 0010')
go
insert into #dblib0005 values (11, 'row 0011')
go
insert into #dblib0005 values (12, 'row 0012')
go
insert into #dblib0005 values (13, 'row 0013')
go
insert into #dblib0005 values (14, 'row 0014')
go
insert into #dblib0005 values (15, 'row 0015')
go
insert into #dblib0005 values (16, 'row 0016')
go
insert into #dblib0005 values (17, 'row 0017')
go
insert into #dblib0005 values (18, 'row 0018')
go
insert into #dblib0005 values (19, 'row 0019')
go
insert into #dblib0005 values (20, 'row 0020')
go
insert into #dblib0005 values (21, 'row 0021')
go
insert into #dblib0005 values (22, 'row 0022')
go
insert into #dblib0005 values (23, 'row 0023')
go
insert into #dblib0005 values (24, 'row 0024')
go
insert into #dblib0005 values (25, 'row 0025')
go
insert into #dblib0005 values (26, 'row 0026')
go
insert into #dblib0005 values (27, 'row 0027')
go
insert into #dblib0005 values (28, 'row 0028')
go
insert into #dblib0005 values (29, 'row 0029')
go
insert into #dblib0005 values (30, 'row 0030')
go
insert into #dblib0005 values (31, 'row 0031')
go
insert into #dblib0005 values (32, 'row 0032')
go
insert into #dblib0005 values (33, 'row 0033')
go
insert into #dblib0005 values (34, 'row 0034')
go
insert into #dblib0005 values (35, 'row 0035')
go
insert into #dblib0005 values (36, 'row 0036')
go
insert into #dblib0005 values (37, 'row 0037')
go
insert into #dblib0005 values (38, 'row 0038')
go
insert into #dblib0005 values (39, 'row 0039')
go
insert into #dblib0005 values (40, 'row 0040')
go
insert into #dblib0005 values (41, 'row 0041')
go
insert into #dblib0005 values (42, 'row 0042')
go
insert into #dblib0005 values (43, 'row 0043')
go
insert into #dblib0005 values (44, 'row 0044')
go
insert into #dblib0005 values (45, 'row 0045')
go
insert into #dblib0005 values (46, 'row 0046')
go
insert into #dblib0005 values (47, 'row 0047')
go
insert into #dblib0005 values (48, 'row 0048')
go
insert into #dblib0005 values (49, 'row 0049')
go
select * from #dblib0005 where i < 5 order by i
go
select * from #dblib0005 where i < 6 order by i
go
/*
 * The following query should fail because the test 
 * has not fetched all the rows in the result set.
 */
select * from #dblib0005 where i > 950 order by i
go
if object_id('t0005_proc') is not null drop procedure t0005_proc
go
create proc t0005_proc (@b int out) as
begin
select * from #dblib0005 where i < 6 order by i
select @b = 42
end

go
declare @myout int exec t0005_proc @b = @myout output
go
select getdate()
go
drop procedure t0005_proc
go
