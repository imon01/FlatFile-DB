BINDIR := $(shell pwd)
UNITY_ROOT := ../Unity#relative dir path, modify as necessary
TEST_INC_DIR := -I$(UNITY_ROOT)/src/ 

CC := gcc
CLIENT_SRC := main.c
SERV_SRC := dbmanager.c db.c
TEST_SRC := $(UNITY_ROOT)/src/unity.c test_db.c db.c

#CFLAGS := -std=c99
CLIENT := dbcli
SERVER := dbmanager
TEST_OBJ := test_db

ALL:= $(CLIENT) $(SERVER)
ALL-DEBUG:= $(CLIENT)-debug $(SERVER)-debug
TEST := $(TEST_OBJ)


all: $(ALL) 
test: 
all-debug: $(ALL-DEBUG) 

.PHONEY: display
display:
	$(info $$UNIT_ROOT is [${UNITY_ROOT}] )
	$(info $$TEST_INC_DIR is [${TEST_INC_DIR}] )
	$(info $$TEST_SRC is [${TEST_SRC}] )
	
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CLIENT_SRC) -o $@ 

$(CLIENT)-debug: $(CLIENT_SRC)
	$(CC) -g -D DEBUG=1 $(CLIENT_SRC) -o $@ 

$(SERVER): $(SERV_SRC)
	$(CC) $(SERV_SRC) -o $@ 

$(SERVER)-debug: $(SERV_SRC)
	$(CC) -g -D DEBUG=1 $(SERV_SRC) -o $@ 

test: 
	$(CC) $(TEST_INC_DIR) $(TEST_SRC)  -o $@ 	


.PHONEY: clean
clean:
	rm $(ALL)

.PHONEY: clean-test
clean-test:
	rm $(TEST) 

.PHONEY: clean-debug
clean-debug:
	rm $(ALL-DEBUG)

#writing the systemd file
.PHONEY: setup
setup:
	#writing the systemd file
	$(shell touch dbcli.service)
	$(shell echo "[Unit]" >> dbcli.service)
	$(shell echo "Description = dbcli service" >> dbcli.service)
	$(shell echo "[Service]" >> dbcli.service)
	$(shell echo "Type=forking" >> dbcli.service)
	$(shell echo "Restart=on-failure" >> dbcli.service)
	$(shell echo 'ExecStart=/'$(pwd)'/dbcli ' >> dbcli.service)
	$(shell echo 'KILLSIGNAL=SIGTERM' >> dbcli.service)
	$(info moving service file to /etc/ requires root) 
	$(shell sudo mv dbcli.service /etc/systemd/system/)
	#$(shell ls -l /etc/systemd/system/dbcli.service)

#start the service
start:
	$(info starting the dbcli service, will require root to start)
	$(shell sudo systemctl enable dbcli) 
	$(shell sudo systemctl start dbcli) 
	#$(shell  sudo systemctl enable foo-daemon)
