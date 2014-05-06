REBOL []

mysql: import %../mysqldb.so

db: make mysql/database []

print "^/* Connect: fail"
either error? res: try [ db/connect "localhost" "test" "test" "foobar" ] [
	print res
] [
	print [ "int-connection:" db/int-connection ]
]

print "^/* Connect: success"
either error? res: try [ db/connect "localhost" "test" "test" "test" ] [
	print res
] [
	print [ "int-connection:" db/int-connection ]
]

print "^/* Execute: select * from addressbook"
either error? res: try [ db/execute "select * from addressbook" ] [
	print res
] [
	print [ "int-result:" db/int-result ]
	print [ "num-rows:" db/num-rows ]
	print [ "num-cols:" db/num-cols ]
]

print "^/* Fetch row"
row: db/fetch-row
while [ row <> none ] [
	print [ "row:" row ]
	print [ "address:" select row "address" ]
	row: db/fetch-row
]

print "^/* Execute: select * from foobar -> fails"
either error? res: try [ db/execute "select * from foobar" ] [
	print res
] [
	print [ "int-result:" db/int-result ]
	print [ "num-rows:" db/num-rows ]
	print [ "num-cols:" db/num-cols ]
]

print "^/* Execute: insert into addressbook"
either error? res: try [ db/execute "insert into addressbook values( NULL, 'First', 'Last', '333.123.444', 'via di qua', 'non so', '9876', 'Italy', now() )" ] [
	print res
] [
	print [ "Affected rows:" db/num-rows ]
]

print "^/* Execute: delete from addressbook"
either error? res: try [ db/execute "delete from addressbook where userid = '11'" ] [
	print res
] [
	print [ "Affected rows:" db/num-rows ]
]

print "^/* Set autocommit = false"
db/set-autocommit false
print [ "autocommit:" db/autocommit ]

print "^/* Close: success"
either error? res: try [ db/close ] [
	print res
] [
	print [ "int-connection:" db/int-connection ]
	print [ "int-result:" db/int-result ]
]

print "^/* Close: fail"
either error? res: try [ db/close ] [
	print res
] [
	print [ "int-connection:" db/int-connection ]
]

