if object_id('t0022') is not null drop proc t0022
go
create proc t0022 (@b int out) as
begin
 select @b = 42
 return 66
end
go
declare @b int
exec t0022 @b = @b output
go
drop proc t0022
go
if object_id('t0022a') is not null drop proc t0022a
go
create proc t0022a (@b int) as
return @b
go
exec t0022a 17 
exec t0022a 1024
go
drop proc t0022a
go
