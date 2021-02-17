----------------------------------
CURRENT_TIME and CURRENT_TIMESTAMP
----------------------------------

  Function:
    Returns current server system time or date+time values.

  Syntax rules:
    CURRENT_TIME [(<seconds precision>)]
    CURRENT_TIMESTAMP [(<seconds precision>)]

  Type:
    TIME, TIMESTAMP

  Scope:
    DSQL, PSQL

  Example(s):
    1. SELECT CURRENT_TIME FROM RDB$DATABASE;
    2. SELECT CURRENT_TIME(3) FROM RDB$DATABASE;
    3. SELECT CURRENT_TIMESTAMP(3) FROM RDB$DATABASE;

  Note(s):
    1. The maximum possible precision is 3 which means accuracy of 1/1000 second
       (one millisecond). This accuracy may be improved in the future versions.
    2. If no seconds precision is specified, the following values are implicit:
       - 0 for CURRENT_TIME
       - 3 for CURRENT_TIMESTAMP
