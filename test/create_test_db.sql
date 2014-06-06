create database test;
grant all privileges on test.* to test@'localhost' identified by 'test';
use test;
set names 'utf8';

create table addressbook (
	userid int not null auto_increment primary key,
	firstname varchar( 255 ),
	lastname varchar( 255 ),
	phone varchar( 40 ),
	address varchar( 255 ),
	city varchar( 255 ),
	zip varchar( 40 ),
	state varchar( 255 ),
	created datetime,
	mod_date date,
	mod_time time,
	height double,
	width float
) engine=InnoDB ;

insert into addressbook values( NULL, 'A', 'B', '123456', 'route 66', 'C', '9876', 'Nowhere', now(), date( now() ), time( now() ), '12.754638', '12.754638' );
insert into addressbook values( NULL, 'D', 'E', '7890', 'Park Row', 'New York', '1234', NULL, now(), date( now() ), time( now() ), '12.754638', '12.754638' );
insert into addressbook values( NULL, 'D', 'E', '184736', 'Cove Road', 'Orleans', '', 'USA', now(), date( now() ), time( now() ), '12.754638', '12.754638' );
insert into addressbook values( NULL, 'àèéìòù€', 'B', '234', 'X', 'Y', '76', 'Z', now(), date( now() ), time( now() ), '12.754638', '12.754638' );

create table images (
	name varchar( 100 ) not null,
	data blob,
	description text
) engine=InnoDB;

insert into images values( 'foo', '23ljksdkj3isdfl23\0w32lkh', 'àèéìòù€' );

