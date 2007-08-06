export FTB_HOME=../../../
export SRC_DIR=$(FTB_HOME)/src
export INC_DIR=$(FTB_HOME)/include
export UTILS_DIR=$(FTB_HOME)/utils
export EVENT_DIR=$(FTB_HOME)/etc
export EXAMPLE_DIR=$(PWD)
export BIN_DIR=$(FTB_HOME)/bin
export LIB_DIR=$(FTB_HOME)/lib

export EVENT_FILE=$(EVENT_DIR)/ftb_event_file
export BUILT_IN_EVENT_CODE_FILE_PREFIX=ftb_built_in_event_list
export CFLAGS=-I$(INC_DIR)
export FTBLDFLAGS=-L$(LIB_DIR)

all: server client examples

basic: ftb_basic

server: $(BIN_DIR)/ftb_server

client: $(LIB_DIR)/libftb.so $(LIB_DIR)/libftb_polling.so

examples: $(EXAMPLE_DIR)/ftb_watchdog $(EXAMPLE_DIR)/ftb_logger $(EXAMPLE_DIR)/ftb_event_thrower $(EXAMPLE_DIR)/ftb_event_lister $(EXAMPLE_DIR)/ftb_multi_callback_monitor

#===================================

ftb_basic: ftb_event ftb_conf

ftb_event: $(EVENT_FILE) $(UTILS_DIR)/ftb_tool_event_generator
	$(UTILS_DIR)/ftb_tool_event_generator $(EVENT_FILE); \
	mv $(BUILT_IN_EVENT_CODE_FILE_PREFIX).c $(SRC_DIR); \
	mv $(BUILT_IN_EVENT_CODE_FILE_PREFIX).h $(INC_DIR)

$(UTILS_DIR)/ftb_tool_event_generator: $(UTILS_DIR)/ftb_tool_event_list_generator.c
	gcc $(CFLAGS) -o $@ $<

ftb_conf:
	sh $(UTILS_DIR)/ftb_setup_conf.sh;\
	mv ftb_conf.h $(INC_DIR)
	
$(BIN_DIR)/ftb_server: ftb_basic $(SRC_DIR)/ftb_server.cpp $(SRC_DIR)/ftb_util.c
	g++ $(CFLAGS) -o $@ $(SRC_DIR)/ftb_server.cpp $(SRC_DIR)/ftb_util.c -lpthread

$(LIB_DIR)/libftb.so: $(SRC_DIR)/libftb.c $(SRC_DIR)/ftb_util.c ftb_basic
	gcc -fPIC -shared $(CFLAGS) -o $@ $(SRC_DIR)/libftb.c $(SRC_DIR)/ftb_util.c $(SRC_DIR)/$(BUILT_IN_EVENT_CODE_FILE_PREFIX).c -lpthread

$(LIB_DIR)/libftb_polling.so: $(SRC_DIR)/libftb.c $(SRC_DIR)/ftb_util.c ftb_basic
	gcc -fPIC -shared $(CFLAGS) -DFTB_CLIENT_POLLING_ONLY -o $@ $(SRC_DIR)/libftb.c $(SRC_DIR)/ftb_util.c $(SRC_DIR)/$(BUILT_IN_EVENT_CODE_FILE_PREFIX).c

$(EXAMPLE_DIR)/ftb_watchdog: ftb_basic $(EXAMPLE_DIR)/ftb_watchdog.c $(LIB_DIR)/libftb_polling.so
	gcc $(CFLAGS) $(FTBLDFLAGS) -o $@ $(EXAMPLE_DIR)/ftb_watchdog.c -lpthread -lftb_polling

$(EXAMPLE_DIR)/ftb_logger: ftb_basic $(EXAMPLE_DIR)/ftb_logger.c $(LIB_DIR)/libftb.so
	gcc $(CFLAGS) $(FTBLDFLAGS) -o $@ $(EXAMPLE_DIR)/ftb_logger.c -lpthread -lftb

$(EXAMPLE_DIR)/ftb_event_thrower: ftb_basic $(EXAMPLE_DIR)/ftb_event_thrower.c $(LIB_DIR)/libftb_polling.so
	gcc $(CFLAGS) $(FTBLDFLAGS) -o $@ $(EXAMPLE_DIR)/ftb_event_thrower.c -lftb_polling

$(EXAMPLE_DIR)/ftb_event_lister: ftb_basic $(EXAMPLE_DIR)/ftb_event_lister.c $(LIB_DIR)/libftb_polling.so
	gcc $(CFLAGS) $(FTBLDFLAGS) -o $@ $(EXAMPLE_DIR)/ftb_event_lister.c -lftb_polling

$(EXAMPLE_DIR)/ftb_multi_callback_monitor: ftb_basic $(EXAMPLE_DIR)/ftb_multi_callback_monitor.c $(LIB_DIR)/libftb.so
	gcc $(CFLAGS) $(FTBLDFLAGS) -o $@ $(EXAMPLE_DIR)/ftb_multi_callback_monitor.c -lpthread -lftb

#==========================

clean: clean_ftb clean_com1

clean_ftb:
	rm -f $(LIB_DIR)/libftb.so $(LIB_DIR)/libftb_polling.so;\
	rm -f ftb_conf $(INC_DIR)/ftb_conf.h;\
	rm -f $(SRC_DIR)/$(BUILT_IN_EVENT_CODE_FILE_PREFIX).c $(INC_DIR)/$(BUILT_IN_EVENT_CODE_FILE_PREFIX).h;\
	rm -f $(BIN_DIR)/ftb_server;\
	rm -f $(UTILS_DIR)/ftb_tool_event_generator;\
	rm -f $(EXAMPLE_DIR)/ftb_watchdog $(EXAMPLE_DIR)/ftb_logger $(EXAMPLE_DIR)/ftb_multi_callback_monitor\
	      $(EXAMPLE_DIR)/ftb_event_thrower $(EXAMPLE_DIR)/ftb_event_lister


#==========================

export COM1_DIR=$(FTB_HOME)/components/example_com1
export COM1_EVENT_FILE=$(COM1_DIR)/ftb_com1_event_file
export COM1_EVENT_CODE_FILE_PREFIX=ftb_com1_event_list
export COM1_INC=-I$(COM1_DIR)

com1: ftb_basic server client
	$(UTILS_DIR)/ftb_tool_event_generator $(COM1_EVENT_FILE) com1 65536; \
	mv $(COM1_EVENT_CODE_FILE_PREFIX).c $(COM1_DIR); \
	mv $(COM1_EVENT_CODE_FILE_PREFIX).h $(COM1_DIR); \
	gcc $(CFLAGS) $(COM1_INC) $(FTBLDFLAGS) -o $(COM1_DIR)/com1_thrower $(COM1_DIR)/com1_thrower.c $(COM1_DIR)/$(COM1_EVENT_CODE_FILE_PREFIX).c -lftb_polling ;\
	gcc $(CFLAGS) $(COM1_INC) $(FTBLDFLAGS) -o $(COM1_DIR)/com1_catcher $(COM1_DIR)/com1_catcher.c $(COM1_DIR)/$(COM1_EVENT_CODE_FILE_PREFIX).c -lftb
	
clean_com1:
	rm -f $(COM1_DIR)/com1_thrower $(COM1_DIR)/com1_catcher; \
	rm -f $(COM1_DIR)/$(COM1_EVENT_CODE_FILE_PREFIX).c $(COM1_DIR)/$(COM1_EVENT_CODE_FILE_PREFIX).h
