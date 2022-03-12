-- Inserting good data
insert into h2.customer values (100, 'Customer_1', 'M')^
insert into h2.customer values (101, 'Customer_2', 'F')^
insert into h2.customer values (102, 'Customer_3', 'F')^
insert into h2.customer values (103, 'Customer_4', 'M')^
insert into h2.customer values (104, 'Customer_5', 'F')^
insert into h2.account values (1000, 100, 10000, 'S')^
insert into h2.account values (1001, 100, 1000, 'C')^
insert into h2.account values (1002, 101, 2000, 'S')^
insert into h2.account values (1003, 101, 3000, 'C')^
insert into h2.account values (1004, 102, 4000, 'C')^
insert into h2.account values (1005, 103, 5000, 'S')^
insert into h2.account values (1006, 104, 6000, 'S')^
insert into h2.account (Number, cust_ID, Type) values (1007, 104, 'C')^
-- Check the content of the tables
select * from h2.customer^
select * from h2.account^

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

-- -- Invalid Customer ID
insert into h2.account_C values (1008, 105, 8000, 'S')^
insert into h2.account values (1008, 105, 8000, 'S')^


-- Duplicate Customer ID
insert into h2.customer_C values (100, 'Customer_6', 'M')^
insert into h2.customer values (100, 'Customer_6', 'M')^

-- Null Customer ID
insert into h2.customer_C values (NULL, 'Customer_7', 'M')^
insert into h2.customer values (NULL, 'Customer_7', 'M')^

update h2.customer_C set id=NULL where name='Customer_1'^
update h2.customer set id=NULL where name='Customer_1'^

-- Null Name
insert into h2.customer_C values (105, NULL, 'F')^
insert into h2.customer values (105, NULL, 'F')^

update h2.customer_C set name=NULL where id=100^
update h2.customer set name=NULL where id=100^


-- Null Gender
insert into h2.customer_C values (108, 'Customer_8', NULL)^
insert into h2.customer values (108, 'Customer_8', NULL)^

update h2.customer_C set gender=NULL where id=100^
update h2.customer set gender=NULL where id=100^


-- Invalid Gender
insert into h2.customer_C values (109, 'Customer_9', 'Z')^
insert into h2.customer values (109, 'Customer_9', 'Z')^

update h2.customer_C set gender='Z' where id=100^
update h2.customer set gender='Z' where id=100^


-- Duplicate Account Number
insert into h2.account_C values (1000, 100, 1000, 'C')^
insert into h2.account values (1000, 100, 1000, 'C')^


-- NULL Account Number
insert into h2.account_C values (NULL, 100, 1000, 'C')^
insert into h2.account values (NULL, 100, 1000, 'C')^

update h2.account_C set number=NULL where cust_id=100^
update h2.account set number=NULL where cust_id=100^


-- Invalid Customer ID
insert into h2.account_C values (2000, 200, 10000, 'S')^
insert into h2.account values (2000, 200, 10000, 'S')^


-- NULL Customer ID
insert into h2.account_C values (3000, NULL, 10000, 'S')^
insert into h2.account values (3000, NULL, 10000, 'S')^

update h2.account_C set cust_id=NULL where number=1002^
update h2.account set cust_id=NULL where number=1002^


-- NULL Account Type
insert into h2.account_C values (4000, 100, 10000, NULL)^
insert into h2.account values (4000, 100, 10000, NULL)^

update h2.account_C set type=NULL where number=1000^
update h2.account set type=NULL where number=1000^


-- NULL Balance
insert into h2.account_C values (1008, 100, NULL, 'S')^
insert into h2.account values (1008, 100, NULL, 'S')^

update h2.account_C set balance=NULL where number=1000^
update h2.account set balance=NULL where number=1000^


-- Invalid Account Type
insert into h2.account_C values (5000, 100, 10000, 'Z')^
insert into h2.account values (5000, 100, 10000, 'Z')^

update h2.account_C set type='Z' where number=1000^
update h2.account set type='Z' where number=1000^


-- Deleting a parent row (violating the RI constraint)
delete from h2.customer_C where ID=100^
delete from h2.customer where ID=100^


-- Testing the sample trigger above - duplicate customer ID
insert into h2.customer_C values (100, 'Customer_1', 'M')^
insert into h2.customer values (100, 'Customer_1', 'M')^


-- Duplicate Customer ID
insert into h2.customer_C values (100, 'Customer_6', 'M')^
insert into h2.customer values (100, 'Customer_6', 'M')^


-- Negative balance 
insert into h2.customer_C values (111, 'Caitlyn', 'F')^
insert into h2.customer values (111, 'Caitlyn', 'F')^

insert into h2.account_C values (900, 111, 100, 'S')^
insert into h2.account values (900, 111, 100, 'S')^

insert into h2.account_C values (901, 111, 200, 'S')^
insert into h2.account values (901, 111, 200, 'S')^

update h2.account_C set balance=-100 where number=900^
update h2.account set balance=-100 where number=900^

-- should fail here
update h2.account_C set balance=-100 where number=901^
update h2.account set balance=-100 where number=901^


-- Updating a parent row (violating the RI constraint)
update h2.customer_C set ID=888 where name='Caitlyn'^
update h2.customer set ID=888 where name='Caitlyn'^


-- Check the content of the tables
select * from h2.customer_C^
select * from h2.customer^

select * from h2.account_C^
select * from h2.account^

