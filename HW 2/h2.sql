-- HW#2
-- Section 1
-- Student ID: 012662273
-- Chau, Caitlyn
-- In order to run this sql script with the ^ as your line delimiter, use the follow command options
-- db2 -td"^" -f trigger_sample.sql

connect to cs157b^
drop TRIGGER h2.cust_check_dup^
drop TRIGGER h2.acc_check_dup^
drop TRIGGER h2.cust_insert_null^
drop TRIGGER h2.cust_update_null^
drop TRIGGER h2.acc_insert_null^
drop TRIGGER h2.acc_update_null^
drop TRIGGER h2.cust_insert_gender^
drop TRIGGER h2.cust_update_gender^
drop TRIGGER h2.acc_insert_type^
drop TRIGGER h2.acc_update_type^
drop TRIGGER h2.ri_insert_cust^
drop TRIGGER h2.ri_update_cust^
drop TRIGGER h2.ri_parent_update^
drop TRIGGER h2.ri_parent_delete^
drop TRIGGER h2.negative_balance^

drop table h2.customer_C^
drop table h2.customer^
drop table h2.account_C^
drop table h2.account^

--Creating customer table with constraints
CREATE TABLE h2.customer_C (
  ID     integer not null,
  Name   varchar (15) not null,
  Gender char not null check (Gender in ('M', 'F')),
  primary key (ID)
)^

--Creating account table with contraints
CREATE TABLE h2.account_C (
  Number integer not null, 
  cust_ID integer not null references h2.customer_C (ID),
  Balance integer not null default 0,
  Type   char not null check (Type in ('S', 'C')),
  primary key (Number)
)^

--Creating customer table without constraints
CREATE TABLE h2.customer (
  ID     integer,
  Name   varchar (15),
  Gender char
)^

--Creating account table without contraints
CREATE TABLE h2.account (
  Number integer, 
  cust_ID integer,
  Balance integer,
  Type   char
)^


--  Check for duplicate primary keys in customer
CREATE TRIGGER h2.cust_check_dup
BEFORE INSERT ON h2.customer
REFERENCING NEW AS newrow  
FOR EACH ROW MODE DB2SQL
WHEN ( (SELECT COUNT(*)
  FROM h2.customer 
  WHERE h2.customer.ID = newrow.ID ) >= 1 ) 
BEGIN ATOMIC
  DECLARE ID_count int;

  SET ID_count = (SELECT COUNT(*)
    FROM h2.customer 
    WHERE h2.customer.ID = newrow.ID );

  IF ( ID_count >= 1 ) 
  THEN   SIGNAL SQLSTATE '88888' ( 'Primary Key Violated: Duplicate Customer ID' );
  END IF;
END^

--  Check for duplicate primary keys in account
CREATE TRIGGER h2.acc_check_dup
BEFORE INSERT ON h2.account
REFERENCING NEW AS newrow  
FOR EACH ROW MODE DB2SQL
WHEN ( (SELECT COUNT(*)
  FROM h2.account 
  WHERE h2.account.Number = newrow.Number ) >= 1 ) 
BEGIN ATOMIC
  DECLARE num_count int;

  SET num_count = (SELECT COUNT(*)
    FROM h2.account 
    WHERE h2.account.Number = newrow.Number );

  IF ( num_count >= 1 ) 
  THEN   SIGNAL SQLSTATE '88888' ( 'Primary Key Violated: Duplicate Account Number' );
  END IF;
END^

-- Triggers when null value inserted into customer
CREATE TRIGGER h2.cust_insert_null
BEFORE INSERT ON h2.customer
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL 
BEGIN ATOMIC
  IF newrow.ID IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Customer ID');
  END IF;
  IF newrow.NAME IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Name');
  END IF;
  IF newrow.GENDER IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Gender');
  END IF;
END^

-- Triggers when null value is updated in customer
CREATE TRIGGER h2.cust_update_null
NO CASCADE BEFORE UPDATE ON h2.customer
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL 
BEGIN ATOMIC
  IF newrow.ID IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Customer ID');
  END IF;
  IF newrow.NAME IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Name');
  END IF;
  IF newrow.GENDER IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Gender');
  END IF;
END^

-- Triggers when null value is inserted in account
CREATE TRIGGER h2.acc_insert_null
NO CASCADE BEFORE INSERT ON h2.account
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL 
BEGIN ATOMIC
  IF newrow.NUMBER IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Account Number');
  END IF;
  IF newrow.CUST_ID IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Customer ID');
  END IF;
  IF newrow.BALANCE IS NULL THEN
    SET newrow.BALANCE = 0;
  END IF;
  IF newrow.TYPE IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Type');
  END IF;
END^

-- Triggers when null value is updated in account
CREATE TRIGGER h2.acc_update_null
NO CASCADE BEFORE UPDATE ON h2.account
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL 
BEGIN ATOMIC
  IF newrow.NUMBER IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Account Number');
  END IF;
  IF newrow.CUST_ID IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Customer ID');
  END IF;
  IF newrow.BALANCE IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Balance');
  END IF;
  IF newrow.TYPE IS NULL THEN
    SIGNAL SQLSTATE '88888' ('Null Type');
  END IF;
END^

-- Check constraint for customer's Gender. 
CREATE TRIGGER h2.cust_insert_gender
NO CASCADE BEFORE INSERT ON h2.customer
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL 
BEGIN ATOMIC
  IF newrow.GENDER != 'M' AND newrow.GENDER != 'F' THEN
    SIGNAL SQLSTATE '88888' ('Invalid Gender');
  END IF;
END^

CREATE TRIGGER h2.cust_update_gender
NO CASCADE BEFORE UPDATE ON h2.customer
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL 
BEGIN ATOMIC
  IF newrow.GENDER != 'M' AND newrow.GENDER != 'F' THEN
    SIGNAL SQLSTATE '88888' ('Invalid Gender');
  END IF;
END^

-- Check constraint for account's type. 
CREATE TRIGGER h2.acc_insert_type
NO CASCADE BEFORE INSERT ON h2.account
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL 
BEGIN ATOMIC
  IF newrow.type != 'S' AND newrow.type != 'C' THEN
    SIGNAL SQLSTATE '88888' ('Invalid Account Type');
  END IF;
END^

CREATE TRIGGER h2.acc_update_type
NO CASCADE BEFORE UPDATE ON h2.account
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL 
BEGIN ATOMIC
  IF newrow.type != 'S' AND newrow.type != 'C' THEN
    SIGNAL SQLSTATE '88888' ('Invalid Account Type');
  END IF;
END^

-- RI Constraint ID <-> cust_ID
CREATE TRIGGER h2.ri_insert_cust
NO CASCADE BEFORE INSERT ON h2.account
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL
WHEN ( (SELECT COUNT(*)
  FROM h2.customer 
  WHERE h2.customer.ID = newrow.cust_id ) != 1 ) 
BEGIN ATOMIC
  DECLARE cust_count int;

  SET cust_count = (SELECT COUNT(*)
    FROM h2.customer 
    WHERE h2.customer.ID = newrow.cust_id );

  IF ( cust_count != 1 ) 
  THEN   SIGNAL SQLSTATE '88888' ( 'RI Violation - Customer ID does not exist' );
  END IF;
END^

CREATE TRIGGER h2.ri_update_cust
NO CASCADE BEFORE UPDATE ON h2.account
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL
WHEN ( (SELECT COUNT(*)
  FROM h2.customer 
  WHERE h2.customer.ID = newrow.cust_id ) != 1 ) 
BEGIN ATOMIC
  DECLARE cust_count int;

  SET cust_count = (SELECT COUNT(*)
    FROM h2.customer 
    WHERE h2.customer.ID = newrow.cust_id );

  IF ( cust_count != 1 ) 
  THEN   SIGNAL SQLSTATE '88888' ( 'RI Violation - Customer ID does not exist' );
  END IF;
END^

-- Disallow customer update if there are accounts depending on it
CREATE TRIGGER h2.ri_parent_update
BEFORE UPDATE ON h2.customer
REFERENCING OLD as oldrow
FOR EACH ROW MODE DB2SQL
WHEN ( (SELECT COUNT(*)
  FROM h2.account 
  WHERE h2.account.cust_id = oldrow.id ) > 0 ) 
BEGIN ATOMIC
  DECLARE accounts_count int;

  SET accounts_count = (select count(*) from h2.account where h2.account.cust_id = oldrow.id);
  IF (accounts_count > 0) THEN 
    SIGNAL SQLSTATE '88888' ( 'RI Violation - Cannot update: At least one account associated with this customer' );
  END IF;
END^

-- Disallow delete customer if there are accounts associated with ID
CREATE TRIGGER h2.ri_parent_delete
BEFORE DELETE ON h2.customer
REFERENCING OLD as oldrow
FOR EACH ROW MODE DB2SQL
WHEN ( (SELECT COUNT(*)
  FROM h2.account 
  WHERE h2.account.cust_id = oldrow.id ) > 0 ) 
BEGIN ATOMIC
  DECLARE accounts_count int;

  SET accounts_count = (select count(*) from h2.account where h2.account.cust_id = oldrow.id);
  IF (accounts_count > 0) THEN 
    SIGNAL SQLSTATE '88888' ( 'RI Violation - Cannot delete: At least one account associated with this customer' );
  END IF;
END^

-- Disallow negative account balance iff total balance for all accounts < 0
CREATE TRIGGER h2.negative_balance
AFTER UPDATE ON h2.account
REFERENCING NEW as newrow
FOR EACH ROW MODE DB2SQL
WHEN ( (SELECT SUM(balance) FROM h2.account WHERE h2.account.cust_id = newrow.cust_id ) < 0 ) 
BEGIN ATOMIC
  DECLARE total_balance int;

  SET total_balance = (SELECT SUM(balance) FROM h2.account WHERE h2.account.cust_id = newrow.cust_id );
  IF (total_balance < 0) THEN 
    SIGNAL SQLSTATE '88888' ( 'Total sum of all balances cannot be negative' );
  END IF;
END^ 