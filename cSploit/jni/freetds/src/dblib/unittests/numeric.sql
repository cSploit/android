IF OBJECT_ID('testDecimal') IS NOT NULL DROP PROC testDecimal
go
CREATE PROCEDURE testDecimal
  @idecimal NUMERIC(20,10)
AS
BEGIN
	SELECT @idecimal*2
END
go
IF OBJECT_ID('testDecimal') IS NOT NULL DROP PROC testDecimal
go

