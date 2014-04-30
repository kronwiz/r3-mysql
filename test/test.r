REBOL []

mysql: import %../mysqldb.so

db: make mysql/database []

print "^/* Connect: fail"
res: db/connect "localhost" "test" "test" "foobar"

print [ "res:" res type? res ]
either res = false [
	print [ "error:" db/error ]
] [
	print [ "int-connection:" db/int-connection ]
]

print "^/* Connect: success"
res: db/connect "localhost" "test" "test" "test"

print [ "res:" res type? res ]
either res = false [
	print [ "error:" db/error ]
] [
	print [ "int-connection:" db/int-connection ]
]

print "^/* Execute: select * from addressbook"
res: db/execute "select * from addressbook"
either res = false [
	print [ "error:" db/error ]
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
res: db/execute "select * from foobar"
either res = false [
	print [ "error:" db/error ]
] [
	print [ "int-result:" db/int-result ]
	print [ "num-rows:" db/num-rows ]
	print [ "num-cols:" db/num-cols ]
]


print "^/* Close: success"
db/close
print [ "int-connection:" db/int-connection ]
print [ "int-result:" db/int-result ]

print "^/* Close: fail"
res: db/close
either res = false [
	print [ "error:" db/error ]
] [
	print [ "int-connection:" db/int-connection ]
]

