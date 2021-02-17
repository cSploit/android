CREATE PROCEDURE #t0022 
  @null_input varchar(30) OUTPUT 
, @first_type varchar(30) OUTPUT 
, @nullout int OUTPUT
, @nrows int OUTPUT 
, @c_this_name_is_way_more_than_thirty_characters_charlie varchar(20)
, @nv nvarchar(20) = N'hello'
AS 
BEGIN 
if @null_input is not NULL begin 
	select 'error: should be NULL' as status, @null_input as 'null_input'
	return -42
end else begin
	print 'Good: @null_input is NULL'
end
if @c_this_name_is_way_more_than_thirty_characters_charlie is not NULL begin 
	select 'error: should be NULL' as status, @c_this_name_is_way_more_than_thirty_characters_charlie as '@c_this_name_is_way_more_than_thirty_characters_charlie'
	return -42
end else begin
	print 'Good: @c_this_name_is_way_more_than_thirty_characters_charlie is NULL'
end
select @null_input = max(convert(varchar(30), name)) from systypes 
select @first_type = min(convert(varchar(30), name)) from systypes 
select name from sysobjects where 0=1
select distinct convert(varchar(30), name) as 'type'  from systypes 
where name in ('int', 'char', 'text') 
select @nrows = @@rowcount 
select distinct @nv as '@nv', convert(varchar(30), name) as name  from sysobjects where type = 'S' 
select	  @null_input as 'null_input'
	, @first_type as 'first_type'
	, @nullout as 'nullout'
	, @nrows as 'nrows'
	, @c_this_name_is_way_more_than_thirty_characters_charlie as 'c'
	, @nv as 'nv'
	into #parameters
select * from #parameters
return 42 
END 

go
IF OBJECT_ID('t0022') IS NOT NULL DROP PROC t0022
go
CREATE PROCEDURE t0022 
  @null_input varchar(30) OUTPUT 
, @first_type varchar(30) OUTPUT 
, @nullout int OUTPUT
, @nrows int OUTPUT 
, @c_this_name_is_way_more_than_thirty_characters_charlie varchar(20)
, @nv nvarchar(20) = N'hello'
AS 
BEGIN 
if @null_input is not NULL begin 
	select 'error: should be NULL' as status, @null_input as 'null_input'
	return -42
end else begin
	print '@null_input is NULL, as expected'
end
if @c_this_name_is_way_more_than_thirty_characters_charlie is not NULL begin 
	select 'error: should be NULL' as status, @c_this_name_is_way_more_than_thirty_characters_charlie as '@c_this_name_is_way_more_than_thirty_characters_charlie'
	return -42
end else begin
	print 'Good: @c_this_name_is_way_more_than_thirty_characters_charlie is NULL'
end
select @null_input = max(convert(varchar(30), name)) from systypes 
select @first_type = min(convert(varchar(30), name)) from systypes 
select name from sysobjects where 0=1
select distinct convert(varchar(30), name) as 'type'  from systypes 
where name in ('int', 'char', 'text') 
select @nrows = @@rowcount 
select distinct @nv as '@nv', convert(varchar(30), name) as name  from sysobjects where type = 'S' 
select	  @null_input as 'null_input'
	, @first_type as 'first_type'
	, @nullout as 'nullout'
	, @nrows as 'nrows'
	, @c_this_name_is_way_more_than_thirty_characters_charlie as 'c'
	, @nv as 'nv'
	into #parameters
select * from #parameters
return 42 
END 

go
IF OBJECT_ID('t0022') IS NOT NULL DROP PROC t0022
go
