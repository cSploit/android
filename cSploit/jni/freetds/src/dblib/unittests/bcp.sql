if exists (select 1 from sysobjects where type = 'U' and name = 'all_types_bcp_unittest') drop table all_types_bcp_unittest
go
CREATE TABLE all_types_bcp_unittest (
	  not_null_bit			bit NOT NULL

	, not_null_char			char(10) NOT NULL
	, not_null_varchar		varchar(10) NOT NULL

	, not_null_datetime		datetime NOT NULL
	, not_null_smalldatetime	smalldatetime NOT NULL

	, not_null_money		money NOT NULL
	, not_null_smallmoney		smallmoney NOT NULL

	, not_null_float		float NOT NULL
	, not_null_real			real NOT NULL

	, not_null_decimal		decimal(5,2) NOT NULL
	, not_null_numeric		numeric(5,2) NOT NULL

	, not_null_int			int NOT NULL
	, not_null_smallint		smallint NOT NULL
	, not_null_tinyint		tinyint NOT NULL

	, nullable_char			char(10)  NULL
	, nullable_varchar		varchar(10)  NULL

	, nullable_datetime		datetime  NULL
	, nullable_smalldatetime	smalldatetime  NULL

	, nullable_money		money  NULL
	, nullable_smallmoney		smallmoney  NULL

	, nullable_float		float  NULL
	, nullable_real			real  NULL

	, nullable_decimal		decimal(5,2)  NULL
	, nullable_numeric		numeric(5,2)  NULL

	, nullable_int			int  NULL
	, nullable_smallint		smallint  NULL
	, nullable_tinyint		tinyint  NULL

	/* Excludes: 
	 * binary
	 * image
	 * uniqueidentifier
	 * varbinary
	 * text
	 * timestamp
	 * nchar
	 * ntext
	 * nvarchar
	 */
)

INSERT all_types_bcp_unittest   
VALUES ( 1 -- not_null_bit

	, 'a char' -- not_null_char
	, 'a varchar' -- not_null_varchar

	, 'Dec 17 2003  3:44PM' -- not_null_datetime
	, 'Dec 17 2003  3:44PM' -- not_null_smalldatetime

	, 12.34 -- not_null_money
	, 12.34 -- not_null_smallmoney

	, 12.34 -- not_null_float
	, 12.34 -- not_null_real

	, 12.34 -- not_null_decimal
	, 12.34 -- not_null_numeric

	, 1234 -- not_null_int
	, 1234 -- not_null_smallint
	, 123  -- not_null_tinyint

	, 'a char' -- nullable_char
	, 'a varchar' -- nullable_varchar

	, 'Dec 17 2003  3:44PM' -- nullable_datetime
	, 'Dec 17 2003  3:44PM' -- nullable_smalldatetime

	, 12.34 -- nullable_money
	, 12.34 -- nullable_smallmoney

	, 12.34 -- nullable_float
	, 12.34 -- nullable_real

	, 12.34 -- nullable_decimal
	, 12.34 -- nullable_numeric

	, 1234 -- nullable_int
	, 1234 -- nullable_smallint
	, 123  -- nullable_tinyint
)
INSERT all_types_bcp_unittest
				( not_null_bit			

				, not_null_char			
				, not_null_varchar		

				, not_null_datetime		
				, not_null_smalldatetime	

				, not_null_money		
				, not_null_smallmoney		

				, not_null_float		
				, not_null_real			

				, not_null_decimal		
				, not_null_numeric		

				, not_null_int			
				, not_null_smallint		
				, not_null_tinyint		
				) 
VALUES (
	  1 -- not_null_bit

	, 'a char' -- not_null_char
	, 'a varchar' -- not_null_varchar

	, 'Dec 17 2003  3:44PM' -- not_null_datetime
	, 'Dec 17 2003  3:44PM' -- not_null_smalldatetime

	, 12.34 -- not_null_money
	, 12.34 -- not_null_smallmoney

	, 12.34 -- not_null_float
	, 12.34 -- not_null_real

	, 12.34 -- not_null_decimal
	, 12.34 -- not_null_numeric

	, 1234 -- not_null_int
	, 1234 -- not_null_smallint
	, 123  -- not_null_tinyint
)
go
select colid, cast(c.name as varchar(30)) as name, c.length , '  '+ substring('NY', convert(bit,(c.status & 8))+1,1) as Nulls from syscolumns as c left join systypes as t on c.usertype = t.usertype where c.id = object_id('all_types_bcp_unittest') order by colid
go
select   'nullable_char' as col, count(*) nrows, datalength(nullable_char) as len, nullable_char as value from all_types_bcp_unittest group by nullable_char
UNION
select   'nullable_varchar' as col, count(*) nrows, datalength(nullable_varchar) as len, nullable_varchar as value from all_types_bcp_unittest group by nullable_varchar
UNION
select   'nullable_int' as col, count(*) nrows, datalength(nullable_int) as len, cast(nullable_int as varchar(6))as value from all_types_bcp_unittest group by nullable_int
order by col, len, nrows

go
drop table all_types_bcp_unittest
go
