-- HW#2
-- Section x
-- Student ID: xxxxxxxxx
-- Last Name, First Name
-- In order to run this sql script with the ^ as your line delimiter, use the follow command options
-- db2 -td"^" -f trigger_sample.sql

connect to cs157b^
drop TRIGGER h2.check_dup^
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


--Creating trigger for checking duplicate key value.
CREATE TRIGGER h2.check_dup
BEFORE INSERT ON h2.customer
REFERENCING NEW AS newrow  
FOR EACH ROW MODE DB2SQL
WHEN ( (SELECT COUNT(*)
              FROM h2.customer 
              WHERE h2.customer.ID = newrow.ID ) >= 1 ) 
BEGIN ATOMIC
       DECLARE ID_count int;
       DECLARE prereq_pass int;

       SET ID_count = (SELECT COUNT(*)
                            FROM h2.customer 
                            WHERE h2.customer.ID = newrow.ID );

       IF ( ID_count >= 1 ) 
       THEN   SIGNAL SQLSTATE '88888' ( 'Duplicate Customer ID' );
       END IF;
END^

-- Inserting good data
insert into h2.customer_C values (100, 'Customer_1', 'M')^
insert into h2.customer_C values (101, 'Customer_2', 'F')^
insert into h2.customer_C values (102, 'Customer_3', 'F')^
insert into h2.customer_C values (103, 'Customer_4', 'M')^
insert into h2.customer_C values (104, 'Customer_5', 'F')^
insert into h2.account_C values (1000, 100, 10000, 'S')^
insert into h2.account_C values (1001, 100, 1000, 'C')^
insert into h2.account_C values (1002, 101, 2000, 'S')^
insert into h2.account_C values (1003, 101, 3000, 'C')^
insert into h2.account_C values (1004, 102, 4000, 'C')^
insert into h2.account_C values (1005, 103, 5000, 'S')^
insert into h2.account_C values (1006, 104, 6000, 'S')^
insert into h2.account_C (Number, cust_ID, Type) values (1007, 104, 'C')^
-- Check the content of the tables
select * from h2.customer_C^
select * from h2.account_C^
-- Invalid Customer ID
insert into h2.account_C values (1008, 105, 8000, 'S')^
-- Duplicate Customer ID
insert into h2.customer_C values (100, 'Customer_6', 'M')^
-- Null Customer ID
insert into h2.customer_C values (NULL, 'Customer_7', 'M')^
-- Null Name
insert into h2.customer_C values (105, NULL, 'F')^
-- Null Gender
insert into h2.customer_C values (108, 'Customer_8', NULL)^
-- Invalid Gender
insert into h2.customer_C values (109, 'Customer_9', 'Z')^
-- Duplicate Account Number
insert into h2.account_C values (1000, 100, 1000, 'C')^
-- NULL Account Number
insert into h2.account_C values (NULL, 100, 1000, 'C')^
-- Invalid Customer ID
insert into h2.account_C values (2000, 200, 10000, 'S')^
-- NULL Customer ID
insert into h2.account_C values (3000, NULL, 10000, 'S')^
-- NULL Account Type
insert into h2.account_C values (4000, 100, 10000, NULL)^
-- Invalid Account Type
insert into h2.account_C values (5000, 100, 10000, 'Z')^
-- Deleting a parent row (violating the RI constraint)
delete from h2.customer_C where ID = 104^
-- Testing the sample trigger above
insert into h2.customer values (100, 'Customer_1', 'M')^
-- Duplicate Customer ID
insert into h2.customer values (100, 'Customer_6', 'M')^
-- Check the content of the tables
select * from h2.customer^