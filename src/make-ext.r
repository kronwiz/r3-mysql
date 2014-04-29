REBOL [
	Title: "Build REBOL 3.0 Extension Module Data"
	Needs: 2.100.70
]

source: load/all %mysqldb.r

out: make string! 10000
emit: func [d] [repend out d]

source/rebol/exports: words: collect-words/set source

emit-cmt: func [text] [
	emit [
{/***********************************************************************
**
**  } text {
**
***********************************************************************/

}]
]

to-cstr: either system/version/4 = 3 [
	; Windows format:
	func [str /local out] [
		out: make string! 4 * (length? str)
		out: insert out tab
		forall str [
			out: insert out reduce [to-integer first str ", "]
			if zero? ((index? str) // 10) [out: insert out "^/^-"]
		]
		remove/part out either (pick out -1) = #" " [-2][-4]
		head out
	]
][
	; Other formats (Linux, OpenBSD, etc.):
	func [str /local out data] [
		out: make string! 4 * (length? str)
		forall str [
			data: copy/part str 16
			str: skip str 15
			data: enbase/base data 16
			forall data [
				insert data "\x"
				data: skip data 3
			]
			data: tail data
			insert data {"^/}
			append out {"}
			append out head data
		]
		;remove head out
	]
]


emit-cmt "REBOL Extension Module (generated)"

emit "enum command_nums {^/"
foreach word words [
	emit [tab "CMD_" uppercase replace/all replace/all to-string word #"-" #"_" #"?" #"Q" ",^/" ]
]
emit "};^/^/"


data: append trim/head mold/only source newline
append data null

emit ["const char init_block[] = {^/" to-cstr data "^/};"]
emit [newline newline]

write %mysqldb_init.h to-binary out
quit

