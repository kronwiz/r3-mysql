#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
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
RXIEXT int R3MYSQL_execute_prepared_stmt ( RXIFRM *frm );
RXIEXT int R3MYSQL_fetch_row_prepared_stmt ( RXIFRM *frm );
MYSQL_BIND *R3MYSQL_build_bind_from_param ( REBSER *params, int params_length );
MYSQL_BIND *R3MYSQL_build_bind_for_result ( MYSQL_RES *result, i64 num_cols, REBSER *field_convert_list );
void R3MYSQL_free_prepared_stmt ( REBSER *database );

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
		return R3MYSQL_execute_prepared_stmt ( frm );

	case CMD_INT_FETCH_ROW:
		return R3MYSQL_fetch_row_prepared_stmt ( frm );

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

/* {{{ R3MYSQL_connect ( RXIFRM *frm )

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

	// free an existing prepared statement
	R3MYSQL_free_prepared_stmt ( database );

	mysql_close ( conn );

	value.addr = NULL;
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-connection" ), value, RXT_NONE );

	return MAKE_OK ();
}
// }}}

/* {{{ R3MYSQL_execute ( RXIFRM *frm )

DEPRECATED: now the queries are always executed using R3MYSQL_execute_prepared_stmt().

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
	int sqllength = 0;  // sql statement length if it's a standard string (Rebol returns a negative value)
	int utf8string_length = 0;  // length of the UTF8 buffer to allocate
	int stmt_length = 0;  // actual length of the sql statement (after conversion to UTF8 if needed)

	if ( ( res = R3MYSQL_get_database_connection ( frm, TRUE, &database, &conn ) ) != RXR_TRUE ) return res;
	rsql = RXA_SERIES ( frm, 2 );

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

	sqllength = RL_GET_STRING ( rsql, 0, (void **) &sql );
	// if the string is unicode we have to encode it in UTF8 before sending it to the database
	if ( sqllength > 0 ) {
		utf8string_length = ( sqllength *3 ) + 1;  // the +1 is for the trailing \0
		sql = malloc ( utf8string_length );
		bzero ( sql, utf8string_length );
		stmt_length = string_rebol_unicode_to_utf8 ( sql, utf8string_length, rsql );
	} else {
		stmt_length = -1 * sqllength;
	}

	while ( count < EXECUTE_QUERY_RETRIES ) {
		if ( mysql_real_query ( conn, sql, stmt_length ) != 0 ) {
			if ( ( mysql_errno ( conn ) == ER_LOCK_DEADLOCK ) && ( count++ < EXECUTE_QUERY_RETRIES ) )
				continue;
			else {
				if ( sqllength > 0 ) free ( sql );
				return MAKE_ERROR ( mysql_error ( conn ) );
			}
		} else
			break;
	}

	if ( sqllength > 0 ) free ( sql );

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

DEPRECATED: now a result set is always retrieved using R3MYSQL_fetch_row_prepared_stmt().

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

/* {{{ R3MYSQL_execute_prepared_stmt ( RXIFRM *frm )

Executes the query and binds the buffers to retrieve the result (if any).

Parameters:

- frm: Rebol command frame;

Calls R3MYSQL_make_ok on success, R3MYSQL_make_error on error.
*/
RXIEXT int R3MYSQL_execute_prepared_stmt ( RXIFRM *frm ) {
	REBSER *database = NULL;
	REBSER *rsql = NULL;
	REBSER *params = NULL;
	MYSQL *conn = NULL;
	MYSQL_RES *result = NULL;
	MYSQL_STMT *mstmt = NULL;
	MYSQL_BIND *mbind_arr = NULL;
	int res = 0;
	int params_length = 0;
	RXIARG value;
	RXIARG r_num_fields;
	RXIARG r_num_rows;
	char *sql = NULL;
	int sqllength = 0;  // sql statement length if it's a standard string (Rebol returns a negative value)
	int utf8string_length = 0;  // length of the UTF8 buffer to allocate
	int stmt_length = 0;  // actual length of the sql statement (after conversion to UTF8 if needed)
	REBSER *field_convert_list = NULL;


	if ( ( res = R3MYSQL_get_database_connection ( frm, TRUE, &database, &conn ) ) != RXR_TRUE ) return res;
	rsql = RXA_SERIES ( frm, 2 );
	params = RXA_SERIES ( frm, 3 );
	if ( params ) params_length = RXI_SERIES_INFO ( params, RXI_SER_TAIL );

	// free and close a previous statement (if any)
	R3MYSQL_free_prepared_stmt ( database );

	//printf ( "C: params length: %d\n", params_length );

	// *** prepare the SQL string ***

	sqllength = RL_GET_STRING ( rsql, 0, (void **) &sql );
	// if the string is unicode we have to encode it in UTF8 before sending it to the database
	if ( sqllength > 0 ) {
		utf8string_length = ( sqllength * 3 ) + 1;  // the +1 is for the trailing \0
		sql = malloc ( utf8string_length );
		memset ( sql, 0, utf8string_length );
		stmt_length = string_rebol_unicode_to_utf8 ( sql, utf8string_length, rsql );
		value.addr = sql;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-sql-string" ), value, RXT_HANDLE );
	} else {
		stmt_length = -1 * sqllength;
	}

	// *** initialize the statement ***

	mstmt = mysql_stmt_init ( conn );
	value.addr = mstmt;
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-statement" ), value, RXT_HANDLE );
	if ( mysql_stmt_prepare ( mstmt, sql, stmt_length ) != 0 )
		return MAKE_ERROR ( mysql_error ( conn ) );

	//printf ( "C: mstmt: %p\n", mstmt );

	// *** bind and fill the parameters ***

	if ( params_length > 0 ) {
		mbind_arr = R3MYSQL_build_bind_from_param ( params, params_length );

		value.addr = mbind_arr;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-params" ), value, RXT_HANDLE );
		value.int64 = params_length;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-params-length" ), value, RXT_INTEGER );

		if ( mysql_stmt_bind_param ( mstmt, mbind_arr ) != 0 )
			return MAKE_ERROR ( mysql_error ( conn ) );
	}

	// *** execute the statement ***

	if ( mysql_stmt_execute ( mstmt ) != 0 )
		return MAKE_ERROR ( mysql_error ( conn ) );

	// *** bind the result buffers ***

	if ( ( result = mysql_stmt_result_metadata ( mstmt ) ) != NULL ) {
		r_num_fields.int64 = mysql_num_fields ( result );
		r_num_rows.int64 = -1;

		field_convert_list = RL_MAKE_BLOCK ( r_num_fields.int64 );
		value.series = field_convert_list;
		value.index = 0;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-field-convert-list" ), value, RXT_BLOCK );

		mbind_arr = R3MYSQL_build_bind_for_result ( result, r_num_fields.int64, field_convert_list );

		value.addr = result;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), value, RXT_HANDLE );
		value.addr = mbind_arr;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-result" ), value, RXT_HANDLE );
		value.int64 = r_num_fields.int64;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-result-length" ), value, RXT_INTEGER );

		mysql_stmt_bind_result ( mstmt, mbind_arr );

		RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "store-result-client-side" ), &value );
		if ( value.int32a ) {
			mysql_stmt_store_result ( mstmt );
			r_num_rows.int64 = mysql_stmt_num_rows ( mstmt );
		}

	} else {
		value.addr = 0;
		RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), value, RXT_NONE );

		r_num_fields.int64 = 0;
		r_num_rows.int64 = mysql_stmt_affected_rows ( mstmt );

		if ( mysql_stmt_insert_id ( mstmt ) != 0 ) {
			value.int64 = mysql_stmt_insert_id ( mstmt );
			RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "last-insert-id" ), value, RXT_INTEGER );
		} else {
			value.int64 = 0;
			RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "last-insert-id" ), value, RXT_NONE );
		}
	}

	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "num-cols" ), r_num_fields, RXT_INTEGER );
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "num-rows" ), r_num_rows, RXT_INTEGER );

	return MAKE_OK ();
}
// }}}

/* {{{ R3MYSQL_build_bind_for_result ( MYSQL_RES *result, i64 num_cols, REBSER *field_convert_list )

Creates the MYSQL_BIND array and allocates all the buffers needed to store the
result set of the query.

Parameters:

- result: MYSQL_RES structure obtained from mysql_stmt_result_metadata();
- num_cols: number of columns in the result set.

Returns the MYSQL_BIND array address.

From MySQL documentation the buffer types are:

Input Variable C Type | buffer_type Value    | SQL Type of Destination Value
======================+======================+==============================
signed char           | MYSQL_TYPE_TINY      | TINYINT
short int             | MYSQL_TYPE_SHORT     | SMALLINT
int                   | MYSQL_TYPE_LONG      | INT
long long int         | MYSQL_TYPE_LONGLONG  | BIGINT
float                 | MYSQL_TYPE_FLOAT     | FLOAT
double                | MYSQL_TYPE_DOUBLE    | DOUBLE
MYSQL_TIME            | MYSQL_TYPE_TIME      | TIME
MYSQL_TIME            | MYSQL_TYPE_DATE      | DATE
MYSQL_TIME            | MYSQL_TYPE_DATETIME  | DATETIME
MYSQL_TIME            | MYSQL_TYPE_TIMESTAMP | TIMESTAMP
char[]                | MYSQL_TYPE_STRING    | TEXT, CHAR, VARCHAR
char[]                | MYSQL_TYPE_BLOB      | BLOB, BINARY, VARBINARY
                      | MYSQL_TYPE_NULL      | NULL

and the field types are:

Type Value            | Type Description
======================+=================
MYSQL_TYPE_TINY       | TINYINT field
MYSQL_TYPE_SHORT      | SMALLINT field
MYSQL_TYPE_LONG       | INTEGER field
MYSQL_TYPE_INT24      | MEDIUMINT field
MYSQL_TYPE_LONGLONG   | BIGINT field
MYSQL_TYPE_DECIMAL    | DECIMAL or NUMERIC field
MYSQL_TYPE_NEWDECIMAL | Precision math DECIMAL or NUMERIC field (MySQL 5.0.3 and up)
MYSQL_TYPE_FLOAT      | FLOAT field
MYSQL_TYPE_DOUBLE     | DOUBLE or REAL field
MYSQL_TYPE_BIT        | BIT field (MySQL 5.0.3 and up)
MYSQL_TYPE_TIMESTAMP  | TIMESTAMP field
MYSQL_TYPE_DATE       | DATE field
MYSQL_TYPE_TIME       | TIME field
MYSQL_TYPE_DATETIME   | DATETIME field
MYSQL_TYPE_YEAR       | YEAR field
MYSQL_TYPE_STRING     | CHAR or BINARY field
MYSQL_TYPE_VAR_STRING | VARCHAR or VARBINARY field
MYSQL_TYPE_BLOB       | BLOB or TEXT field (use max_length to determine the maximum length)
MYSQL_TYPE_SET        | SET field
MYSQL_TYPE_ENUM       | ENUM field
MYSQL_TYPE_GEOMETRY   | Spatial field
MYSQL_TYPE_NULL       | NULL-type field

*/
MYSQL_BIND *R3MYSQL_build_bind_for_result ( MYSQL_RES *result, i64 num_cols, REBSER *field_convert_list ) {
	MYSQL_BIND *mbind_arr = NULL;
	MYSQL_BIND *mbind = NULL;
	MYSQL_FIELD *field = NULL;
	RXIARG value;
	int i = 0;
	int fcl_pos = 0;

	//printf ( "C: >> R3MYSQL_build_bind_for_result\n" );

	mbind_arr = malloc ( num_cols * sizeof ( MYSQL_BIND ) );
	memset ( mbind_arr, 0, num_cols * sizeof ( MYSQL_BIND ) );

	//printf ( "C: mbind_arr: %p\n", mbind_arr );

	for ( i = 0; i < num_cols; i++ ) {
		field = mysql_fetch_field_direct ( result, i );
		mbind = &mbind_arr [ i ];
		//printf ( "C: mbind: %p\n", mbind );

		switch ( field->type ) {
			case MYSQL_TYPE_TINY:
			case MYSQL_TYPE_SHORT:
			case MYSQL_TYPE_LONG:
			case MYSQL_TYPE_INT24:
			case MYSQL_TYPE_LONGLONG:
				mbind->buffer_type = MYSQL_TYPE_LONGLONG;
				mbind->buffer = malloc ( sizeof ( long long int ) );
				memset ( mbind->buffer, 0, sizeof ( long long int ) );
				mbind->buffer_length = sizeof ( long long int );
				mbind->is_unsigned = (my_bool) 0;
				break;

			case MYSQL_TYPE_DECIMAL:
			case MYSQL_TYPE_NEWDECIMAL:
			case MYSQL_TYPE_DOUBLE:
				mbind->buffer_type = MYSQL_TYPE_DOUBLE;
				mbind->buffer = malloc ( sizeof ( double ) );
				memset ( mbind->buffer, 0, sizeof ( double ) );
				mbind->buffer_length = sizeof ( double );
				mbind->is_unsigned = (my_bool) 0;
				break;

			// case MYSQL_TYPE_BIT:
			// case MYSQL_TYPE_STRING:
			// case MYSQL_TYPE_VAR_STRING:
			// case MYSQL_TYPE_BLOB:
			// case MYSQL_TYPE_SET:
			// case MYSQL_TYPE_ENUM:
			// case MYSQL_TYPE_GEOMETRY:
			// case MYSQL_TYPE_NULL:

			case MYSQL_TYPE_TIMESTAMP:
			case MYSQL_TYPE_DATE:
			case MYSQL_TYPE_TIME:
			case MYSQL_TYPE_DATETIME:
			// case MYSQL_TYPE_YEAR:
				mbind->buffer_type = field->type;
				mbind->buffer = malloc ( sizeof ( MYSQL_TIME ) );
				memset ( mbind->buffer, 0, sizeof ( MYSQL_TIME ) );
				mbind->buffer_length = sizeof ( MYSQL_TIME );

				value.int64 = i + 1;
				RL_SET_VALUE ( field_convert_list, fcl_pos++, value, RXT_INTEGER );
				//printf ( "C: fcl_pos: %d\n", fcl_pos );
				break;

			default:
				//printf ( "C: field->length: %d\n", field->length );
				mbind->buffer_type = MYSQL_TYPE_STRING;
				mbind->buffer = malloc ( field->length );
				memset ( mbind->buffer, 0, field->length );
				mbind->buffer_length = field->length;
				break;
		}

		mbind->is_null = malloc ( sizeof ( my_bool ) );
		memset ( (void *) mbind->is_null, 0, sizeof ( my_bool ) );
		//printf ( "C: mbind.is_null: %p\n", mbind->is_null );
		mbind->error = malloc ( sizeof ( my_bool ) );
		memset ( (void *) mbind->error, 0, sizeof ( my_bool ) );
		mbind->length = malloc ( sizeof ( unsigned long ) );
		memset ( mbind->length, 0, sizeof ( unsigned long ) );
	}

	return mbind_arr;
}
// }}}

// {{{ R3MYSQL_build_bind_from_param ( MYSQL_BIND mbind, int dtype, RXIARG value )
MYSQL_BIND *R3MYSQL_build_bind_from_param ( REBSER *params, int params_length ) {
	REBSER block;
	RXIARG value;
	MYSQL_BIND *mbind_arr = NULL;
	MYSQL_BIND *mbind = NULL;
	//MYSQL_FIELD *field = NULL;
	char *strvalue = NULL;
	int strval_len = 0;
	char *buffer = NULL;
	int buffer_length = 0;
	int i = 0;
	int dtype = 0;

	mbind_arr = malloc ( params_length * sizeof ( MYSQL_BIND ) );
	memset ( mbind_arr, 0, params_length * sizeof ( MYSQL_BIND ) );

	for ( i = 0; i < params_length; i++ ) {
		mbind = &mbind_arr [ i ];
		mbind->length = malloc ( sizeof ( unsigned long ) );
		mbind->is_null = malloc ( sizeof ( my_bool ) );
		memset ( (void *) mbind->is_null, 0, sizeof ( my_bool ) );
		mbind->error = malloc ( sizeof ( my_bool ) );
		memset ( (void *) mbind->error, 0, sizeof ( my_bool ) );

		dtype = RL_GET_VALUE ( params, i, &value );
		//printf ( "C: dtype: %d\n", (int) dtype );

		switch ( dtype ) {
			case RXT_BLOCK:
				block = value.series;  // save for later use
				RL_GET_VALUE ( block, 0, &value );
				RL_GET_STRING ( value.series, 0, (void **) &strvalue );
				printf ( "C: block of type: %s\n", strvalue );
				break;

			case RXT_STRING:
				strval_len = RL_GET_STRING ( value.series, 0, (void **) &strvalue );
				// we have to encode the string in UTF8 before sending it to the database
				strval_len = strval_len > 0 ? strval_len : -1 * strval_len;
				buffer_length = ( strval_len * 3 ) + 1;  // the +1 is for the trailing \0
				buffer = malloc ( buffer_length );
				memset ( buffer, 0, buffer_length );
				strval_len = string_rebol_unicode_to_utf8 ( buffer, buffer_length, value.series );

				mbind->buffer_type = MYSQL_TYPE_STRING;
				mbind->buffer = buffer;
				mbind->buffer_length = buffer_length;
				*(mbind->length) = strval_len;
				break;

			case RXT_BINARY:
				// binary is like a string but without encoding
				strval_len = RL_GET_STRING ( value.series, 0, (void **) &strvalue );
				strval_len = strval_len > 0 ? strval_len : -1 * strval_len;
				buffer_length = strval_len;
				buffer = malloc ( buffer_length );
				memset ( buffer, 0, buffer_length );
				memcpy ( buffer, strvalue, strval_len );

				mbind->buffer_type = MYSQL_TYPE_STRING;
				mbind->buffer = buffer;
				mbind->buffer_length = buffer_length;
				*(mbind->length) = strval_len;
				break;

			case RXT_INTEGER:
				mbind->buffer_type = MYSQL_TYPE_LONGLONG;
				mbind->buffer = malloc ( sizeof ( long long int ) );
				memset ( mbind->buffer, 0, sizeof ( long long int ) );
				mbind->buffer_length = sizeof ( long long int );
				mbind->is_unsigned = (my_bool) 0;
				*(mbind->length) = sizeof ( long long int );
				memcpy ( mbind->buffer, &value.int64, sizeof ( long long int ) );
				break;

			case RXT_DECIMAL:
				mbind->buffer_type = MYSQL_TYPE_DOUBLE;
				mbind->buffer = malloc ( sizeof ( double ) );
				memset ( mbind->buffer, 0, sizeof ( double ) );
				mbind->buffer_length = sizeof ( double );
				mbind->is_unsigned = (my_bool) 0;
				*(mbind->length) = sizeof ( double );
				memcpy ( mbind->buffer, &value.dec64, sizeof ( double ) );
				break;

			case RXT_WORD:
				// the word "none" is passed as a generic RXT_WORD
				*(mbind->is_null) = 1;
				break;

			default:
				printf ( "C: unknown type\n" );
				*(mbind->is_null) = 1;
		}
	}

	return mbind_arr;
}
// }}}

/* {{{ R3MYSQL_fetch_row_prepared_stmt ( RXIFRM *frm )

Retrieves a single row of data.

Parameters:

- frm: Rebol command frame;

Returns a block containing the row data. Each field is represented by two
values: the first is a string containing the field name and the second is the
field value. The Rebol datatype of the field value depends on the field type.

Example:

    [ "field A" "value A" "field B" "value B" ... ]

Calls R3MYSQL_make_error on error.
*/
RXIEXT int R3MYSQL_fetch_row_prepared_stmt ( RXIFRM *frm ) {
	REBSER *database = NULL;
	MYSQL *conn = NULL;
	MYSQL_RES *result = NULL;
	MYSQL_STMT *mstmt = NULL;
	MYSQL_BIND *mbind_arr = NULL;
	MYSQL_BIND *mbind;
	MYSQL_FIELD *field = NULL;
	int res = 0;
	int dtype = 0;
	RXIARG value;
	int value_type = 0;
	int num_cols = 0;
	int k = 0;
	REBSER *rowblock = NULL;
	REBSER *block = NULL;
	REBSER *s = NULL;
	unsigned int i = 0;

	char DATETIME_STRING[] = "datetime";
	char TIME_STRING[] = "time";
	char DATE_STRING[] = "date";
	char *buffer_char = NULL;
	long long int *buffer_int = NULL;
	MYSQL_TIME *buffer_time = NULL;
	double *buffer_double = NULL;


	if ( ( res = R3MYSQL_get_database_connection ( frm, TRUE, &database, &conn ) ) != RXR_TRUE ) return res;

	// get the prepared statement of the last query
	dtype = RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-statement" ), &value );
	switch ( dtype ) {
		case RXT_NONE:
			return MAKE_ERROR ( "Invalid statement handle: none" );
			break;

		case RXT_HANDLE:
			mstmt = value.addr;
			break;

		default:
			return MAKE_ERROR ( "Invalid statement handle" );
	}

	dtype = RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), &value );
	result = value.addr;
	dtype = RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-result" ), &value );
	mbind_arr = value.addr;

	//printf ( "C: 1. mstmt: %p\n", mstmt );

	res = mysql_stmt_fetch ( mstmt );

	//printf ( "C: 2. res: %d\n", res );

	switch ( res ) {
		case 1: // error
			return MAKE_ERROR ( mysql_stmt_error ( mstmt ) );

		case MYSQL_NO_DATA:
			return RXR_NONE;
	}

	// transform the row in a Rebol block

	//printf ( "C: 3\n" );

	dtype = RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "num-cols" ), &value );
	// TODO: signal a proper error
	num_cols = ( dtype == RXT_INTEGER ) ? value.int64 : 0;

	//printf ( "C: 4. num_cols: %d\n", num_cols );

	rowblock = RL_MAKE_BLOCK ( num_cols * 2 );

	for ( k = 0; k < num_cols; k++ ) {
		field = mysql_fetch_field_direct ( result, k );

		//printf ( "C: 5. field->name: %s\n", field->name );

		// create the field name

		s = RL_MAKE_STRING ( field->name_length, 1 );
		string_utf8_to_rebol_unicode ( (uint8_t *) field->name, s );

		value.series = s;
		value.index = 0;
		RL_SET_VALUE ( rowblock, 2 * k, value, RXT_STRING );

		mbind = &mbind_arr [ k ];

		//printf ( "C: 6. mbind: %p\n", mbind );
		//printf ( "C: 6.1. mbind.is_null: %p\n", mbind->is_null );
		//printf ( "C: 6.2. mbind.error: %d\n", *mbind->error );

		if ( ! *mbind->is_null ) {
			// create the field value

			switch ( mbind->buffer_type ) {
				/* For the possibile buffer types see the documentation of the
				 * R3MYSQL_build_bind_for_result() function. */

				case MYSQL_TYPE_LONGLONG:
					//printf ( "C: 7. buffer_type: MYSQL_TYPE_LONGLONG\n" );

					buffer_int = (long long int *) mbind->buffer;
					value.int64 = *buffer_int;
					value_type = RXT_INTEGER;
					break;

				case MYSQL_TYPE_DOUBLE:
					buffer_double = (double *) mbind->buffer;
					value.dec64 = *buffer_double;
					value_type = RXT_DECIMAL;
					break;

				case MYSQL_TYPE_DATE:
					buffer_time = (MYSQL_TIME *) mbind->buffer;
					block = RL_MAKE_BLOCK ( 4 );
					s = RL_MAKE_STRING ( 4, 0 );
					for ( i = 0; i < 4; i++ ) RL_SET_CHAR ( s, i, DATE_STRING [ i ] );
					value.series = s;
					value.index = 0;
					RL_SET_VALUE ( block, 0, value, RXT_STRING );

					value.int64 = buffer_time->day;
					RL_SET_VALUE ( block, 1, value, RXT_INTEGER );
					value.int64 = buffer_time->month;
					RL_SET_VALUE ( block, 2, value, RXT_INTEGER );
					value.int64 = buffer_time->year;
					RL_SET_VALUE ( block, 3, value, RXT_INTEGER );

					value.series = block;
					value.index = 0;
					value_type = RXT_BLOCK;
					break;

				case MYSQL_TYPE_TIME:
					buffer_time = (MYSQL_TIME *) mbind->buffer;
					block = RL_MAKE_BLOCK ( 4 );
					s = RL_MAKE_STRING ( 4, 0 );
					for ( i = 0; i < 4; i++ ) RL_SET_CHAR ( s, i, TIME_STRING [ i ] );
					value.series = s;
					value.index = 0;
					RL_SET_VALUE ( block, 0, value, RXT_STRING );

					value.int64 = buffer_time->hour;
					RL_SET_VALUE ( block, 1, value, RXT_INTEGER );
					value.int64 = buffer_time->minute;
					RL_SET_VALUE ( block, 2, value, RXT_INTEGER );
					value.int64 = buffer_time->second;
					RL_SET_VALUE ( block, 3, value, RXT_INTEGER );

					value.series = block;
					value.index = 0;
					value_type = RXT_BLOCK;
					break;

				case MYSQL_TYPE_TIMESTAMP:
				case MYSQL_TYPE_DATETIME:
					//printf ( "C: 8. buffer_type: MYSQL_TYPE_DATETIME\n" );

					buffer_time = (MYSQL_TIME *) mbind->buffer;
					block = RL_MAKE_BLOCK ( 7 );
					s = RL_MAKE_STRING ( 8, 0 );
					for ( i = 0; i < 8; i++ ) RL_SET_CHAR ( s, i, DATETIME_STRING [ i ] );
					value.series = s;
					value.index = 0;
					RL_SET_VALUE ( block, 0, value, RXT_STRING );

					value.int64 = buffer_time->day;
					RL_SET_VALUE ( block, 1, value, RXT_INTEGER );
					value.int64 = buffer_time->month;
					RL_SET_VALUE ( block, 2, value, RXT_INTEGER );
					value.int64 = buffer_time->year;
					RL_SET_VALUE ( block, 3, value, RXT_INTEGER );
					value.int64 = buffer_time->hour;
					RL_SET_VALUE ( block, 4, value, RXT_INTEGER );
					value.int64 = buffer_time->minute;
					RL_SET_VALUE ( block, 5, value, RXT_INTEGER );
					value.int64 = buffer_time->second;
					RL_SET_VALUE ( block, 6, value, RXT_INTEGER );

					value.series = block;
					value.index = 0;
					value_type = RXT_BLOCK;
					break;

				default:
					//printf ( "C: 9. buffer_type: other\n" );
					//printf ( "C: 8.1. length: %d\n", *mbind->length );

					s = RL_MAKE_STRING ( *mbind->length, 1 );

					buffer_char = (char *) mbind->buffer;
					*( buffer_char + *mbind->length ) = '\0';

					if (
						( ( field->type == MYSQL_TYPE_STRING ) || ( field->type == MYSQL_TYPE_VAR_STRING ) || ( field->type == MYSQL_TYPE_BLOB ) )
						&&
						( field->flags & BINARY_FLAG )
					) {
						for ( i = 0; i < *mbind->length; i++ ) RL_SET_CHAR ( s, i, buffer_char [ i ] );
					} else {
						string_utf8_to_rebol_unicode ( (uint8_t *) buffer_char, s );
					}

					value.series = s;
					value.index = 0;
					value_type = RXT_STRING;
					break;
			}

		} else {
			value.int64 = 0;
			value_type = RXT_NONE;
		}

		RL_SET_VALUE ( rowblock, ( 2 * k ) + 1, value, value_type );
	}

	RXA_SERIES ( frm, 1 ) = rowblock;
	RXA_INDEX ( frm, 1 ) = 0;
	RXA_TYPE ( frm, 1 ) = RXT_BLOCK;
	return RXR_VALUE;
}
// }}}

/* {{{ R3MYSQL_free_prepared_stmt ( REBSER *database )

Frees all resources associated with a query.

Parameters:

- database: database object.

Returns nothing.
*/
void R3MYSQL_free_prepared_stmt ( REBSER *database ) {
	RXIARG value;
	char *sql = NULL;
	MYSQL_STMT *mstmt = NULL;
	MYSQL_BIND *mbind_arr = NULL;
	MYSQL_BIND *mbind;
	MYSQL_RES *result = NULL;
	int arr_length = 0;
	int i = 0;

	RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-sql-string" ), &value );
	sql = value.addr;
	if ( sql ) free ( sql );

	RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-statement" ), &value );
	mstmt = value.addr;
	if ( mstmt ) {
		// This releases the result data if it has been stored locally
		mysql_stmt_free_result ( mstmt );
		mysql_stmt_close ( mstmt );
	}

	RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-params" ), &value );
	mbind_arr = value.addr;
	RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-params-length" ), &value );
	arr_length = value.int64;

	if ( mbind_arr ) {
		for ( i = 0; i < arr_length; i++ ) {
			mbind = &mbind_arr [ i ];
			if ( mbind->buffer ) free ( mbind->buffer );
			if ( mbind->length ) free ( mbind->length );
			if ( mbind->is_null ) free ( mbind->is_null );
			if ( mbind->error ) free ( mbind->error );
		}

		free ( mbind_arr );
	}

	RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-result" ), &value );
	mbind_arr = value.addr;
	RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-result-length" ), &value );
	arr_length = value.int64;

	if ( mbind_arr ) {
		for ( i = 0; i < arr_length; i++ ) {
			mbind = &mbind_arr [ i ];
			if ( mbind->buffer ) free ( mbind->buffer );
			if ( mbind->length ) free ( mbind->length );
			if ( mbind->is_null ) free ( mbind->is_null );
			if ( mbind->error ) free ( mbind->error );
		}

		free ( mbind_arr );
	}

	// This frees the result metadata
	RL_GET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), &value );
	result = value.addr;
	if ( result ) mysql_free_result ( result );

	value.int64 = 0;
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-sql-string" ), value, RXT_NONE );
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-statement" ), value, RXT_NONE );
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-params" ), value, RXT_NONE );
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-params-length" ), value, RXT_INTEGER );
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-result" ), value, RXT_NONE );
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-bind-result-length" ), value, RXT_INTEGER );
	RL_SET_FIELD ( database, RL_MAP_WORD ( (REBYTE *) "int-result" ), value, RXT_NONE );
}
// }}}

