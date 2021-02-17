/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */
/*CREATE DATABASE "intl_emp.fdb" DEFAULT CHARACTER SET ISO8859_1; */

/**  18-July-1994:  Clare Taylor:  Created this file, intlddl.sql, from the
 **                 empddl.sql file.  It has been modified as shown below
 **                 to create an international version of the example database.
 **
 **  Create a sample international employee database (intlemp.fdb).
 **
 **  This database keeps track of employees, departments, projects, and sales
 **  for a small company and uses ISO8859_1 for the DEFAULT CHARACTER SET.
 **
 **  International changes to this file:
 **
 **  - DOMAIN lastname:         increased field size to 25 from 20.
 **  - CUSTOMER.CUSTOMER field: increased the field size to 40. required
 **                             for long German and French company names.
 **  - CUSTOMER.ADDRESS1 and 2: increased from 30 to 40 chars.
 **  - Added 4 sorting stored procedures--may be changed in next build.
 **/

/* set echo; */

/*
 *   Define domains.
 */
CREATE DOMAIN firstname     AS VARCHAR(15) COLLATE FR_FR;

CREATE DOMAIN lastname      AS VARCHAR(25) COLLATE FR_FR;

CREATE DOMAIN phonenumber   AS VARCHAR(20);

CREATE DOMAIN countryname   AS VARCHAR(15); 

CREATE DOMAIN addressline   AS VARCHAR(40);


CREATE DOMAIN empno
    AS SMALLINT;

CREATE DOMAIN deptno
    AS CHAR(3) CHARACTER SET ASCII
    CHECK (VALUE = '000' OR (VALUE > '0' AND VALUE <= '999') OR VALUE IS NULL);

CREATE DOMAIN projno
    AS CHAR(5) CHARACTER SET ASCII
    CHECK (VALUE = UPPER (VALUE));

CREATE DOMAIN custno
    AS INTEGER
    CHECK (VALUE > 1000);

/* must begin with a letter */
CREATE DOMAIN jobcode
    AS VARCHAR(5) CHARACTER SET ASCII
    CHECK (VALUE > '99999');

CREATE DOMAIN jobgrade
    AS SMALLINT
    CHECK (VALUE BETWEEN 0 AND 6);

/* salary is in any currency type */
CREATE DOMAIN salary
    AS NUMERIC(10,2)
    DEFAULT 0
    CHECK (VALUE > 0);

/* budget is in US dollars */
CREATE DOMAIN budget
    AS DECIMAL(12,2)
    DEFAULT 50000
    CHECK (VALUE > 10000 AND VALUE <= 2000000);

CREATE DOMAIN prodtype
    AS VARCHAR(12)
    DEFAULT 'software' NOT NULL
    CHECK (VALUE IN ('software', 'hardware', 'other', 'N/A'));

CREATE DOMAIN PONUMBER
    AS CHAR(8)
    CHECK (VALUE STARTING WITH 'V');


/*
 *  Create generators.
 */

CREATE GENERATOR emp_no_gen;

CREATE GENERATOR cust_no_gen;
SET GENERATOR cust_no_gen to 1000;

COMMIT;

/*
 *  Create tables.
 */


/*
 *  Country name, currency type.
 */
CREATE TABLE country
(
    country         COUNTRYNAME NOT NULL PRIMARY KEY,
    currency        VARCHAR(10) NOT NULL
);


/*
 *  Job id, job title, minimum and maximum salary, job description,
 *  and required languages.
 *
 *  A job is defined by a multiple key, consisting of a job_code
 *  (a 5-letter job abbreviation), a job grade, and a country name
 *  indicating the salary currency type.
 *
 *  The salary range is expressed in the appropriate country's currency.
 *
 *  The job requirement is a text blob.
 *
 *  The job may also require some knowledge of foreign languages,
 *  stored in a character array.
 */
CREATE TABLE job
(
    job_code            JOBCODE NOT NULL,
    job_grade           JOBGRADE NOT NULL,
    job_country         COUNTRYNAME NOT NULL,
    job_title           VARCHAR(25) NOT NULL,
    min_salary          SALARY NOT NULL,
    max_salary          SALARY NOT NULL,
    job_requirement     BLOB(400,1),
    language_req        VARCHAR(15) [5],

    PRIMARY KEY (job_code, job_grade, job_country),
    FOREIGN KEY (job_country) REFERENCES country (country),

    CHECK (min_salary < max_salary)
);

CREATE ASCENDING INDEX minsalx ON job (job_country, min_salary);
CREATE DESCENDING INDEX maxsalx ON job (job_country, max_salary);


/*
 *  Department number, name, head department, manager id,
 *  budget, location, department phone number.
 *
 *  Each department is a sub-department in some department, determined
 *  by head_dept.  The head of this tree is the company.
 *  This information is used to produce a company organization chart.
 *
 *  Departments have managers; however, manager id can be null to allow
 *  for temporary situations where a manager needs to be hired.
 *
 *  Budget is allocated in U.S. dollars for all departments.
 *
 *  Foreign key mngr_no is added after the employee table is created,
 *  using 'alter table'.
 */
CREATE TABLE department
(
    dept_no         DEPTNO NOT NULL,
    department      VARCHAR(25) NOT NULL UNIQUE,
    head_dept       DEPTNO,
    mngr_no         EMPNO,
    budget          BUDGET,
    location        VARCHAR(15),
    phone_no        PHONENUMBER DEFAULT '555-1234',

    PRIMARY KEY (dept_no),
    FOREIGN KEY (head_dept) REFERENCES department (dept_no)
);

CREATE DESCENDING INDEX budgetx ON department (budget);


/*
 *  Employee id, name, phone extension, date of hire, department id,
 *  job and salary information.
 *
 *  Salary can be entered in any country's currency.
 *  Therefore, some of the salaries can appear magnitudes larger than others,
 *  depending on the currency type.  Ex. Italian lira vs. U.K. pound.
 *  The currency type is determined by the country code.
 *
 *  job_code, job_grade, and job_country reference employee's job information,
 *  illustrating two tables related by referential constraints on multiple
 *  columns.
 *
 *  The employee salary is verified to be in the correct salary range
 *  for the given job title.
 */
CREATE TABLE employee
(
    emp_no          EMPNO NOT NULL,
    first_name      FIRSTNAME NOT NULL,
    last_name       LASTNAME NOT NULL,
    phone_ext       VARCHAR(4),
    hire_date       TIMESTAMP DEFAULT 'NOW' NOT NULL,
    dept_no         DEPTNO NOT NULL,
    job_code        JOBCODE NOT NULL,
    job_grade       JOBGRADE NOT NULL,
    job_country     COUNTRYNAME NOT NULL,
    salary          SALARY NOT NULL,
    full_name       COMPUTED BY (last_name || ', ' || first_name),

    PRIMARY KEY (emp_no),
    FOREIGN KEY (dept_no) REFERENCES
            department (dept_no),
    FOREIGN KEY (job_code, job_grade, job_country) REFERENCES
            job (job_code, job_grade, job_country),

    CHECK ( salary >= (SELECT min_salary FROM job WHERE
                        job.job_code = employee.job_code AND
                        job.job_grade = employee.job_grade AND
                        job.job_country = employee.job_country) AND
            salary <= (SELECT max_salary FROM job WHERE
                        job.job_code = employee.job_code AND
                        job.job_grade = employee.job_grade AND
                        job.job_country = employee.job_country))
);

CREATE INDEX namex ON employee (last_name, first_name);

CREATE VIEW phone_list AS SELECT
    emp_no, first_name, last_name, phone_ext, location, phone_no
    FROM employee, department
    WHERE employee.dept_no = department.dept_no;

COMMIT;

SET TERM !! ;

CREATE TRIGGER set_emp_no FOR employee
BEFORE INSERT AS
BEGIN
    new.emp_no = gen_id(emp_no_gen, 1);
END !!

SET TERM ; !!


/*
 *  Add an additional constraint to department: check manager numbers
 *  in the employee table.
 */
ALTER TABLE department ADD FOREIGN KEY (mngr_no) REFERENCES employee (emp_no);


/*
 *  Project id, project name, description, project team leader,
 *  and product type.
 *
 *  Project description is a text blob.
 */
CREATE TABLE project
(
    proj_id         PROJNO NOT NULL,
    proj_name       VARCHAR(20) NOT NULL UNIQUE,
    proj_desc       BLOB(800,1),
    team_leader     EMPNO,
    product         PRODTYPE,

    PRIMARY KEY (proj_id),
    FOREIGN KEY (team_leader) REFERENCES employee (emp_no)
);

CREATE UNIQUE INDEX prodtypex ON project (product, proj_name);


/*
 *  Employee id, project id, employee's project duties.
 *
 *  Employee duties is a text blob.
 */
CREATE TABLE employee_project
(
    emp_no          EMPNO NOT NULL,
    proj_id         PROJNO NOT NULL,

    PRIMARY KEY (emp_no, proj_id),
    FOREIGN KEY (emp_no) REFERENCES employee (emp_no),
    FOREIGN KEY (proj_id) REFERENCES project (proj_id)
);


/*
 *  Fiscal year, project id, department id, projected head count by
 *  fiscal quarter, projected budget.
 *
 *  Tracks head count and budget planning by project by department.
 *
 *  Quarterly head count is an array of integers.
 */
CREATE TABLE proj_dept_budget
(
    fiscal_year         INTEGER NOT NULL CHECK (FISCAL_YEAR >= 1993),
    proj_id             PROJNO NOT NULL,
    dept_no             DEPTNO NOT NULL,
    quart_head_cnt      INTEGER [4],
    projected_budget    BUDGET,

    PRIMARY KEY (fiscal_year, proj_id, dept_no),
    FOREIGN KEY (dept_no) REFERENCES department (dept_no),
    FOREIGN KEY (proj_id) REFERENCES project (proj_id)
);


/*
 *  Employee number, salary change date, updater's user id, old salary,
 *  and percent change between old and new salary.
 */
CREATE TABLE salary_history
(
    emp_no              EMPNO NOT NULL,
    change_date         TIMESTAMP DEFAULT 'NOW' NOT NULL,
    updater_id          VARCHAR(20) NOT NULL,
    old_salary          SALARY NOT NULL,
    percent_change      DOUBLE PRECISION
                            DEFAULT 0
                            NOT NULL
                            CHECK (percent_change between -50 and 50),
    new_salary          COMPUTED BY
                            (old_salary + old_salary * percent_change / 100),

    PRIMARY KEY (emp_no, change_date, updater_id),
    FOREIGN KEY (emp_no) REFERENCES employee (emp_no)
);

CREATE INDEX updaterx ON salary_history (updater_id);
CREATE DESCENDING INDEX changex ON salary_history (change_date);

COMMIT;

SET TERM !! ;

CREATE TRIGGER save_salary_change FOR employee
AFTER UPDATE AS
BEGIN
    IF (old.salary <> new.salary) THEN
        INSERT INTO salary_history
            (emp_no, change_date, updater_id, old_salary, percent_change)
        VALUES (
            old.emp_no,
            'NOW',
            user,
            old.salary,
            (new.salary - old.salary) * 100 / old.salary);
END !!

SET TERM ; !!

COMMIT;


/*
 *  Customer id, customer name, contact first and last names,
 *  phone number, address lines, city, state or province, country,
 *  postal code or zip code, and customer status.
 */
CREATE TABLE customer
(
    cust_no             CUSTNO NOT NULL,
    customer            VARCHAR(35) NOT NULL COLLATE FR_FR,
    contact_first       FIRSTNAME,
    contact_last        LASTNAME,
    phone_no            PHONENUMBER,
    address_line1       ADDRESSLINE,
    address_line2       ADDRESSLINE,
    city                VARCHAR(25),
    state_province      VARCHAR(15),
    country             COUNTRYNAME,
    postal_code         VARCHAR(12),
    on_hold             CHAR
                            DEFAULT NULL
                            CHECK (on_hold IS NULL OR on_hold = '*'),
    PRIMARY KEY (cust_no),
    FOREIGN KEY (country) REFERENCES country (country)
);

CREATE INDEX custnamex ON customer (customer);
CREATE INDEX custregion ON customer (country, city);

SET TERM !! ;

CREATE TRIGGER set_cust_no FOR customer
BEFORE INSERT AS
BEGIN
    new.cust_no = gen_id(cust_no_gen, 1);
END !!

SET TERM ; !!

COMMIT;


/*
 *  Purchase order number, customer id, sales representative, order status,
 *  order date, date shipped, date need to ship by, payment received flag,
 *  quantity ordered, total order value, type of product ordered,
 *  any percent discount offered.
 *
 *  Tracks customer orders.
 *
 *  sales_rep is the ID of the employee handling the sale.
 *
 *  Number of days passed since the order date is a computed field.
 *
 *  Several checks are performed on this table, among them:
 *      - A sale order must have a status: open, shipped, waiting.
 *      - The ship date must be entered, if order status is 'shipped'.
 *      - New orders can't be shipped to customers with 'on_hold' status.
 *      - Sales rep
 */
CREATE TABLE sales
(
    po_number       PONUMBER NOT NULL,
    cust_no         CUSTNO NOT NULL,
    sales_rep       EMPNO,
    order_status    VARCHAR(7)
                        DEFAULT 'new'
                        NOT NULL
                        CHECK (order_status in
                            ('new', 'open', 'shipped', 'waiting')),
    order_date      TIMESTAMP 
			DEFAULT 'NOW'
                        NOT NULL,
    ship_date       TIMESTAMP
                        CHECK (ship_date >= order_date OR ship_date IS NULL),
    date_needed     TIMESTAMP
                        CHECK (date_needed > order_date OR date_needed IS NULL),
    paid            CHAR
                        DEFAULT 'n'
                        CHECK (paid in ('y', 'n')),
    qty_ordered     INTEGER
                        DEFAULT 1
                        NOT NULL
                        CHECK (qty_ordered >= 1),
    total_value     DECIMAL(9,2)
                        NOT NULL
                        CHECK (total_value >= 0),
    discount        FLOAT
                        DEFAULT 0
                        NOT NULL
                        CHECK (discount >= 0 AND discount <= 1),
    item_type       PRODTYPE,
    aged            COMPUTED BY
                        (ship_date - order_date),

    PRIMARY KEY (po_number),
    FOREIGN KEY (cust_no) REFERENCES customer (cust_no),
    FOREIGN KEY (sales_rep) REFERENCES employee (emp_no),

    CHECK (NOT (order_status = 'shipped' AND ship_date IS NULL)),

    CHECK (NOT (order_status = 'shipped' AND
            EXISTS (SELECT on_hold FROM customer
                    WHERE customer.cust_no = sales.cust_no
                    AND customer.on_hold = '*')))
);

CREATE INDEX needx ON sales (date_needed);
CREATE INDEX salestatx ON sales (order_status, paid);
CREATE DESCENDING INDEX qtyx ON sales (item_type, qty_ordered);

SET TERM !! ;

CREATE TRIGGER post_new_order FOR sales
AFTER INSERT AS
BEGIN
    POST_EVENT 'new_order';
END !!

SET TERM ; !!

COMMIT;





/****************************************************************************
 *
 *	Create stored procedures.
 * 
*****************************************************************************/


SET TERM !! ;

/*
 *	Get employee's projects.
 *
 *	Parameters:
 *		employee number
 *	Returns:
 *		project id
 */

CREATE PROCEDURE get_emp_proj (emp_no SMALLINT)
RETURNS (proj_id CHAR(5)) AS
BEGIN
	FOR SELECT proj_id
		FROM employee_project
		WHERE emp_no = :emp_no
		INTO :proj_id
	DO
		SUSPEND;
END !!



/*
 *	Add an employee to a project.
 *
 *	Parameters:
 *		employee number
 *		project id
 *	Returns:
 *		--
 */

CREATE EXCEPTION unknown_emp_id 'Invalid employee number or project id.' !!

CREATE PROCEDURE add_emp_proj (emp_no SMALLINT, proj_id CHAR(5))  AS
BEGIN
	BEGIN
	INSERT INTO employee_project (emp_no, proj_id) VALUES (:emp_no, :proj_id);
	WHEN SQLCODE -530 DO
		EXCEPTION unknown_emp_id;
	END
	SUSPEND;
END !!



/*
 *	Select one row.
 *
 *	Compute total, average, smallest, and largest department budget.
 *
 *	Parameters:
 *		department id
 *	Returns:
 *		total budget
 *		average budget
 *		min budget
 *		max budget
 */

CREATE PROCEDURE sub_tot_budget (head_dept CHAR(3))
RETURNS (tot_budget DECIMAL(12, 2), avg_budget DECIMAL(12, 2),
	min_budget DECIMAL(12, 2), max_budget DECIMAL(12, 2))
AS
BEGIN
	SELECT SUM(budget), AVG(budget), MIN(budget), MAX(budget)
		FROM department
		WHERE head_dept = :head_dept
		INTO :tot_budget, :avg_budget, :min_budget, :max_budget;
	SUSPEND;
END !!



/*
 *	Delete an employee.
 *
 *	Parameters:
 *		employee number
 *	Returns:
 *		--
 */

CREATE EXCEPTION reassign_sales
		'Reassign the sales records before deleting this employee.' !!

CREATE PROCEDURE delete_employee (emp_num INTEGER)
AS
	DECLARE VARIABLE any_sales INTEGER;
BEGIN
	any_sales = 0;

	/*
	 *	If there are any sales records referencing this employee,
	 *	can't delete the employee until the sales are re-assigned
	 *	to another employee or changed to NULL.
	 */
	SELECT count(po_number)
	FROM sales
	WHERE sales_rep = :emp_num
	INTO :any_sales;

	IF (any_sales > 0) THEN
	BEGIN
		EXCEPTION reassign_sales;
		SUSPEND;
	END

	/*
	 *	If the employee is a manager, update the department.
	 */
	UPDATE department
	SET mngr_no = NULL
	WHERE mngr_no = :emp_num;

	/*
	 *	If the employee is a project leader, update project.
	 */
	UPDATE project
	SET team_leader = NULL
	WHERE team_leader = :emp_num;

	/*
	 *	Delete the employee from any projects.
	 */
	DELETE FROM employee_project
	WHERE emp_no = :emp_num;

	/*
	 *	Delete old salary records.
	 */
	DELETE FROM salary_history
	WHERE emp_no = :emp_num;

	/*
	 *	Delete the employee.
	 */
	DELETE FROM employee
	WHERE emp_no = :emp_num;

	SUSPEND;
END !!



/*
 *	Recursive procedure.
 *
 *	Compute the sum of all budgets for a department and all the
 *	departments under it.
 *
 *	Parameters:
 *		department id
 *	Returns:
 *		total budget
 */

CREATE PROCEDURE dept_budget (dno CHAR(3))
RETURNS (tot decimal(12,2)) AS
	DECLARE VARIABLE sumb DECIMAL(12, 2);
	DECLARE VARIABLE rdno CHAR(3);
	DECLARE VARIABLE cnt INTEGER;
BEGIN
	tot = 0;

	SELECT budget FROM department WHERE dept_no = :dno INTO :tot;

	SELECT count(budget) FROM department WHERE head_dept = :dno INTO :cnt;

	IF (cnt = 0) THEN
		SUSPEND;

	FOR SELECT dept_no
		FROM department
		WHERE head_dept = :dno
		INTO :rdno
	DO
		BEGIN
			EXECUTE PROCEDURE dept_budget :rdno RETURNING_VALUES :sumb;
			tot = tot + sumb;
		END

	SUSPEND;
END !!



/*
 *	Display an org-chart.
 *
 *	Parameters:
 *		--
 *	Returns:
 *		parent department
 *		department name
 *		department manager
 *		manager's job title
 *		number of employees in the department
 */

CREATE PROCEDURE org_chart
RETURNS (head_dept CHAR(25), department CHAR(25),
		mngr_name CHAR(20), title CHAR(5), emp_cnt INTEGER)
AS
	DECLARE VARIABLE mngr_no INTEGER;
	DECLARE VARIABLE dno CHAR(3);
BEGIN
	FOR SELECT h.department, d.department, d.mngr_no, d.dept_no
		FROM department d
		LEFT OUTER JOIN department h ON d.head_dept = h.dept_no
		ORDER BY d.dept_no
		INTO :head_dept, :department, :mngr_no, :dno
	DO
	BEGIN
		IF (:mngr_no IS NULL) THEN
		BEGIN
			mngr_name = '--TBH--';
			title = '';
		END

		ELSE
			SELECT full_name, job_code
			FROM employee
			WHERE emp_no = :mngr_no
			INTO :mngr_name, :title;

		SELECT COUNT(emp_no)
		FROM employee
		WHERE dept_no = :dno
		INTO :emp_cnt;

		SUSPEND;
	END
END !!



/*
 *	Generate a 6-line mailing label for a customer.
 *	Some of the lines may be blank.
 *
 *	Parameters:
 *		customer number
 *	Returns:
 *		6 address lines
 */

CREATE PROCEDURE mail_label (cust_no INTEGER)
RETURNS (line1 CHAR(40), line2 CHAR(40), line3 CHAR(40),
		line4 CHAR(40), line5 CHAR(40), line6 CHAR(40))
AS
	DECLARE VARIABLE customer	VARCHAR(25);
	DECLARE VARIABLE first_name		VARCHAR(15);
	DECLARE VARIABLE last_name		VARCHAR(20);
	DECLARE VARIABLE addr1		VARCHAR(30);
	DECLARE VARIABLE addr2		VARCHAR(30);
	DECLARE VARIABLE city		VARCHAR(25);
	DECLARE VARIABLE state		VARCHAR(15);
	DECLARE VARIABLE country	VARCHAR(15);
	DECLARE VARIABLE postcode	VARCHAR(12);
	DECLARE VARIABLE cnt		INTEGER;
BEGIN
	line1 = '';
	line2 = '';
	line3 = '';
	line4 = '';
	line5 = '';
	line6 = '';

	SELECT customer, contact_first, contact_last, address_line1,
		address_line2, city, state_province, country, postal_code
	FROM CUSTOMER
	WHERE cust_no = :cust_no
	INTO :customer, :first_name, :last_name, :addr1, :addr2,
		:city, :state, :country, :postcode;

	IF (customer IS NOT NULL) THEN
		line1 = customer;
	IF (first_name IS NOT NULL) THEN
		line2 = first_name || ' ' || last_name;
	ELSE
		line2 = last_name;
	IF (addr1 IS NOT NULL) THEN
		line3 = addr1;
	IF (addr2 IS NOT NULL) THEN
		line4 = addr2;

	IF (country = 'USA') THEN
	BEGIN
		IF (city IS NOT NULL) THEN
			line5 = city || ', ' || state || '  ' || postcode;
		ELSE
			line5 = state || '  ' || postcode;
	END
	ELSE
	BEGIN
		IF (city IS NOT NULL) THEN
			line5 = city || ', ' || state;
		ELSE
			line5 = state;
		line6 = country || '    ' || postcode;
	END

	SUSPEND;
END !!



/*
 *	Ship a sales order.
 *	First, check if the order is already shipped, if the customer
 *	is on hold, or if the customer has an overdue balance.
 *
 *	Parameters:
 *		purchase order number
 *	Returns:
 *		--
 *
 */

CREATE EXCEPTION order_already_shipped 'Order status is "shipped."' !!
CREATE EXCEPTION customer_on_hold 'This customer is on hold.' !!
CREATE EXCEPTION customer_check 'Overdue balance -- can not ship.' !!

CREATE PROCEDURE ship_order (po_num CHAR(8))
AS
	DECLARE VARIABLE ord_stat CHAR(7);
	DECLARE VARIABLE hold_stat CHAR(1);
	DECLARE VARIABLE cust_no INTEGER;
	DECLARE VARIABLE any_po CHAR(8);
BEGIN
	SELECT s.order_status, c.on_hold, c.cust_no
	FROM sales s, customer c
	WHERE po_number = :po_num
	AND s.cust_no = c.cust_no
	INTO :ord_stat, :hold_stat, :cust_no;

	/* This purchase order has been already shipped. */
	IF (ord_stat = 'shipped') THEN
	BEGIN
		EXCEPTION order_already_shipped;
		SUSPEND;
	END

	/*	Customer is on hold. */
	ELSE IF (hold_stat = '*') THEN
	BEGIN
		EXCEPTION customer_on_hold;
		SUSPEND;
	END

	/*
	 *	If there is an unpaid balance on orders shipped over 2 months ago,
	 *	put the customer on hold.
	 */
	FOR SELECT po_number
		FROM sales
		WHERE cust_no = :cust_no
		AND order_status = 'shipped'
		AND paid = 'n'
		AND ship_date < CAST('NOW' AS TIMESTAMP) - 60
		INTO :any_po
	DO
	BEGIN
		EXCEPTION customer_check;

		UPDATE customer
		SET on_hold = '*'
		WHERE cust_no = :cust_no;

		SUSPEND;
	END

	/*
	 *	Ship the order.
	 */
	UPDATE sales
	SET order_status = 'shipped', ship_date = 'NOW' 
	WHERE po_number = :po_num;

	SUSPEND;
END !!


CREATE PROCEDURE show_langs (code VARCHAR(5), grade SMALLINT, cty VARCHAR(15))
  RETURNS (languages VARCHAR(15))
AS
DECLARE VARIABLE i INTEGER;
BEGIN
  i = 1;
  WHILE (i <= 5) DO
  BEGIN
    SELECT language_req[:i] FROM joB
    WHERE ((job_code = :code) AND (job_grade = :grade) AND (job_country = :cty)
           AND (language_req IS NOT NULL))
    INTO :languages;
    IF (languages = ' ') THEN  /* Prints 'NULL' instead of blanks */
       languages = 'NULL';         
    i = i +1;
    SUSPEND;
  END
END!!



CREATE PROCEDURE all_langs RETURNS 
    (code VARCHAR(5), grade VARCHAR(5), 
     country VARCHAR(15), LANG VARCHAR(15)) AS
    BEGIN
	FOR SELECT job_code, job_grade, job_country FROM job 
		INTO :code, :grade, :country

	DO
	BEGIN
	    FOR SELECT languages FROM show_langs 
 		    (:code, :grade, :country) INTO :lang DO
	        SUSPEND;
	    /* Put nice separators between rows */
	    code = '=====';
	    grade = '=====';
	    country = '===============';
	    lang = '==============';
	    SUSPEND;
	END
    END!!

SET TERM ; !!
/*******************************************************************
 * Set of procedures to demonstrate sorts based on different
 * collations.
*******************************************************************/
SET TERM !! ;

CREATE PROCEDURE FRENCH_CUST_SORT
RETURNS (customer VARCHAR(40), city VARCHAR(25), country VARCHAR(15))
AS
BEGIN
        FOR SELECT customer, city, country 
	    FROM customer ORDER BY customer
            INTO :customer, :city, :country
        DO
           SUSPEND;

END !!

CREATE PROCEDURE GERMAN_CUST_SORT
RETURNS (customer VARCHAR(40), city VARCHAR(25), country VARCHAR(15))
AS
BEGIN
        FOR SELECT customer, city, country 
	    FROM customer ORDER BY customer COLLATE DE_DE
            INTO :customer, :city, :country
        DO
           SUSPEND;

END !!

CREATE PROCEDURE NORWAY_CUST_SORT
RETURNS (customer VARCHAR(40), city VARCHAR(25), country VARCHAR(15))
AS
BEGIN
        FOR SELECT customer, city, country 
	    FROM customer ORDER BY customer COLLATE NO_NO
            INTO :customer, :city, :country
        DO
           SUSPEND;

END !!

SET TERM ; !!

/*--------------------------------------------------------------
**			Function definitions for example database
**   Functions not supported for international, maybe in next build
**--------------------------------------------------------------
*/

/* Privileges */

GRANT ALL PRIVILEGES ON country TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON job TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON department TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON employee TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON phone_list TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON project TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON employee_project TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON proj_dept_budget TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON salary_history TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON customer TO PUBLIC WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON sales TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE get_emp_proj TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE add_emp_proj TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE sub_tot_budget TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE delete_employee TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE dept_budget TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE org_chart TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE mail_label TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE ship_order TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE show_langs TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE all_langs TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE french_cust_sort TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE german_cust_sort TO PUBLIC WITH GRANT OPTION;
GRANT EXECUTE ON PROCEDURE norway_cust_sort TO PUBLIC WITH GRANT OPTION;


