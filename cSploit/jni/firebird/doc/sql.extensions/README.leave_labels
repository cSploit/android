----------------------------------------
PSQL labels and LEAVE statement (FB 2.0)
----------------------------------------

  Function:
    Allows to stop execution of the current block and unwind to the specified label.
    After that execution continues from the statement following by the terminated loop statement.

  Author:
    Dmitry Yemanov <dimitr@users.sf.net>

  Syntax rules:
    <label_name>: <loop_statement>
    ...
    LEAVE [<label_name>]
    
    Where <loop_statement> is one of: WHILE, FOR SELECT, FOR EXECUTE STATEMENT

  Example(s):
    1. FOR
         SELECT COALESCE(RDB$SYSTEM_FLAG, 0), RDB$RELATION_NAME
         FROM RDB$RELATIONS
         ORDER BY 1
         INTO :RTYPE, :RNAME
       DO
       BEGIN
         IF (RTYPE = 0) THEN
           SUSPEND;
         ELSE
           LEAVE; -- exits current loop
       END

    2. CNT = 100;
       L1:
       WHILE (CNT >= 0) DO
       BEGIN
         IF (CNT < 50) THEN
           LEAVE L1; -- exists WHILE loop
         CNT = CNT - l;
       END

    3. STMT1 = 'SELECT RDB$RELATION_NAME FROM RDB$RELATIONS';
       L1:
       FOR
         EXECUTE STATEMENT :STMT1 INTO :RNAME
       DO
       BEGIN
         STMT2 = 'SELECT RDB$FIELD_NAME FROM RDB$RELATION_FIELDS WHERE RDB$RELATION_NAME = ';
         L2:
         FOR
           EXECUTE STATEMENT :STMT2 || :RNAME INTO :FNAME
         DO
         BEGIN
           IF (RNAME = 'RDB$DATABASE') THEN
             LEAVE L1; -- exits the outer loop
           ELSE IF (RNAME = 'RDB$RELATIONS') THEN
             LEAVE L2; -- exits the inner loop
           ELSE
             SUSPEND;
         END
       END

  Note(s):
    LEAVE without explicit lable means interrupting the current (most inner) loop.
