#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <mysql/mysql.h>

#ifndef REB_HOST_H
#define REB_HOST_H
#include "reb-host.h"
#endif

#define RXI_SERIES_INFO(a,b) RL->series(a,b)

#include "mysqldb_init.h"
#include "utils.h"

RL_LIB *RL = NULL;


RXIEXT int R3MYSQL_make_error ( RXIFRM *frm, REBSER *database, const char * message );
RXIEXT int R3MYSQL_make_ok ( RXIFRM *frm );
RXIEXT int R3MYSQL_get_database_connection ( RXIFRM *frm, BOOL null_is_error, REBSER **database, MYSQL **connection );
RXIEXT int R3MYSQL_connect ( RXIFRM *frm );
RXIEXT int R3MYSQL_close ( RXIFRM *frm );
RXIEXT int R3MYSQL_execute ( RXIFRM *frm );
RXIEXT int R3MYSQL_fetch_row ( RXIFRM *frm );
RXIEXT int R3MYSQL_set_autocommit ( RXIFRM *frm );

#define MAKE_ERROR(message) R3MYSQL_make_error( frm, database, message )
#define MAKE_OK() R3MYSQL_make_ok( frm )

#define EXECUTE_QUERY_RETRIES 5  // number of times to retry the query if an error occurs
#define ER_LOCK_DEADLOCK 1213    // MySQL code for deadlock error

/* {{{ RX_Init ( int opts, RL_LIB *lib )

Rebol extension initialization function.
*/
RXIEXT const char *RX_Init ( int opts, RL_LIB *lib ) {
	RL = lib;
	if ( !CHECK_STRUCT_ALIGN ) {
		printf ( "CHECK_STRUCT_ALIGN failed\n" );
		return 0;
	}
	return init_block;
}
// }}}

/* {{{ RX_Quit ( int opts )

Rebol extension quit function.
*/
RXIEXT int RX_Quit ( int opts ) {
	return 0;
}
// }}}

/* {{{ RX_Call ( int cmd, RXIFRM *frm, void *data )

Rebol extension command dispatcher.
*/
RXIEXT int RX_Call ( int cmd, RXIFRM *frm, void *data ) {
	switch ( cmd ) {

	case CMD_INT_CONNECT:
		return R3MYSQL_connect ( frm );

	case CMD_INT_CLOSE:
		return R3MYSQL_close ( frm );

	case CMD_INT_EXECUTE:
		return R3MYSQL_execute ( frm );

	case CMD_INT_FETCH_ROW:
		return R3MYSQL_fetch_row ( frm );

	case CMD_INT_SET_AUTOCOMMIT:
		return R3MYSQL_set_autocommit ( frm );

	default:
		return RXR_NO_COMMAND;
	}
}
// }}}

/* {{{ R3MYSQL_make_error ( RXIFRM *frm, REBSER *database, const char * message )

Writes the message in the database/error field and sets the return value to "false".

Parameters:

 - frm: Rebol command frame;
 - database: Rebol database object;
 - message: string with the error message.

Returns the RXR_VALUE result.
*/
RXIEXT int R3MYSQL_make_error ( RXIFRM *frm, REBSER *database, const char * message ) {
	RXIARG value;
	unsigned int i = 0;  // unsigned otherwise RL_SET_CHAR complains

	// TODO: verify if I should create a UTF8 string (last parameter set to 1)
	REBSER *desc = RL_MAKE_STRING ( strlen ( message ), 0 );
	for ( i = 0; i < strlen ( message ); i++ ) RL_SET_CHAR ( desc, i, message [ i ] );

	value.series = desc;
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "error" ), value, RXT_STRING );

	RXA_LOGIC ( frm, 1 ) = 0;
	RXA_INDEX ( frm, 1 ) = 0;
	RXA_TYPE ( frm, 1 ) = RXT_LOGIC;
	return RXR_VALUE;
}
// }}}

/* {{{ R3MYSQL_make_ok ( RXIFRM *frm )

Sets the return value to "true".

Returns the RXR_VALUE result.
*/
RXIEXT int R3MYSQL_make_ok ( RXIFRM *frm ) {
	RXA_LOGIC ( frm, 1 ) = 1;
	RXA_INDEX ( frm, 1 ) = 0;
	RXA_TYPE ( frm, 1 ) = RXT_LOGIC;
	return RXR_VALUE;
}
// }}}

/* {{{ R3MYSQL_get_database_connection ( RXIFRM *frm, BOOL null_is_error, REBSER **database, MYSQL **connection )

Retrieves from the Rebol command frame the database object and the connection handle.

Parameters:

- frm: Rebol command frame;
- null_is_error: if TRUE an error will be returned if connection is NULL;
- database: pointer to a REBSER* structure: it will be filled with the first argument in "frm";
- connection: pointer to a MYSQL* structure: it will be filled with the "int-connection"
  word in the "database" object.

Note that the returned "connection" value can be NULL.

Returns:

- RXR_TRUE on success;
- RXR_VALUE if an error has occurred.
*/
RXIEXT int R3MYSQL_get_database_connection ( RXIFRM *frm, BOOL null_is_error, REBSER **database, MYSQL **connection ) {
	int dtype = 0;
	RXIARG value;

	*database = RXA_OBJECT ( frm, 1 );
	dtype = RL_GET_FIELD ( *database, RL_MAP_WORD ( (REBYTE *) "int-connection" ), &value );
	switch ( dtype ) {
		case RXT_NONE:
			*connection = NULL;
			if ( null_is_error ) return R3MYSQL_make_error ( frm, *database, "Invalid connection handle: none" );
			break;

		case RXT_HANDLE:
			*connection = value.addr;
			break;

		default:
			return R3MYSQL_make_error ( frm, *database, "Invalid connection handle" );
	}

	return RXR_TRUE;
}
// }}}

/* {{{ RXIEXT int R3MYSQL_connect ( RXIFRM *frm )

Connects to the database.

Parameters:

- frm: Rebol command frame;

Calls R3MYSQL_make_ok on success, R3MYSQL_make_error on error.
*/
RXIEXT int R3MYSQL_connect ( RXIFRM *frm ) {
	REBSER *database, *rhost, *rlogin, *rpassword, *rname = NULL;
	char *host, *login, *password, *name = NULL;
	MYSQL *conn = NULL;
	//int dtype = 0;
	RXIARG value;
	int res = 0;

	//database = RXA_OBJECT ( frm, 1 );

	if ( ( res = R3MYSQL_get_database_connection ( frm, FALSE, &database, &conn ) ) != RXR_TRUE ) return res;

	rhost = RXA_SERIES ( frm, 2 );
	rlogin = RXA_SERIES ( frm, 3 );
	rpassword = RXA_SERIES ( frm, 4 );
	rname = RXA_SERIES ( frm, 5 );

	// TODO: verify if strings are in UTF8
	RL_GET_STRING ( rhost, 0, (void **) &host );
	RL_GET_STRING ( rlogin, 0, (void **) &login );
	RL_GET_STRING ( rpassword, 0, (void **) &password );
	RL_GET_STRING ( rname, 0, (void **) &name );

	/* printf ( "host: %s\n", host );
	printf ( "login: %s\n", login );
	printf ( "password: %s\n", password );
	printf ( "name: %s\n", name ); */


	conn = mysql_init ( conn );
	//if conn == NULL ...

	mysql_options ( conn, MYSQL_OPT_RECONNECT, (const void *) "1" );
	// Probably this isn't needed because I used mysql_set_character_set() below.
	// mysql_options ( conn, MYSQL_SET_CHARSET_NAME, (const void *) "utf8" );

	if ( mysql_real_connect ( conn, host, login, password, name, 0, 0, 0 ) == NULL ) {
		int err = MAKE_ERROR ( mysql_error ( conn ) );
		mysql_close ( conn );
		return err;
	}

	mysql_autocommit ( conn, 1 );
	mysql_set_character_set ( conn, "utf8" );

	value.addr = conn;
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-connection" ), value, RXT_HANDLE );
	return MAKE_OK ();
}
// }}}

/* {{{ R3MYSQL_close ( RXIFRM *frm )

Closes the connection to the database.

Parameters:

- frm: Rebol command frame;

Calls R3MYSQL_make_ok on success, R3MYSQL_make_error on error.
*/
RXIEXT int R3MYSQL_close ( RXIFRM *frm ) {
	REBSER *database = NULL;
	MYSQL *conn = NULL;
	MYSQL_RES *result = NULL;
	int dtype = 0;
	RXIARG value;
	int res = 0;

	if ( ( res = R3MYSQL_get_database_connection ( frm, TRUE, &database, &conn ) ) != RXR_TRUE ) return res;

	// free an existing result set
	dtype = RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), &value );
	if ( dtype == RXT_HANDLE ) result = value.addr;

	if ( result ) {
		mysql_free_result ( result );
		value.addr = NULL;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), value, RXT_NONE );
	}

	mysql_close ( conn );

	value.addr = NULL;
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-connection" ), value, RXT_NONE );
	return MAKE_OK ();
}
// }}}

/* {{{ R3MYSQL_execute ( RXIFRM *frm )

Executes the query and stores the result (if any).

Parameters:

- frm: Rebol command frame;

Calls R3MYSQL_make_ok on success, R3MYSQL_make_error on error.
*/
RXIEXT int R3MYSQL_execute ( RXIFRM *frm ) {
	REBSER *database = NULL;
	REBSER *rsql = NULL;
	MYSQL *conn = NULL;
	MYSQL_RES *result = NULL;
	int res = 0;
	int dtype = 0;
	RXIARG value;
	RXIARG r_num_fields;
	char *sql = NULL;

	if ( ( res = R3MYSQL_get_database_connection ( frm, TRUE, &database, &conn ) ) != RXR_TRUE ) return res;
	rsql = RXA_SERIES ( frm, 2 );
	// TODO: verify if strings are in UTF8
	RL_GET_STRING ( rsql, 0, (void **) &sql );

	// free an existing result set (if any)
	dtype = RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), &value );
	switch ( dtype ) {
		case RXT_NONE:
			result = NULL;
			break;

		case RXT_HANDLE:
			result = value.addr;
			mysql_free_result ( result );
			result = NULL;
			value.addr = NULL;
			RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), value, RXT_NONE );
			break;

		default:
			return MAKE_ERROR ( "Invalid result handle" );
	}

	// perform the query
	int count = 0;

	while ( count < EXECUTE_QUERY_RETRIES ) {
		if ( mysql_query ( conn, sql ) != 0 ) {
			if ( ( mysql_errno ( conn ) == ER_LOCK_DEADLOCK ) && ( count++ < EXECUTE_QUERY_RETRIES ) )
				continue;
			else
				return MAKE_ERROR ( mysql_error ( conn ) );
		} else
			break;
	}

	// retrieve results

	result = mysql_store_result ( conn );
	if ( mysql_errno ( conn ) ) return MAKE_ERROR ( mysql_error ( conn ) );

	value.int64 = mysql_affected_rows ( conn );
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "num-rows" ), value, RXT_INTEGER );

	// result is NULL if the sql isn't a "select" query

	if ( result ) {
		r_num_fields.int64 = mysql_num_fields ( result );
		value.addr = result;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), value, RXT_HANDLE );

	} else {
		r_num_fields.int64 = 0;
		value.addr = 0;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), value, RXT_NONE );

		if ( mysql_field_count ( conn ) == 0 && mysql_insert_id ( conn ) != 0 ) {
			value.int64 = mysql_insert_id ( conn );
			RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "last-insert-id" ), value, RXT_INTEGER );
		} else {
			value.int64 = 0;
			RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "last-insert-id" ), value, RXT_NONE );
		}
	}

	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "num-cols" ), r_num_fields, RXT_INTEGER );

	return MAKE_OK ();
}
// }}}

/* {{{ R3MYSQL_fetch_row ( RXIFRM *frm )

Retrieves a single row of data.

Parameters:

- frm: Rebol command frame;

Returns a block containing the row data. Each field is represented by two
strings: the first is the field name and the second is the field value.

Example:

    [ "field A" "value A" "field B" "value B" ... ]

Calls R3MYSQL_make_error on error.
*/
RXIEXT int R3MYSQL_fetch_row ( RXIFRM *frm ) {
	REBSER *database = NULL;
	MYSQL *conn = NULL;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row;
	MYSQL_FIELD *field = NULL;
	int res = 0;
	int dtype = 0;
	RXIARG value;
	int num_cols = 0;
	int k = 0;
	REBSER *block = NULL;
	REBSER *s = NULL;
	unsigned int i = 0;
	unsigned long *field_lengths = NULL;

	if ( ( res = R3MYSQL_get_database_connection ( frm, TRUE, &database, &conn ) ) != RXR_TRUE ) return res;

	// get the result set of the last query
	dtype = RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), &value );
	switch ( dtype ) {
		case RXT_NONE:
			return MAKE_ERROR ( "Invalid result handle: none" );
			break;

		case RXT_HANDLE:
			result = value.addr;
			break;

		default:
			return MAKE_ERROR ( "Invalid result handle" );
	}

	row = mysql_fetch_row ( result );

	if ( ! row ) {
		if ( mysql_errno ( conn ) ) {
			return MAKE_ERROR ( mysql_error ( conn ) );
		} else {
			// end of rows
			return RXR_NONE;
		}
	}

	field_lengths = mysql_fetch_lengths ( result );

	// transform the row in a Rebol block

	dtype = RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "num-cols" ), &value );
	// TODO: signal a proper error
	num_cols = ( dtype == RXT_INTEGER ) ? value.int64 : 0;

	block = RL_MAKE_BLOCK ( num_cols * 2 );

	for ( k = 0; k < num_cols; k++ ) {
		field = mysql_fetch_field_direct ( result, k );

		// create the field name

		s = RL_MAKE_STRING ( field->name_length, 1 );
		string_utf8_to_rebol_unicode ( (uint8_t *) field->name, s );

		value.series = s;
		value.index = 0;
		RL_SET_VALUE ( block, 2 * k, value, RXT_STRING );

		if ( row [ k ] ) {
			// create the field value

			s = RL_MAKE_STRING ( field_lengths [ k ], 1 );
			//printf ( "C: field name: %s\n", field->name );

			if (
				( ( field->type == MYSQL_TYPE_STRING ) || ( field->type == MYSQL_TYPE_VAR_STRING ) || ( field->type == MYSQL_TYPE_BLOB ) )
				&&
				( field->flags & BINARY_FLAG )
			) {
				for ( i = 0; i < field_lengths [ k ]; i++ ) RL_SET_CHAR ( s, i, row [ k ] [ i ] );
			} else {
				string_utf8_to_rebol_unicode ( (uint8_t *) row [ k ], s );
			}

			value.series = s;
			value.index = 0;
			RL_SET_VALUE ( block, ( 2 * k ) + 1, value, RXT_STRING );

		} else {
			RL_SET_VALUE ( block, ( 2 * k ) + 1, value, RXT_NONE );
		}
	}

	RXA_SERIES ( frm, 1 ) = block;
	RXA_INDEX ( frm, 1 ) = 0;
	RXA_TYPE ( frm, 1 ) = RXT_BLOCK;
	return RXR_VALUE;
}
// }}}

/* {{{ R3MYSQL_set_autocommit ( RXIFRM *frm )

Sets the autocommit flag.

Parameters:

- frm: Rebol command frame;

Calls R3MYSQL_make_ok on success, R3MYSQL_make_error on error.
*/
RXIEXT int R3MYSQL_set_autocommit ( RXIFRM *frm ) {
	REBSER *database = NULL;
	MYSQL *conn = NULL;
	RXIARG value;
	i32 enabled = 1;
	int res = 0;

	if ( ( res = R3MYSQL_get_database_connection ( frm, TRUE, &database, &conn ) ) != RXR_TRUE ) return res;

	enabled = RXA_LOGIC ( frm, 2 );

	mysql_autocommit ( conn, enabled );

	value.int32a = enabled;
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "autocommit" ), value, RXT_LOGIC );
	return MAKE_OK ();
}
// }}}


