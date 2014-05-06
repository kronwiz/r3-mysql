r3-mysql
========

MySQL extension for Rebol3

## About

This extension allows Rebol 3 to connect to the MySQL database.

So far it has been built and tested only under Linux.


## Build

### Dependencies

On your system you should have MySQL and Rebol C headers.

If you installed MySQL using a package manager on Linux, you have to install MySQL **devel** package.

To get Rebol headers you have to download the source tree from here: https://github.com/rebol/rebol. You'll find them in the `src/include` directory.


### Building

Before building you have to check that the path pointing to the Rebol 3 C headers is correct:

1. enter the `src` directory;
2. open the `Makefile`;
3. substitute the correct path in the `INCLUDES` variable keeping the `-I` at the beginning.

Then return to the main directory and type `make`. You should find the `mysqldb.so` file in the same directory.


## Test

To test that the extension is working correctly there's the `test` directory. You'll find there two files:

1. `create_test_db.sql`: contains the SQL instructions to create the db used by the test script;
2. `test.r`: if you run this in `r3` after having built the extension you should get some results from the database.


## Usage

First you have to create the database object:

    db: make mysql/database []

Then you connect to the database:

    db/connect "localhost" "test" "test" "test"

To execute queries there's the `execute` method:

    db/execute "select * from addressbook"

If the query was a `select` you have to fetch the result set. You fetch it a row at a time like this:

    row: db/fetch-row

The `row` is a series of field name, field value. For example, from the test database:

    ["userid" "1" "firstname" "A" "lastname" "B" "phone" "123456" "address" "route" "66" "city" "C" "zip" "9876" "state" "Nowhere" "created" "2014-04-30 20:33:54"]

This plays well with `select` so that to retrieve a field value you can do:

    address: select row "address"

Note that all fields are returned as strings. **This may change in the future: the idea is to map MySQL datatypes to Rebol datatypes when possible.** The `NULL` value is returned as `none`.

When there are no more rows `db/fetch-row` returns `none`.

To close the connection to the database you use:

    db/close

Remember to always do that before exiting the program, otherwise the server side connection will hang on until MySQL timeout expires.

Queries other than "select" are always executed with th `execute` method. For example, an insert query:

    db/execute "insert into addressbook values( NULL, 'First', 'Last', '333.123.444', 'via di qua', 'non so', '9876', 'Italy', now() )"

or a delete query:

    db/execute "delete from addressbook where userid = '11'"

After a query the `num-rows` attribute always contains the number of rows affected by the operation; in the case of a "select" query this is the number of rows returned by the select.


### Other attributes and methods

The `database` object has the following **readonly** attributes:

- `num-rows`: number of affected rows. It is filled after a query;
- `num-cols`: number of fields in the result set. It is filled after a `select` query;
- `autocommit`: is `true` if autocommit is enabled (default), otherwise `false`.

Beside the methods highlighted before, the `database` object has also the following:

- `set-autocommit [ enabled ]`: enables or disables autocommit. The `enabled` parameter must be of type `logic!`.


### Error handling

Every method raises an error creating an `error!` object, so that to check for an error you use the common `try` pattern. For example:

    either error? res: try [ db/execute "select * from foobar" ] [
      print res
    ] [
      print [ "num-rows:" db/num-rows ]
      print [ "num-cols:" db/num-cols ]
    ]

Printing the `error!` object you get the error message.


## License

This library is copyright by Andrea Galimberti. This library is under the GNU LESSER GENERAL PUBLIC LICENSE Version 3. For more information about using/distributing the library see the `LICENSE` file or go to http://www.gnu.org/copyleft/lesser.html.

The above copyright notice, the licence and the following disclaimer shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES, EXPRESSED OR IMPLIED, WITH REGARD TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

