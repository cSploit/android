if exists ( select 1 from tempdb..sysobjects where id = object_id('tempdb..#dblib0002') )
	drop table #dblib0002

go
create table #dblib0002 (i int not null, s char(10) not null)
go
insert into #dblib0002 values (1, 'row 001')
go
insert into #dblib0002 values (2, 'row 002')
go
insert into #dblib0002 values (3, 'row 003')
go
insert into #dblib0002 values (4, 'row 004')
go
insert into #dblib0002 values (5, 'row 005')
go
insert into #dblib0002 values (6, 'row 006')
go
insert into #dblib0002 values (7, 'row 007')
go
insert into #dblib0002 values (8, 'row 008')
go
insert into #dblib0002 values (9, 'row 009')
go
insert into #dblib0002 values (10, 'row 010')
go
insert into #dblib0002 values (11, 'row 011')
go
insert into #dblib0002 values (12, 'row 012')
go
insert into #dblib0002 values (13, 'row 013')
go
insert into #dblib0002 values (14, 'row 014')
go
insert into #dblib0002 values (15, 'row 015')
go
insert into #dblib0002 values (16, 'row 016')
go
insert into #dblib0002 values (17, 'row 017')
go
insert into #dblib0002 values (18, 'row 018')
go
insert into #dblib0002 values (19, 'row 019')
go
insert into #dblib0002 values (20, 'row 020')
go
insert into #dblib0002 values (21, 'row 021')
go
insert into #dblib0002 values (22, 'row 022')
go
insert into #dblib0002 values (23, 'row 023')
go
insert into #dblib0002 values (24, 'row 024')
go
insert into #dblib0002 values (25, 'row 025')
go
insert into #dblib0002 values (26, 'row 026')
go
insert into #dblib0002 values (27, 'row 027')
go
insert into #dblib0002 values (28, 'row 028')
go
insert into #dblib0002 values (29, 'row 029')
go
insert into #dblib0002 values (30, 'row 030')
go
insert into #dblib0002 values (31, 'row 031')
go
insert into #dblib0002 values (32, 'row 032')
go
insert into #dblib0002 values (33, 'row 033')
go
insert into #dblib0002 values (34, 'row 034')
go
insert into #dblib0002 values (35, 'row 035')
go
insert into #dblib0002 values (36, 'row 036')
go
insert into #dblib0002 values (37, 'row 037')
go
insert into #dblib0002 values (38, 'row 038')
go
insert into #dblib0002 values (39, 'row 039')
go
insert into #dblib0002 values (40, 'row 040')
go
insert into #dblib0002 values (41, 'row 041')
go
insert into #dblib0002 values (42, 'row 042')
go
insert into #dblib0002 values (43, 'row 043')
go
insert into #dblib0002 values (44, 'row 044')
go
insert into #dblib0002 values (45, 'row 045')
go
insert into #dblib0002 values (46, 'row 046')
go
insert into #dblib0002 values (47, 'row 047')
go
insert into #dblib0002 values (48, 'row 048')
go
insert into #dblib0002 values (49, 'row 049')
go
insert into #dblib0002 values (50, 'row 050')
go
select * from #dblib0002 order by i
select * from #dblib0002 order by i
go
