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
	params [block!]
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
	int-statement: none  ; pointer to the statement structure
	int-bind-params: none  ; pointer to the bind array for parameters
	int-bind-params-length: 0  ; length of the bind array for parameters
	int-bind-result: none  ; pointer to the bind array for results
	int-bind-result-length: 0  ; length of the bind array for results
	int-sql-string: none  ; pointer to the actual sql string sent to the database
	int-field-convert-list: none  ; list of (positions of) fields to be converted to a Rebol datatype

	; public fields

	autocommit: true  ; (readonly) autocommit is enabled by default
	error: ""   ; (readonly) error description
	num-rows: 0  ; (readonly) number of affected rows
	num-cols: 0  ; (readonly) number of fields in the result set
	last-insert-id: none  ; (readonly) id of the last inserted record if an auto_increment column exists
	store-result-client-side: false  ; a value of "true" causes the complete result set to be buffered on the client

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
		sql [string!] "Query string"
		/params parm [block!] "Query parameters (still unsupported)"
	] [
		if not parm [ parm: [] ]
		self/int-field-convert-list: none
		self/check-error int-execute self sql parm
	]

	fetch-row: function [
		{Retrieves a single row of data.}
	] [
		row: self/check-error int-fetch-row self

		if row [
			foreach field self/int-field-convert-list [
				poke row ( 2 * field ) from-c-type pick row ( 2 * field )
			]
		]

		row
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



to-c-type: function [ thing ] [
	switch/default type?/word thing [
		date! [
			res: reduce [ "date" thing/day thing/month thing/year ]
			if thing/time [
				res/1: "datetime"
				append res reduce [ thing/hour thing/minute thing/second ]
			]
			res
		]

		time! [
			reduce [ "time" thing/hour thing/minute thing/second ]
		]
	] [
		; default
		thing
	]
]


from-c-type: function [ thing ] [
	either ( type? thing ) = block! [
		switch thing/1 [
			"date" [ to-date next thing ]
			"time" [ to-time next thing ]
			"datetime" [
				t: to-time at thing 5
				to-date append copy/part next thing 3 t
			]
		]
	] [
		thing
	]
]

