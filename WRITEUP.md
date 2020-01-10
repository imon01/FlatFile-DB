
##Description
When I first read the description, I tought of simple system. Open and closing a file
on every command.

```
    ./dbcli <db name> <command> <value>
 ```
 
 However, after an initial development effort, I redesigned it. The APIs to db.c are the same; the major change is creating a service based system that manages multiple `DBs`. With this approaches, the underlying data structure for the database itself is a memory mapped filed.
 Communication between the `db service` and the `db client` is done via named pipes.
 
##Submission
Included are all source files. When running the `test` file, change the path for `UNITY_ROOT` in the makefile. 

####Db service
- db.c
- dbmanager.c

The name of executable will be `dbmanger`.
<br/>

####Db client
- db.c
- main.c

The name of executable will be `dbcli`.

