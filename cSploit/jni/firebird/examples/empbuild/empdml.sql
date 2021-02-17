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
/****************************************************************************
 *
 *  Create data.
 * 
*****************************************************************************/

/*
 *  Add countries.
 */
INSERT INTO country (country, currency) VALUES ('USA',         'Dollar');
INSERT INTO country (country, currency) VALUES ('England',     'Pound'); 
INSERT INTO country (country, currency) VALUES ('Canada',      'CdnDlr');
INSERT INTO country (country, currency) VALUES ('Switzerland', 'SFranc');
INSERT INTO country (country, currency) VALUES ('Japan',       'Yen');
INSERT INTO country (country, currency) VALUES ('Italy',       'Euro');
INSERT INTO country (country, currency) VALUES ('France',      'Euro');
INSERT INTO country (country, currency) VALUES ('Germany',     'Euro');
INSERT INTO country (country, currency) VALUES ('Australia',   'ADollar');
INSERT INTO country (country, currency) VALUES ('Hong Kong',   'HKDollar');
INSERT INTO country (country, currency) VALUES ('Netherlands', 'Euro');
INSERT INTO country (country, currency) VALUES ('Belgium',     'Euro');
INSERT INTO country (country, currency) VALUES ('Austria',     'Euro');
INSERT INTO country (country, currency) VALUES ('Fiji',        'FDollar');
INSERT INTO country (country, currency) VALUES ('Russia',      'Ruble');
INSERT INTO country (country, currency) VALUES ('Romania',     'RLeu');

COMMIT;

/*
 *  Add departments.
 *  Don't assign managers yet.
 *
 *  Department structure (4-levels):
 *
 *      Corporate Headquarters
 *          Finance
 *          Sales and Marketing
 *              Marketing
 *              Pacific Rim Headquarters (Hawaii)
 *                  Field Office: Tokyo
 *                  Field Office: Singapore
 *              European Headquarters (London)
 *                  Field Office: France
 *                  Field Office: Italy
 *                  Field Office: Switzerland
 *              Field Office: Canada
 *              Field Office: East Coast
 *          Engineering
 *              Software Products Division (California)
 *                  Software Development
 *                  Quality Assurance
 *                  Customer Support
 *              Consumer Electronics Division (Vermont)
 *                  Research and Development
 *                  Customer Services
 *
 *  Departments have parent departments.
 *  Corporate Headquarters is the top department in the company.
 *  Singapore field office is new and has 0 employees.
 *
 */
INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('000', 'Corporate Headquarters', null, 1000000, 'Monterey','(408) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('100', 'Sales and Marketing',  '000', 2000000, 'San Francisco',
'(415) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('600', 'Engineering', '000', 1100000, 'Monterey', '(408) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('900', 'Finance',   '000', 400000, 'Monterey', '(408) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('180', 'Marketing', '100', 1500000, 'San Francisco', '(415) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('620', 'Software Products Div.', '600', 1200000, 'Monterey', '(408) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('621', 'Software Development', '620', 400000, 'Monterey', '(408) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('622', 'Quality Assurance',    '620', 300000, 'Monterey', '(408) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('623', 'Customer Support', '620', 650000, 'Monterey', '(408) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('670', 'Consumer Electronics Div.', '600', 1150000, 'Burlington, VT',
'(802) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('671', 'Research and Development', '670', 460000, 'Burlington, VT',
'(802) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('672', 'Customer Services', '670', 850000, 'Burlington, VT', '(802) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('130', 'Field Office: East Coast', '100', 500000, 'Boston', '(617) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('140', 'Field Office: Canada',     '100', 500000, 'Toronto', '(416) 677-1000');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('110', 'Pacific Rim Headquarters', '100', 600000, 'Kuaui', '(808) 555-1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('115', 'Field Office: Japan',      '110', 500000, 'Tokyo', '3 5350 0901');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('116', 'Field Office: Singapore',  '110', 300000, 'Singapore', '3 55 1234');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('120', 'European Headquarters',    '100', 700000, 'London', '71 235-4400');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('121', 'Field Office: Switzerland','120', 500000, 'Zurich', '1 211 7767');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('123', 'Field Office: France',     '120', 400000, 'Cannes', '58 68 11 12');

INSERT INTO department
(dept_no, department, head_dept, budget, location, phone_no) VALUES
('125', 'Field Office: Italy',      '120', 400000, 'Milan', '2 430 39 39');


COMMIT;

/*
 *  Add jobs.
 *  Job requirements (blob) and languages (array) are not added here.
 */

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('CEO',   1, 'USA', 'Chief Executive Officer',    130000, 250000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('CFO',   1, 'USA', 'Chief Financial Officer',    85000,  140000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('VP',    2, 'USA', 'Vice President',             80000,  130000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Dir',   2, 'USA', 'Director',                   75000,  120000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Mngr',  3, 'USA', 'Manager',                    60000,  100000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Mngr',  4, 'USA', 'Manager',                    30000,  60000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Admin', 4, 'USA', 'Administrative Assistant',   35000,  55000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Admin', 5, 'USA', 'Administrative Assistant',   20000,  40000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Admin', 5, 'England', 'Administrative Assistant', 13400, 26800) /* pounds */;

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('PRel',  4, 'USA', 'Public Relations Rep.',      25000,  65000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Mktg',  3, 'USA', 'Marketing Analyst',          40000,  80000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Mktg',  4, 'USA', 'Marketing Analyst',          20000,  50000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Accnt', 4, 'USA', 'Accountant',                 28000,  55000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Finan', 3, 'USA', 'Financial Analyst',          35000,  85000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Eng',   2, 'USA', 'Engineer',                   70000,  110000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Eng',   3, 'USA', 'Engineer',                   50000,  90000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Eng',   3, 'Japan', 'Engineer',                 5400000, 9720000) /* yen */;

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Eng',   4, 'USA', 'Engineer',                   30000,  65000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Eng',   4, 'England', 'Engineer',               20100,  43550) /* Pounds */;

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Eng',   5, 'USA', 'Engineer',                   25000,  35000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Doc',   3, 'USA', 'Technical Writer',           38000,  60000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Doc',   5, 'USA', 'Technical Writer',           22000,  40000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Sales', 3, 'USA', 'Sales Co-ordinator',         40000,  70000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('Sales', 3, 'England', 'Sales Co-ordinator',     26800,  46900) /* pounds */;

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('SRep',  4, 'USA', 'Sales Representative',       20000,  100000);

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('SRep',  4, 'England', 'Sales Representative',   13400,  67000) /* pounds */;

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('SRep',  4, 'Canada', 'Sales Representative', 26400,  132000) /* CndDollar */;

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('SRep',  4, 'Switzerland', 'Sales Representative', 28000, 149000) /* SFranc */;

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('SRep',  4, 'Japan', 'Sales Representative',   2160000, 10800000) /* yen */;

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('SRep',  4, 'Italy', 'Sales Representative',   20000, 100000) /* Euro */;

INSERT INTO job
(job_code, job_grade, job_country, job_title, min_salary, max_salary) VALUES
('SRep',  4, 'France', 'Sales Representative',  20000, 100000) /* Euro */;


COMMIT;

/*
 *  Add employees.
 *
 *  The salaries initialized here are not final.  Employee salaries are
 *  updated below -- see salary_history.
 */


INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(2, 'Robert', 'Nelson', '600', 'VP', 2, 'USA', '12/28/88', 98000, '250');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(4, 'Bruce', 'Young', '621', 'Eng', 2, 'USA', '12/28/88', 90000, '233');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(5, 'Kim', 'Lambert', '130', 'Eng', 2, 'USA', '02/06/89', 95000, '22');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(8, 'Leslie', 'Johnson', '180', 'Mktg',  3, 'USA', '04/05/89', 62000, '410');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(9, 'Phil', 'Forest',   '622', 'Mngr',  3, 'USA', '04/17/89', 72000, '229');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(11, 'K. J.', 'Weston', '130', 'SRep',  4, 'USA', '01/17/90', 70000, '34');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(12, 'Terri', 'Lee', '000', 'Admin', 4, 'USA', '05/01/90', 48000, '256');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(14, 'Stewart', 'Hall', '900', 'Finan', 3, 'USA', '06/04/90', 62000, '227');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(15, 'Katherine', 'Young', '623', 'Mngr',  3, 'USA', '06/14/90', 60000, '231');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(20, 'Chris', 'Papadopoulos', '671', 'Mngr', 3, 'USA', '01/01/90', 80000,
'887');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(24, 'Pete', 'Fisher', '671', 'Eng', 3, 'USA', '09/12/90', 73000, '888');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(28, 'Ann', 'Bennet', '120', 'Admin', 5, 'England', '02/01/91', 20000, '5');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(29, 'Roger', 'De Souza', '623', 'Eng', 3, 'USA', '02/18/91', 62000, '288');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(34, 'Janet', 'Baldwin', '110', 'Sales', 3, 'USA', '03/21/91', 55000, '2');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(36, 'Roger', 'Reeves', '120', 'Sales', 3, 'England', '04/25/91', 30000, '6');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(37, 'Willie', 'Stansbury','120', 'Eng', 4, 'England', '04/25/91', 35000, '7');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(44, 'Leslie', 'Phong', '623', 'Eng', 4, 'USA', '06/03/91', 50000, '216');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(45, 'Ashok', 'Ramanathan', '621', 'Eng', 3, 'USA', '08/01/91', 72000, '209');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(46, 'Walter', 'Steadman', '900', 'CFO', 1, 'USA', '08/09/91', 120000, '210');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(52, 'Carol', 'Nordstrom', '180', 'PRel',  4, 'USA', '10/02/91', 41000, '420');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(61, 'Luke', 'Leung', '110', 'SRep',  4, 'USA', '02/18/92', 60000, '3');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(65, 'Sue Anne','O''Brien', '670', 'Admin', 5, 'USA', '03/23/92', 30000, '877');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(71, 'Jennifer M.', 'Burbank', '622', 'Eng', 3, 'USA', '04/15/92', 51000,
'289');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(72, 'Claudia', 'Sutherland', '140', 'SRep', 4, 'Canada', '04/20/92', 88000,
null);

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(83, 'Dana', 'Bishop', '621', 'Eng',  3, 'USA', '06/01/92', 60000, '290');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(85, 'Mary S.', 'MacDonald', '100', 'VP', 2, 'USA', '06/01/92', 115000, '477');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(94, 'Randy', 'Williams', '672', 'Mngr', 4, 'USA', '08/08/92', 54000, '892');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(105, 'Oliver H.', 'Bender', '000', 'CEO', 1, 'USA', '10/08/92', 220000, '255');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(107, 'Kevin', 'Cook', '670', 'Dir', 2, 'USA', '02/01/93', 115000, '894');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(109, 'Kelly', 'Brown', '600', 'Admin', 5, 'USA', '02/04/93', 27000, '202');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(110, 'Yuki', 'Ichida', '115', 'Eng', 3, 'Japan', '02/04/93',
6000000, '22');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(113, 'Mary', 'Page', '671', 'Eng', 4, 'USA', '04/12/93', 48000, '845');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(114, 'Bill', 'Parker', '623', 'Eng', 5, 'USA', '06/01/93', 35000, '247');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(118, 'Takashi', 'Yamamoto', '115', 'SRep', 4, 'Japan', '07/01/93',
6800000, '23');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(121, 'Roberto', 'Ferrari', '125', 'SRep',  4, 'Italy', '07/12/93',
30000, '1');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(127, 'Michael', 'Yanowski', '100', 'SRep', 4, 'USA', '08/09/93', 40000, '492');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(134, 'Jacques', 'Glon', '123', 'SRep',  4, 'France', '08/23/93', 35000, null);

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(136, 'Scott', 'Johnson', '623', 'Doc', 3, 'USA', '09/13/93', 60000, '265');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(138, 'T.J.', 'Green', '621', 'Eng', 4, 'USA', '11/01/93', 36000, '218');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(141, 'Pierre', 'Osborne', '121', 'SRep', 4, 'Switzerland', '01/03/94',
110000, null);

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(144, 'John', 'Montgomery', '672', 'Eng',   5, 'USA', '03/30/94', 35000, '820');

INSERT INTO employee (emp_no, first_name, last_name, dept_no, job_code,
job_grade, job_country, hire_date, salary, phone_ext) VALUES
(145, 'Mark', 'Guckenheimer', '622', 'Eng', 5, 'USA', '05/02/94', 32000, '221');


COMMIT;

SET GENERATOR emp_no_gen to 145;


/*
 *  Set department managers.
 *  A department manager can be a director, a vice president, a CFO,
 *  a sales rep, etc.  Several departments have no managers (TBH).
 */
UPDATE department SET mngr_no = 105 WHERE dept_no = '000';
UPDATE department SET mngr_no = 85 WHERE dept_no = '100';
UPDATE department SET mngr_no = 2 WHERE dept_no = '600';
UPDATE department SET mngr_no = 46 WHERE dept_no = '900';
UPDATE department SET mngr_no = 9 WHERE dept_no = '622';
UPDATE department SET mngr_no = 15 WHERE dept_no = '623';
UPDATE department SET mngr_no = 107 WHERE dept_no = '670';
UPDATE department SET mngr_no = 20 WHERE dept_no = '671';
UPDATE department SET mngr_no = 94 WHERE dept_no = '672';
UPDATE department SET mngr_no = 11 WHERE dept_no = '130';
UPDATE department SET mngr_no = 72 WHERE dept_no = '140';
UPDATE department SET mngr_no = 118 WHERE dept_no = '115';
UPDATE department SET mngr_no = 36 WHERE dept_no = '120';
UPDATE department SET mngr_no = 141 WHERE dept_no = '121';
UPDATE department SET mngr_no = 134 WHERE dept_no = '123';
UPDATE department SET mngr_no = 121 WHERE dept_no = '125';
UPDATE department SET mngr_no = 34 WHERE dept_no = '110';


COMMIT;

/*
 *  Generate some salary history records.
 */

UPDATE employee SET salary = salary + salary * 0.10 
    WHERE hire_date <= '08/01/91' AND job_grade = 5;
UPDATE employee SET salary = salary + salary * 0.05 + 3000
    WHERE hire_date <= '08/01/91' AND job_grade in (1, 2);
UPDATE employee SET salary = salary + salary * 0.075
    WHERE hire_date <= '08/01/91' AND job_grade in (3, 4) AND emp_no > 9;
UPDATE salary_history
    SET change_date = '12/15/92', updater_id = 'admin2';

UPDATE employee SET salary = salary + salary * 0.0425
    WHERE hire_date < '02/01/93' AND job_grade >= 3;
UPDATE salary_history
    SET change_date = '09/08/93', updater_id = 'elaine'
    WHERE NOT updater_id IN ('admin2');

UPDATE employee SET salary = salary - salary * 0.0325
    WHERE salary > 110000 AND job_country = 'USA';
UPDATE salary_history
    SET change_date = '12/20/93', updater_id = 'tj'
    WHERE NOT updater_id IN ('admin2', 'elaine');

UPDATE employee SET salary = salary + salary * 0.10
    WHERE job_code = 'SRep' AND hire_date < '12/20/93';
UPDATE salary_history
    SET change_date = '12/20/93', updater_id = 'elaine'
    WHERE NOT updater_id IN ('admin2', 'elaine', 'tj');

COMMIT;


/*
 *  Add projects.
 *  Some projects have no team leader.
 */

INSERT INTO project (proj_id, proj_name, team_leader, product) VALUES
('VBASE', 'Video Database', 45, 'software');

        /* proj_desc blob:
                  Design a video data base management system for
                  controlling on-demand video distribution.
        */

INSERT INTO project (proj_id, proj_name, team_leader, product) VALUES
('DGPII', 'DigiPizza', 24, 'other');

        /* proj_desc blob:
                  Develop second generation digital pizza maker
                  with flash-bake heating element and
                  digital ingredient measuring system.
        */

INSERT INTO project (proj_id, proj_name, team_leader, product) VALUES
('GUIDE', 'AutoMap', 20, 'hardware');

        /* proj_desc blob:
                  Develop a prototype for the automobile version of
                  the hand-held map browsing device.
        */

INSERT INTO project (proj_id, proj_name, team_leader, product) VALUES
('MAPDB', 'MapBrowser port', 4, 'software');

        /* proj_desc blob:
                  Port the map browsing database software to run
                  on the automobile model.
        */

INSERT INTO project (proj_id, proj_name, team_leader, product) VALUES
('HWRII', 'Translator upgrade', null, 'software');

        /* proj_desc blob:
                  Integrate the hand-writing recognition module into the
                  universal language translator.
        */

INSERT INTO project (proj_id, proj_name, team_leader, product) VALUES
('MKTPR', 'Marketing project 3', 85, 'N/A');

        /* proj_desc blob:
                  Expand marketing and sales in the Pacific Rim.
                  Set up a field office in Australia and Singapore.
        */

COMMIT;

/*
 *  Assign employees to projects.
 *  One project has no employees assigned.
 */

INSERT INTO employee_project (proj_id, emp_no) VALUES ('DGPII', 144);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('DGPII', 113);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('DGPII', 24);

INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 8);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 136);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 15);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 71);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 145);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 44);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 4);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 83);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 138);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('VBASE', 45);

INSERT INTO employee_project (proj_id, emp_no) VALUES ('GUIDE', 20);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('GUIDE', 24);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('GUIDE', 113);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('GUIDE', 8);

INSERT INTO employee_project (proj_id, emp_no) VALUES ('MAPDB', 4);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('MAPDB', 71);

INSERT INTO employee_project (proj_id, emp_no) VALUES ('MKTPR', 46);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('MKTPR', 105);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('MKTPR', 12);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('MKTPR', 85);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('MKTPR', 110);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('MKTPR', 34);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('MKTPR', 8);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('MKTPR', 14);
INSERT INTO employee_project (proj_id, emp_no) VALUES ('MKTPR', 52);

COMMIT;

/*
 *  Add project budget planning by department.
 *  Head count array is not added here.
 */

INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'GUIDE', '100', 200000);
        /* head count:  1,1,1,0 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'GUIDE', '671', 450000);
        /* head count:  3,2,1,0 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1993, 'MAPDB', '621', 20000);
        /* head count:  0,0,0,1 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'MAPDB', '621', 40000);
        /* head count:  2,1,0,0 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'MAPDB', '622', 60000);
        /* head count:  1,1,0,0 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'MAPDB', '671', 11000);
        /* head count:  1,1,0,0 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'HWRII', '670', 20000);
        /* head count:  1,1,1,1 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'HWRII', '621', 400000);
        /* head count:  2,3,2,1 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'HWRII', '622', 100000);
        /* head count:  1,1,2,2 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'MKTPR', '623', 80000);
        /* head count:  1,1,1,2 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'MKTPR', '672', 100000);
        /* head count:  1,1,1,2 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'MKTPR', '100', 1000000);
        /* head count:  4,5,6,6 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'MKTPR', '110', 200000);
        /* head count:  2,2,0,3 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'MKTPR', '000', 100000);
        /* head count:  1,1,2,2 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1995, 'MKTPR', '623', 1200000);
        /* head count:  7,7,4,4 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1995, 'MKTPR', '672', 800000);
        /* head count:  2,3,3,3 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1995, 'MKTPR', '100', 2000000);
        /* head count:  4,5,6,6 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1995, 'MKTPR', '110', 1200000);
        /* head count:  1,1,1,1 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'VBASE', '621', 1900000);
        /* head count:  4,5,5,3 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1995, 'VBASE', '621', 900000);
        /* head count:  4,3,2,2 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'VBASE', '622', 400000);
        /* head count:  2,2,2,1 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1994, 'VBASE', '100', 300000);
        /* head count:  1,1,2,3 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1995, 'VBASE', '100', 1500000);
        /* head count:  3,3,1,1 */
INSERT INTO proj_dept_budget (fiscal_year, proj_id, dept_no, projected_budget) VALUES
(1996, 'VBASE', '100', 150000);
        /* head count:  1,1,0,0 */


COMMIT;
/*
 *  Add a few customer records.
 */


INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1001, 'Signature Design', 'Dale J.', 'Little', '(619) 530-2710',
'15500 Pacific Heights Blvd.', null, 'San Diego', 'CA', 'USA', '92121', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1002, 'Dallas Technologies', 'Glen', 'Brown', '(214) 960-2233',
'P. O. Box 47000', null, 'Dallas', 'TX', 'USA', '75205', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1003, 'Buttle, Griffith and Co.', 'James', 'Buttle', '(617) 488-1864',
'2300 Newbury Street', 'Suite 101', 'Boston', 'MA', 'USA', '02115', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1004, 'Central Bank', 'Elizabeth', 'Brocket', '61 211 99 88',
'66 Lloyd Street', null, 'Manchester', null, 'England', 'M2 3LA', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1005, 'DT Systems, LTD.', 'Tai', 'Wu', '(852) 850 43 98',
'400 Connaught Road', null, 'Central Hong Kong', null, 'Hong Kong', null, null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1006, 'DataServe International', 'Tomas', 'Bright', '(613) 229 3323',
'2000 Carling Avenue', 'Suite 150', 'Ottawa', 'ON', 'Canada', 'K1V 9G1', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1007, 'Mrs. Beauvais', null, 'Mrs. Beauvais', null,
'P.O. Box 22743', null, 'Pebble Beach', 'CA', 'USA', '93953', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1008, 'Anini Vacation Rentals', 'Leilani', 'Briggs', '(808) 835-7605',
'3320 Lawai Road', null, 'Lihue', 'HI', 'USA', '96766', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1009, 'Max', 'Max', null, '22 01 23',
'1 Emerald Cove', null, 'Turtle Island', null, 'Fiji', null, null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1010, 'MPM Corporation', 'Miwako', 'Miyamoto', '3 880 77 19',
'2-64-7 Sasazuka', null, 'Tokyo', null, 'Japan', '150', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1011, 'Dynamic Intelligence Corp', 'Victor', 'Granges', '01 221 16 50',
'Florhofgasse 10', null, 'Zurich', null, 'Switzerland', '8005', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1012, '3D-Pad Corp.', 'Michelle', 'Roche', '1 43 60 61',
'22 Place de la Concorde', null, 'Paris', null, 'France', '75008', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1013, 'Lorenzi Export, Ltd.', 'Andreas', 'Lorenzi', '02 404 6284',
'Via Eugenia, 15', null, 'Milan', null, 'Italy', '20124', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1014, 'Dyno Consulting', 'Greta', 'Hessels', '02 500 5940',
'Rue Royale 350', null, 'Brussels', null, 'Belgium', '1210', null);

INSERT INTO customer
(cust_no, customer, contact_first, contact_last, phone_no, address_line1,
address_line2, city, state_province, country, postal_code, on_hold) VALUES
(1015, 'GeoTech Inc.', 'K.M.', 'Neppelenbroek', '(070) 44 91 18',
'P.0.Box 702', null, 'Den Haag', null, 'Netherlands', '2514', null);

COMMIT;

SET GENERATOR cust_no_gen to 1015;



/*
 *  Add some sales records.
 */

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V91E0210', 1004, 11,  '03/04/91', '03/05/91', null,
'shipped',      'y',    10,     5000,   0.1,    'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V92E0340', 1004, 11, '10/15/92', '10/16/92', '10/17/92',
'shipped',      'y',    7,      70000,  0,      'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V92J1003', 1010, 61,  '07/26/92', '08/04/92', '09/15/92',
'shipped',      'y',    15,     2985,   0,      'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93J2004', 1010, 118, '10/30/93', '12/02/93', '11/15/93',
'shipped',      'y',    3,      210,    0,      'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93J3100', 1010, 118, '08/20/93', '08/20/93', null,
'shipped',      'y',    16,     18000.40,       0.10, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V92F3004', 1012, 11, '10/15/92', '01/16/93', '01/16/93',
'shipped',      'y',    3,      2000,           0, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93F3088', 1012, 134, '08/27/93', '09/08/93', null,
'shipped',      'n',    10,     10000,          0, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93F2030', 1012, 134, '12/12/93', null,    null,
'open',         'y',    15,     450000.49,      0, 'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93F2051', 1012, 134, '12/18/93', null, '03/01/94',
'waiting',      'n',    1,      999.98,         0, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93H0030', 1005, 118, '12/12/93', null, '01/01/94',
'open',         'y',    20,     5980,           0.20, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V94H0079', 1005, 61, '02/13/94', null, '04/20/94',
'open',         'n',    10,     9000,           0.05, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9324200', 1001, 72, '08/09/93', '08/09/93', '08/17/93',
'shipped',      'y',    1000,   560000, 0.20, 'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9324320', 1001, 127, '08/16/93', '08/16/93', '09/01/93',
'shipped',      'y',    1,              0,      1, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9320630', 1001, 127, '12/12/93', null, '12/15/93',
'open',         'n',    3,      60000,  0.20, 'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9420099', 1001, 127, '01/17/94', null, '06/01/94',
'open',         'n',    100, 3399.15,   0.15, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9427029', 1001, 127, '02/07/94', '02/10/94', '02/10/94',
'shipped',      'n',    17,     422210.97,      0, 'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9333005', 1002, 11, '02/03/93', '03/03/93', null,
'shipped',      'y',    2,      600.50,         0, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9333006', 1002, 11, '04/27/93', '05/02/93', '05/02/93',
'shipped',      'n',    5,      20000,          0, 'other');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9336100', 1002, 11, '12/27/93', '01/01/94', '01/01/94',
'waiting',      'n',    150,    14850,  0.05, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9346200', 1003, 11, '12/31/93', null, '01/24/94',
'waiting',      'n',    3,      0,      1,    'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9345200', 1003, 11, '11/11/93', '12/02/93', '12/01/93',
'shipped',      'y',    900,    27000,  0.30, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9345139', 1003, 127, '09/09/93', '09/20/93', '10/01/93',
'shipped',      'y',    20,     12582.12,       0.10, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93C0120', 1006, 72, '03/22/93', '05/31/93', '04/17/93',
'shipped',      'y',    1,      47.50,  0, 'other');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93C0990', 1006, 72, '08/09/93', '09/02/93', null,
'shipped',      'y',    40,     399960.50,      0.10, 'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V9456220', 1007, 127, '01/04/94', null, '01/30/94',
'open',         'y',    1,      3999.99,        0, 'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93S4702', 1011, 121, '10/27/93', '10/28/93', '12/15/93',
'shipped',      'y',    4,      120000,         0, 'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V94S6400', 1011, 141, '01/06/94', null, '02/15/94',
'waiting',      'y',    20,     1980.72,        0.40, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93H3009', 1008, 61, '08/01/93', '12/02/93', '12/01/93',
'shipped',      'n',    3,      9000,           0.05, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93H0500', 1008, 61, '12/12/93', null, '12/15/93',
'open',         'n',    3,      16000,          0.20, 'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93F0020', 1009, 61,  '10/10/93', '11/11/93', '11/11/93',
'shipped',      'n',    1,      490.69,         0, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93I4700', 1013, 121, '10/27/93', null, '12/15/93',
'open',         'n',    5,      2693,           0, 'hardware');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93B1002', 1014, 134, '09/20/93', '09/21/93', '09/25/93',
'shipped',      'y',    1,      100.02,         0, 'software');

INSERT INTO sales
(po_number, cust_no, sales_rep, order_date, ship_date, date_needed,
order_status, paid, qty_ordered, total_value, discount, item_type) VALUES
('V93N5822', 1015, 134, '12/18/93', '01/14/94', null,
'shipped',      'n',    2,      1500.00,        0, 'software');


COMMIT;
/*
 *  Put some customers on-hold.
 */

UPDATE customer SET on_hold = '*' WHERE cust_no = 1002;
UPDATE customer SET on_hold = '*' WHERE cust_no = 1009;

COMMIT;
