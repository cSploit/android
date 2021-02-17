create table #dblib0017 (c1 int null, c2 text)
go
insert into #dblib0017(c1,c2) values(1144201745,'prova di testo questo testo dovrebbe andare a finire in un campo text')
go
select * from #dblib0017 where 0=1
go
delete from #dblib0017
go
select * from #dblib0017 where 0=1
go
SET NOCOUNT ON DECLARE @n INT SELECT @n = COUNT(*) FROM #dblib0017 WHERE c1=1144201745 AND c2 LIKE 'prova di testo questo testo dovrebbe andare a finire in un campo text' IF @n <> 1 SELECT 0
go
