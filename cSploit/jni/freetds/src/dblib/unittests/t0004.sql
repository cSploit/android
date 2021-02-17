create table #dblib0004 (i int not null, s char(10) not null)
go
insert into #dblib0004 values (1, 'row 0001')
go
insert into #dblib0004 values (2, 'row 0002')
go
insert into #dblib0004 values (3, 'row 0003')
go
insert into #dblib0004 values (4, 'row 0004')
go
insert into #dblib0004 values (5, 'row 0005')
go
insert into #dblib0004 values (6, 'row 0006')
go
insert into #dblib0004 values (7, 'row 0007')
go
insert into #dblib0004 values (8, 'row 0008')
go
insert into #dblib0004 values (9, 'row 0009')
go
insert into #dblib0004 values (10, 'row 0010')
go
insert into #dblib0004 values (11, 'row 0011')
go
insert into #dblib0004 values (12, 'row 0012')
go
insert into #dblib0004 values (13, 'row 0013')
go
insert into #dblib0004 values (14, 'row 0014')
go
insert into #dblib0004 values (15, 'row 0015')
go
insert into #dblib0004 values (16, 'row 0016')
go
insert into #dblib0004 values (17, 'row 0017')
go
insert into #dblib0004 values (18, 'row 0018')
go
insert into #dblib0004 values (19, 'row 0019')
go
insert into #dblib0004 values (20, 'row 0020')
go
insert into #dblib0004 values (21, 'row 0021')
go
insert into #dblib0004 values (22, 'row 0022')
go
insert into #dblib0004 values (23, 'row 0023')
go
insert into #dblib0004 values (24, 'row 0024')
go
insert into #dblib0004 values (25, 'row 0025')
go
insert into #dblib0004 values (26, 'row 0026')
go
insert into #dblib0004 values (27, 'row 0027')
go
insert into #dblib0004 values (28, 'row 0028')
go
insert into #dblib0004 values (29, 'row 0029')
go
insert into #dblib0004 values (30, 'row 0030')
go
insert into #dblib0004 values (31, 'row 0031')
go
insert into #dblib0004 values (32, 'row 0032')
go
insert into #dblib0004 values (33, 'row 0033')
go
insert into #dblib0004 values (34, 'row 0034')
go
insert into #dblib0004 values (35, 'row 0035')
go
insert into #dblib0004 values (36, 'row 0036')
go
insert into #dblib0004 values (37, 'row 0037')
go
insert into #dblib0004 values (38, 'row 0038')
go
insert into #dblib0004 values (39, 'row 0039')
go
insert into #dblib0004 values (40, 'row 0040')
go
insert into #dblib0004 values (41, 'row 0041')
go
insert into #dblib0004 values (42, 'row 0042')
go
insert into #dblib0004 values (43, 'row 0043')
go
insert into #dblib0004 values (44, 'row 0044')
go
insert into #dblib0004 values (45, 'row 0045')
go
insert into #dblib0004 values (46, 'row 0046')
go
insert into #dblib0004 values (47, 'row 0047')
go
insert into #dblib0004 values (48, 'row 0048')
go
insert into #dblib0004 values (49, 'row 0049')
go
select * from #dblib0004 where i<=25 order by i
go
/* The following statement fails intentionally: 
 * the unit test is checking that db-lib returns FAIL
 * when results have not been fully processed. 
 */
select * from #dblib0004 where i<=25 order by i
go
