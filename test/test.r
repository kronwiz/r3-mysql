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

; print the charsets so we're sure we're using utf8
print "^/* Execute: show variables like '%char%'"
either error? res: try [ db/execute "show variables like '%char%'" ] [
	print res
] [
	print [ "num-rows:" db/num-rows ]
	print [ "num-cols:" db/num-cols ]
]

print "^/* Fetch row"
row: db/fetch-row
while [ row <> none ] [
	print [ "row:" row ]
	row: db/fetch-row
]

print "^/* Execute: select * from addressbook"
either error? res: try [ db/execute "select * from addressbook" ] [
	print res
] [
	print [ "int-result:" db/int-result ]
	print [ "num-rows:" db/num-rows ]
	print [ "num-cols:" db/num-cols ]
	print [ "last-insert-id:" db/last-insert-id ]
]

print "^/* Fetch row"
row: db/fetch-row
while [ row <> none ] [
	print [ "row:" mold row ]
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
either error? res: try [ db/execute "insert into addressbook values( NULL, 'First', 'Last', '333.123.444', 'via di qua', 'non so', '9876', 'Italy', now(), date( now() ), time( now() ), '5634.333', '23.16432' )" ] [
	print res
] [
	print [ "Affected rows:" db/num-rows ]
	print [ "last-insert-id:" db/last-insert-id ]
]

print "^/* Execute: insert into addressbook a field too long gets truncated"
either error? res: try [ db/execute "insert into addressbook values( NULL, 'First', 'Last', '333.123.444', 'via di qua', 'non so', '12345678901234567890123456789012345678901234567890', 'Italy', now(), date( now() ), time( now() ), '5634.333', '23.16432' )" ] [
	print res
] [
	print [ "Affected rows:" db/num-rows ]
]

print "^/* Execute: insert into addressbook unicode characters"
either error? res: try [ db/execute "insert into addressbook values( NULL, 'àèéìòù€', 'Unicode', '234', 'X', 'Y', '76', 'Z', now(), date( now() ), time( now() ), '5634.333', '23.16432' )" ] [
	print res
] [
	print [ "Affected rows:" db/num-rows ]
]

print "^/* Execute: insert into addressbook with params"
either error? res: try [ db/execute/params "insert into addressbook values( ?, 'First', 'Last', '333.123.444', ?, 'non so', ?, 'Italy', now(), date( now() ), time( now() ), ?, '23.16432' )" [ none "via di qua" 4563 5634.333 ] ] [
	print res
] [
	print [ "Affected rows:" db/num-rows ]
	print [ "last-insert-id:" db/last-insert-id ]
]

;print "^/* Execute: delete from addressbook"
;either error? res: try [ db/execute "delete from addressbook where firstname = 'First' or lastname = 'Unicode'" ] [
	;print res
;] [
	;print [ "Affected rows:" db/num-rows ]
;]

print "^/* Execute: select * from images"
either error? res: try [ db/execute "select * from images" ] [
	print res
] [
	print [ "num-rows:" db/num-rows ]
	print [ "num-cols:" db/num-cols ]
]

print "^/* Fetch row"
row: db/fetch-row
while [ row <> none ] [
	probe row
	row: db/fetch-row
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

