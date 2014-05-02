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

print "^/* Set autocommit = false"
db/set-autocommit false
print [ "autocommit:" db/autocommit ]

print "^/* Execute: select * from foobar -> fails"
either error? res: try [ db/execute "select * from foobar" ] [
	print res
] [
	print [ "int-result:" db/int-result ]
	print [ "num-rows:" db/num-rows ]
	print [ "num-cols:" db/num-cols ]
]


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

