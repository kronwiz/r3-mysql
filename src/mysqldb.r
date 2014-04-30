REBOL [
	Title: {Rebol 3 MySQL module}
	Name: mysqldb
	Type: extension
	Exports: []
]

print "Initializing extension..."

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
		int-connect self host login password name
	]

	close: function [] [
		int-close self
	]

	execute: function [ sql [string!] ] [
		int-execute self sql
	]

	fetch-row: function [] [
		int-fetch-row self
	]

	set-autocommit: function [ enabled [logic!] ] [
		int-set-autocommit self enabled
	]
]

print "Extension added"

