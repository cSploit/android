CREATE GENERATOR CHANGE_NUM;

/* Domain definitions */
CREATE DOMAIN D_ACTION AS BLOB SUB_TYPE 0 SEGMENT SIZE 0;
CREATE DOMAIN CHANGE_DATE AS TIMESTAMP;
CREATE DOMAIN CHANGE_NUMBER AS INTEGER;
CREATE DOMAIN CHANGE_WHO AS VARCHAR(32);
CREATE DOMAIN CLASS AS CHAR(10);
CREATE DOMAIN DESCRIPTION AS BLOB SUB_TYPE 0 SEGMENT SIZE 80;
CREATE DOMAIN EXPLANATION AS BLOB SUB_TYPE 0 SEGMENT SIZE 0;
CREATE DOMAIN FACILITY AS CHAR(15);
CREATE DOMAIN FAC_CODE AS SMALLINT;
CREATE DOMAIN D_FILE AS VARCHAR(30);
CREATE DOMAIN FLAGS AS SMALLINT;
CREATE DOMAIN LANGUAGE AS VARCHAR(10);
CREATE DOMAIN LAST_CHANGE AS TIMESTAMP;
CREATE DOMAIN LOCALE AS VARCHAR(20);
CREATE DOMAIN MAX_NUMBER AS SMALLINT;
CREATE DOMAIN MODULE AS VARCHAR(32);
CREATE DOMAIN NUMBER AS SMALLINT;
CREATE DOMAIN OLD_SEQUENCE AS SMALLINT;
CREATE DOMAIN ROUTINE AS VARCHAR(40);
CREATE DOMAIN SEQUENCE AS SMALLINT;
CREATE DOMAIN SEVERITY AS SMALLINT;
CREATE DOMAIN SEVERITY_TEXT AS CHAR(7);
CREATE DOMAIN SQL_CLASS AS CHAR(2);
CREATE DOMAIN SQL_CODE AS SMALLINT;
CREATE DOMAIN SQL_STATE AS CHAR(5);
CREATE DOMAIN SQL_SUBCLASS AS CHAR(3);
CREATE DOMAIN SYMBOL AS VARCHAR(32);
CREATE DOMAIN TEMPLATE AS BLOB SUB_TYPE 0 SEGMENT SIZE 256;
CREATE DOMAIN TEXT AS VARCHAR(118);
CREATE DOMAIN TRANSLATOR AS VARCHAR(32);
CREATE DOMAIN TRANS_DATE AS TIMESTAMP;
CREATE DOMAIN TRANS_NOTES AS BLOB SUB_TYPE 0 SEGMENT SIZE 0;
CREATE DOMAIN D_TYPE AS CHAR(12);
CREATE DOMAIN D_VALUE AS INTEGER;

COMMIT;

/* Tables */

CREATE TABLE FACILITIES (
    LAST_CHANGE  LAST_CHANGE,
    FACILITY     FACILITY NOT NULL CONSTRAINT FAC_2 UNIQUE,
    FAC_CODE     FAC_CODE NOT NULL CONSTRAINT FAC_1 PRIMARY KEY,
    MAX_NUMBER   MAX_NUMBER
);

CREATE TABLE HISTORY (
    CHANGE_NUMBER    CHANGE_NUMBER NOT NULL CONSTRAINT HIS_1 PRIMARY KEY,
    CHANGE_WHO       CHANGE_WHO NOT NULL,
    CHANGE_DATE      CHANGE_DATE NOT NULL,
    FAC_CODE         FAC_CODE NOT NULL /*REFERENCES FACILITIES*/,
    NUMBER           NUMBER NOT NULL,
    OLD_TEXT         TEXT,
    OLD_ACTION       D_ACTION,
    OLD_EXPLANATION  EXPLANATION,
    LOCALE           LOCALE,
    CODE             COMPUTED BY (FAC_CODE * 10000 + NUMBER)
);

CREATE TABLE LOCALES (
    LOCALE       LOCALE NOT NULL CONSTRAINT PRIMARY_LOCALES PRIMARY KEY,
    DESCRIPTION  DESCRIPTION
);

CREATE TABLE MESSAGES (
    SYMBOL       SYMBOL CONSTRAINT MSGSYMBOL UNIQUE,
    ROUTINE      ROUTINE,
    MODULE       MODULE,
    TRANS_NOTES  TRANS_NOTES,
    FAC_CODE     FAC_CODE NOT NULL REFERENCES FACILITIES,
    NUMBER       NUMBER NOT NULL,
    FLAGS        FLAGS,
    TEXT         TEXT NOT NULL,
    "ACTION"     D_ACTION,
    EXPLANATION  EXPLANATION,
    CODE         COMPUTED BY (FAC_CODE * 10000 + NUMBER),
    CONSTRAINT MESSAGES_PK PRIMARY KEY (FAC_CODE, NUMBER),
    CONSTRAINT CNS UNIQUE (FAC_CODE, NUMBER, SYMBOL)
);

CREATE TABLE SQLSTATES (
    SQL_CLASS       SQL_CLASS NOT NULL,
    SQL_SUBCLASS    SQL_SUBCLASS DEFAULT '000' NOT NULL,
    SQL_STATE       COMPUTED BY (CAST(SQL_CLASS || SQL_SUBCLASS AS SQL_STATE)),
    SQL_STATE_TEXT  TEXT NOT NULL,
    CONSTRAINT SQLSTATES_PK PRIMARY KEY (SQL_CLASS, SQL_SUBCLASS)
);

CREATE TABLE SYMBOLS (
    SYMBOL    SYMBOL NOT NULL CONSTRAINT SYM2 PRIMARY KEY,
    "VALUE"   D_VALUE,
    CLASS     CLASS,
    "TYPE"    D_TYPE,
    SEQUENCE  SEQUENCE
);

CREATE TABLE SYSTEM_ERRORS (
    SQL_CODE       SQL_CODE NOT NULL,
    SQL_CLASS      SQL_CLASS NOT NULL,
    SQL_SUBCLASS   SQL_SUBCLASS NOT NULL,
    FAC_CODE       FAC_CODE NOT NULL REFERENCES FACILITIES,
    NUMBER         NUMBER NOT NULL,
    GDS_SYMBOL     SYMBOL NOT NULL CONSTRAINT SYSERR2 UNIQUE,
    SEVERITY       SEVERITY,
    SEVERITY_TEXT  SEVERITY_TEXT,
    CODE           COMPUTED BY (FAC_CODE * 10000 + NUMBER),
    SQL_STATE      COMPUTED BY (CAST(SQL_CLASS || SQL_SUBCLASS AS SQL_STATE)),
    CONSTRAINT SYSTEM_ERRORS_PK PRIMARY KEY (FAC_CODE, NUMBER),
    CONSTRAINT SQL_STATE_FK FOREIGN KEY (SQL_CLASS, SQL_SUBCLASS) REFERENCES SQLSTATES (SQL_CLASS, SQL_SUBCLASS),
    --CONSTRAINT SYMBOL_FK FOREIGN KEY (GDS_SYMBOL) REFERENCES MESSAGES (SYMBOL)
    CONSTRAINT CNS_FK FOREIGN KEY (FAC_CODE, NUMBER, GDS_SYMBOL)
        REFERENCES MESSAGES (FAC_CODE, NUMBER, SYMBOL)
);

CREATE TABLE TEMPLATES (
    LANGUAGE  LANGUAGE,
    "FILE"    D_FILE,
    TEMPLATE  TEMPLATE
);

COMMIT;

CREATE TABLE TRANSMSGS (
    ENG_TEXT         TEXT,
    ENG_ACTION       D_ACTION,
    ENG_EXPLANATION  EXPLANATION,
    FAC_CODE         FAC_CODE NOT NULL, -- REFERENCES FACILITIES, redundant, see FULLCODE_FK
    NUMBER           NUMBER NOT NULL,
    LOCALE           LOCALE NOT NULL CONSTRAINT LOCALE_FK REFERENCES LOCALES,
    TEXT             TEXT NOT NULL,
    "ACTION"         D_ACTION,
    EXPLANATION      EXPLANATION,
    TRANSLATOR       TRANSLATOR,
    TRANS_DATE       TRANS_DATE,
    CODE             COMPUTED BY (FAC_CODE * 10000 + NUMBER),
    CONSTRAINT TRANSMSGS_PK PRIMARY KEY (LOCALE, FAC_CODE, NUMBER),
    CONSTRAINT FULLCODE_FK FOREIGN KEY (FAC_CODE, NUMBER) REFERENCES MESSAGES (FAC_CODE, NUMBER)
);

COMMIT;

/*  Index definitions for all user tables */
CREATE INDEX SYM1 ON SYMBOLS(CLASS, "TYPE");

COMMIT;

SET TERM ^ ;

/* Triggers */
CREATE TRIGGER FACILITIES$STORE FOR FACILITIES
ACTIVE BEFORE INSERT POSITION 0 AS
begin
	new.last_change = CURRENT_TIMESTAMP;
end^

CREATE TRIGGER FACILITIES$MODIFY FOR FACILITIES
ACTIVE BEFORE UPDATE POSITION 0 AS
begin
	new.last_change = CURRENT_TIMESTAMP;
end^

CREATE TRIGGER MESSAGES$MODIFY FOR MESSAGES
ACTIVE AFTER UPDATE POSITION 0 AS
begin
	update facilities
		set last_change = current_timestamp
		where fac_code = new.fac_code;
end^

CREATE TRIGGER MSGS$MODIFY FOR MESSAGES
ACTIVE AFTER UPDATE POSITION 1 AS
begin
   if (new.text <> old.text) then
      begin
		insert into history (change_number, change_date, change_who, fac_code,
							 number, old_text, old_action, old_explanation, locale)
			values (gen_id(change_num, 1), current_timestamp, current_user,
					old.fac_code, old.number, old.text, old."ACTION",
					old.explanation, 'c_pg');
      end
end^

CREATE TRIGGER TRANSMSGS$STORE FOR TRANSMSGS
ACTIVE BEFORE INSERT POSITION 0 AS
begin
   if (new.translator is NULL) then
	   new.translator = current_user;
   if (new.trans_date is NULL) then
	   new.trans_date = current_timestamp;
end^

CREATE TRIGGER TRANSMSGS$MODIFY FOR TRANSMSGS
ACTIVE AFTER UPDATE POSITION 0 AS
begin
   if (new.text <> old.text) then
      begin
		insert into history (change_number, change_date, change_who, fac_code,
							 number, old_text, old_action, old_explanation, locale)
			values (gen_id(change_num, 1), current_timestamp, current_user,
					old.fac_code, old.number, old.text, old."ACTION",
					old.explanation, old.locale);
      end
end^

COMMIT^
SET TERM ; ^

-- Grant permission to the general audience to read these tables.
GRANT SELECT ON FACILITIES    TO PUBLIC;
GRANT SELECT ON HISTORY       TO PUBLIC;
GRANT SELECT ON LOCALES       TO PUBLIC;
GRANT SELECT ON MESSAGES      TO PUBLIC;
GRANT SELECT ON SYMBOLS       TO PUBLIC;
GRANT SELECT ON SYSTEM_ERRORS TO PUBLIC;
GRANT SELECT ON TEMPLATES     TO PUBLIC;
GRANT SELECT ON TRANSMSGS     TO PUBLIC;

COMMIT;

/* Notes:

1) Dumping new messages that need translation.

select '(''' || replace(m.text, '''', '''''') ||
	''', NULL, NULL, ' || m.fac_code || ', ' || m.number ||
	', ''#'', NULL, NULL, ''truser'', ''1000-01-01 00:00:00'')'
from messages m
where not exists (select * from transmsgs t
				where t.fac_code = m.fac_code and t.number = m.number)
order by m.fac_code, m.number;

2) Identify messages whose default text in the English main version is different
than in the localized version (it leads to screwed human translation, since the
English version serves as model to translate).

select m.fac_code, m.number, t.locale
from messages m
join transmsgs t
on m.fac_code = t.fac_code and m.number = t.number
where m.text <> t.eng_text
order by m.fac_code, m.number;

3) Identify messages whose number of parameters in the English main version
is different than in the localized versions (it leads old clients to crash).
Starting with the new parameters using @, it's possible to reorder the parameters
in localized version to suit each human language's syntax, but the old messages
before the change have to be maintained in the strict same order, because old
clients use the old printf syntax that's rigid.

select t.fac_code, t.number, t.locale
from transmsgs t
where char_length(t.eng_text) - char_length(replace(t.eng_text, '@', '')) <>
	char_length(t.text) - char_length(replace(t.text, '@', ''))
order by t.fac_code, t.number;

4) The following procedure verifies some oddities in translated messages,
regarding the arguments used. Not necessarily errors, because there may be the
need to put params out of order in translated messages when compared to the English
version or to repeat an argument (features that couldn't be done with the
original, printf-based system). The argument for the procedure is the locale. Example:
select * from verifymsg('de_DE');

Claudio Valderrama C., March 2008.
*/

set term ^;
create procedure verifymsg(inloc varchar(5))
returns (reason varchar(20), fac int, num int, loc varchar(5))
as
	declare flags int;
	declare test1 int;
	declare test2 int;
	declare test3 int;
	declare test4 int;
	declare test5 int;
	declare iter int;
	declare found_loop int;
	declare efound int;
	declare newpos int;
	declare testchar char;
	declare len int;
	declare a1 int;
	declare a2 int;
	declare a3 int;
	declare a4 int;
	declare a5 int;
	-- declare fac int;
	-- declare num int;
	-- declare loc varchar(5);
	declare trans_text type of text;
begin
	for select m.fac_code, m.number, t.locale, t.text
	from messages m
	join transmsgs t
	on m.fac_code = t.fac_code and m.number = t.number
	where t.locale = :inloc
	order by m.fac_code, m.number
	into :fac, :num, :loc, :trans_text
	do begin
		a1 = 0;
		a2 = 0;
		a3 = 0;
		a4 = 0;
		a5 = 0;
		newpos = 0;
		flags = 0;
		found_loop = 0;
		iter = 1;
		efound = 0;

		len = char_length(trans_text);
		while (iter <= len)
		do begin
			newpos = position('@', trans_text, iter);
			if (newpos = 0)
			then break;
			testchar = substring(trans_text from newpos + 1 for 1);
			found_loop = 1;

			if (testchar = '1')
			then a1 = a1 + 1;
			else if (testchar = '2')
			then a2 = a2 + 1;
			else if (testchar = '3')
			then a3 = a3 + 1;
			else if (testchar = '4')
			then a4 = a4 + 1;
			else if (testchar = '5')
			then a5 = a5 + 1;
			else if (testchar <> '@')
			then begin
				efound = 1;
				reason = 'Found ' || testchar || ' after @';
				break;
			end

			if (testchar <> '@')
			then flags = bin_or(flags, bin_shl(1, cast(testchar as int)));
			iter = newpos + 2;
		end

		if (efound = 0)
		then if (a1 > 1 or a2 > 1 or a3 > 1 or a4 > 1 or a5 > 1)
		then begin
			efound = 1;
			reason = 'One param has count > 1';
		end
		else if (found_loop = 1)
		then begin
			-- efound = 0;
			if (bin_and(flags, 2) > 0) -- 2 == bin_shl(1, 1)
			then found_loop = 1;
			else found_loop = 0;

			iter = 2;
			while (iter < 6) do
			begin
				if (bin_and(flags, bin_shl(1, iter)) > 0)
				then begin
					if (found_loop = 0)
					then begin
						efound = 1;
						reason = '@' || cast(iter as varchar(1)) || ' out of order';
						break;
					end
					-- else found_loop = 1;
				end
				else found_loop = 0;
				iter = iter + 1;
			end
		end

		if (efound = 1)
		then suspend;
	end
end ^
set term ;^

grant execute on procedure verifymsg to public;

commit;


