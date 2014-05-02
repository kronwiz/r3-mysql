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
	{Connects to the database}
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

	connect: function [ host login password name ] [
		self/check-error int-connect self host login password name
	]

	close: function [] [
		self/check-error int-close self
	]

	execute: function [ sql [string!] ] [
		self/check-error int-execute self sql
	]

	fetch-row: function [] [
		self/check-error int-fetch-row self
	]

	set-autocommit: function [ enabled [logic!] ] [
		self/check-error int-set-autocommit self enabled
	]

	check-error: function [ res ] [
		either res = false [
			cause-error 'user 'message self/error
		] [
			res
		]
	]
]


