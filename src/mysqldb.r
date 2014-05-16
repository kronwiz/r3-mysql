REBOL [
	Title: {Rebol 3 MySQL module}
	Name: mysqldb
	Type: extension
	Exports: []
]


; ============================
; internal (C level) functions
; ============================

int-connect: command [
	database [object!]
	host [string!]
	login [string!]
	password [string!]
	name [string!]
]

int-close: command [
	database [object!]
]

int-execute: command [
	database [object!]
	sql [string!]
]

int-fetch-row: command [
	database [object!]
]

int-set-autocommit: command [
	database [object!]
	enabled [logic!]
]


; =====================
; Rebol level interface
; =====================

database: object [
	; internal (don't use at the Rebol level) fields

	int-connection: none  ; pointer to current connection
	int-result: none  ; pointer to the result structure

	; public fields

	autocommit: true  ; (readonly) autocommit is enabled by default
	error: ""   ; (readonly) error description
	num-rows: 0  ; (readonly) number of affected rows
	num-cols: 0  ; (readonly) number of fields in the result set
	last-insert-id: none  ; (readonly) id of the last inserted record if an auto_increment column exists

	connect: function [
		{Connects to the database.}
		host [string!] "Host name or address"
		login [string!] "User name"
		password [string!] "User password"
		name [string!] "Database name"
	] [
		self/check-error int-connect self host login password name
	]

	close: function [
		{Closes the connection to the database.}
	] [
		self/check-error int-close self
	]

	execute: function [
		{Executes a SQL query.}
		sql [string!] "SQL query"
	] [
		self/check-error int-execute self sql
	]

	fetch-row: function [
		{Retrieves a single row of data.}
	] [
		self/check-error int-fetch-row self
	]

	set-autocommit: function [
		{Enables or disables autocommit mode.}
		enabled [logic!] "Set to true to enable or false to disable"
	] [
		self/check-error int-set-autocommit self enabled
	]

	check-error: function [
		{If the C module returned an error creates a Rebol error! object}
		res
	] [
		either res = false [
			cause-error 'user 'message self/error
		] [
			res
		]
	]
]


