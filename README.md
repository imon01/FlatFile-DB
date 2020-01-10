# Description
A simple file-based database with a command-line interface for the Linux
platform. The database will consist of unique IDs (8 byte unsigned integers). The
command-line interface will allow three commands ‘insert’, ‘find’, ‘delete’.

Usage:

`dbcli <db filename> insert <id>`

inserts the number `<id>` into the database in `<db filename>`

`dbcli <db filename> find <id>`

Search for `<id>` in the database in `<db filename>`. If found, return success exit code. If not
found return failure exit code and print a meaningful error message to stderr.

`dbcli <db filename> delete <id>`

Removing the id number `<id>` from the database `<db filename>`. If a number is removed, a
subsequent ‘find’ should fail.
