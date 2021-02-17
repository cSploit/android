create table #dblib0018 (i int not null, s char(10) not null)
go
insert into #dblib0018 values (0, 'row 000')
go
insert into #dblib0018 values (1, 'row 001')
go
insert into #dblib0018 values (2, 'row 002')
go
insert into #dblib0018 values (3, 'row 003')
go
insert into #dblib0018 values (4, 'row 004')
go
insert into #dblib0018 values (5, 'row 005')
go
insert into #dblib0018 values (6, 'row 006')
go
insert into #dblib0018 values (7, 'row 007')
go
insert into #dblib0018 values (8, 'row 008')
go
insert into #dblib0018 values (9, 'row 009')
go
insert into #dblib0018 values (10, 'row 010')
go
insert into #dblib0018 values (11, 'row 011')
go
insert into #dblib0018 values (12, 'row 012')
go
insert into #dblib0018 values (13, 'row 013')
go
insert into #dblib0018 values (14, 'row 014')
go
insert into #dblib0018 values (15, 'row 015')
go
insert into #dblib0018 values (16, 'row 016')
go
insert into #dblib0018 values (17, 'row 017')
go
insert into #dblib0018 values (18, 'row 018')
go
insert into #dblib0018 values (19, 'row 019')
go
insert into #dblib0018 values (20, 'row 020')
go
insert into #dblib0018 values (21, 'row 021')
go
insert into #dblib0018 values (22, 'row 022')
go
insert into #dblib0018 values (23, 'row 023')
go
insert into #dblib0018 values (24, 'row 024')
go
insert into #dblib0018 values (25, 'row 025')
go
insert into #dblib0018 values (26, 'row 026')
go
insert into #dblib0018 values (27, 'row 027')
go
insert into #dblib0018 values (28, 'row 028')
go
insert into #dblib0018 values (29, 'row 029')
go
insert into #dblib0018 values (30, 'row 030')
go
insert into #dblib0018 values (31, 'row 031')
go
insert into #dblib0018 values (32, 'row 032')
go
insert into #dblib0018 values (33, 'row 033')
go
insert into #dblib0018 values (34, 'row 034')
go
insert into #dblib0018 values (35, 'row 035')
go
insert into #dblib0018 values (36, 'row 036')
go
insert into #dblib0018 values (37, 'row 037')
go
insert into #dblib0018 values (38, 'row 038')
go
insert into #dblib0018 values (39, 'row 039')
go
insert into #dblib0018 values (40, 'row 040')
go
insert into #dblib0018 values (41, 'row 041')
go
insert into #dblib0018 values (42, 'row 042')
go
insert into #dblib0018 values (43, 'row 043')
go
insert into #dblib0018 values (44, 'row 044')
go
insert into #dblib0018 values (45, 'row 045')
go
insert into #dblib0018 values (46, 'row 046')
go
insert into #dblib0018 values (47, 'row 047')
go
insert into #dblib0018 values (48, 'row 048')
go
insert into #dblib0018 values (49, 'row 049')
go
select * into #tmp0 from #dblib0018
select * from #tmp0 order by i
go
update #dblib0018 set s = 'row 000'
go
