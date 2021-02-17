create table #dummy (s char(10))
go
insert into #dummy values('xxx')
go
if object_id('done_test') is not NULL drop proc done_test
go
create proc done_test @a varchar(10) output 
as 
	select * from #dummy
go
if object_id('done_test2') is not NULL drop proc done_test2
go
create proc done_test2 
as 
	select * from #dummy where s = 'aaa' 
	select * from #dummy
go
select * from #dummy	/* normal row */
go
set nocount on
go
select * from #dummy	/* normal row with no count */
go
set nocount off
go
select * from #dummy where 0=1	/* normal row without rows */
go
select dklghdlgkh from #dummy	/* error query */
go
/* stored procedure call with output parameters */
declare @s varchar(10) exec done_test @s output	
go
exec done_test2
go
drop proc done_test
go
drop proc done_test2
go
