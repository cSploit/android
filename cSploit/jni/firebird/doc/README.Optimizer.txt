Optimizer enhancements in FB2.0

The Optimizer has two different optimizer "modules" depending on the ODS of the 
database. The reason for this is that in ODS 11 segment-selectivity is introduced.


Improvements for every ODS:

A) Deliver allowable comparisons from HAVING clause to WHERE clause so that indexes 
   could be used. Only simple comparisons are checked if they are deliverable. At 
   least 1 side of the comparison should be a field contained in the GROUP BY clause.
   Note that WHERE clauses on aggregate views are also put in the HAVING clause. 

   SELECT Count(*) FROM RelationCategories GROUP BY CategoryID HAVING CategoryID = 5

B) Try to deliver conjuntions made on a UNION inside every single stream.

   SELECT Count(*) FROM
    (SELECT RelationID FROM Relations
     UNION ALL
     SELECT CategoryID FROM Categories) dt (ID)
   WHERE
     dt.ID = 1

C) Cardinality calculation has been improved for a better estimated value.

D) MERGE/SORT operation may now be generated for inner joins using equality 
   comparison on expressions.

   SELECT Count(*) FROM
     Relations r
     JOIN RelationCategories rc ON (rc.RelationID + 0 = r.RelationID + 0)
   WHERE
     rc.CategoryID = 3

E) When a UNIQUE index used on a relation can be fully matched (equality), use this 
   index and forget any possible indexes.

F) Indexed order read with an OUTER JOIN.

   SELECT * FROM
     Relations r
     LEFT JOIN RelationCategories rc ON (rc.RelationID = r.RelationID and rc.CategoryID = 3)
   ORDER BY
     r.LASTNAME

  Note: Patch provided by <Radek Palmowski>


Improvements for ODS 11:

G) Ignore NULL values in index scan for operators other than IS NULL and Equivalent.
   In previous versions with a NULL-able index NULL-keys could be put in the bitmap 
   for record retrieval.

   Note: This works currently only at single segment indexes.


H) Calculate with more accuracy selectivity for compound indexes which are partially 
   matched with conjunctions. Due this we can now use cost-based approach also for 
   partially matched indexes. 

   Example:
   SELECT * FROM RelationCategories WHERE CategoryID = 1

I) IS NULL can be used inside compound indexes on every segment (assuming previous 
   segments are matched too).

   Example:
   SELECT * FROM Relations WHERE LastName = 'Goebel' and MiddleName IS NULL

J) STARTING WITH can be used inside compound indexes on every segment, but only the 
   last matching segment.

   Example:
   SELECT * FROM Relations WHERE FirstName = 'Joe' and LastName STARTING WITH 'D'

K) Use AND conjunctions also inside OR conjunctions to help with matching on a 
   compound index. The same calculation for every left and right node of an OR 
   conjunction is used as for the AND conjunctions.

   Example:
   SELECT * FROM RelationCategories WHERE CategoryID = 3 and RelationID IN (123, 720, 708)

   Above select will use 3 times the index created for the primary key.

L) JOIN order calculation doesn't prefer primary indexes, but uses cost based 
   calculation for every relation (selectivity, cardinality and number of indexes).
   Selectivity for a node is calculated over all selectivities used in that node.
   Indexes matched with IS NULL and STARTING WITH are now also considered in this 
   calculation. Less JOIN order combinations are being calculated.

   SELECT * FROM 
     Relations r
     JOIN RelationCategories rc ON (rc.RelationID = r.RelationID and rc.CATEGORYID = 1)
   WHERE
     r.FIRSTNAME LIKE 'J%'


Example metadata:

CREATE TABLE Relations (
  RelationID INT NOT NULL PRIMARY KEY,
  FirstName VARCHAR(35),
  MiddleName VARCHAR(35),
  LastName VARCHAR(35)
);

CREATE INDEX IDX_FIRSTNAME ON Relations (FirstName);
CREATE INDEX IDX_LASTNAME ON Relations (LastName);
CREATE INDEX IDX_FIRST_LASTNAME ON Relations (FirstName, LastName);
CREATE INDEX IDX_LAST_MIDDLENAME ON Relations (MiddleName, LastName);

CREATE TABLE Categories (
  CategoryID INT NOT NULL PRIMARY KEY,
  Description VARCHAR(30)
);

CREATE TABLE RelationCategories (
  RelationID INT NOT NULL,
  CategoryID INT NOT NULL,
  CONSTRAINT FK_RC_RELATIONS FOREIGN KEY (RelationID)
    REFERENCES Relations (RelationID),
  CONSTRAINT FK_RC_CATEGORIES FOREIGN KEY (CategoryID)
    REFERENCES Categories (CategoryID),
  PRIMARY KEY (CategoryID, RelationID)
);




