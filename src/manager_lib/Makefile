all: ftb_network ftb_manager_lib.o ftb_event_table.o libs 

export CFLAGS_MANAGER     = $(CFLAGS_SRC) -I$(PWD)/include -I$(PWD)/network/include -I$(PWD)/network/network_$(network)/include $(FTB_NETWORK_CFLAG)

CFLAGS = $(CFLAGS_MANAGER)

libs: $(MANAGER_LIB_PATH_STATIC)/lib$(MANAGER_LIB_NAME).a $(MANAGER_LIB_PATH_SHARED)/lib$(MANAGER_LIB_NAME).so

$(MANAGER_LIB_PATH_SHARED)/lib$(MANAGER_LIB_NAME).so: ftb_network ftb_manager_lib.o ftb_event_table.o
	$(CC) -shared -o $@ $(UTIL_OBJ) ftb_manager_lib.o ftb_event_table.o $(PWD)/network/network_$(network)/*.o

$(MANAGER_LIB_PATH_STATIC)/lib$(MANAGER_LIB_NAME).a: ftb_network ftb_manager_lib.o ftb_event_table.o
	$(AR) -rc $@ $(UTIL_OBJ) ftb_manager_lib.o ftb_event_table.o $(PWD)/network/network_$(network)/*.o
	$(RANLIB) $@

clean:
	rm -f $(MANAGER_LIB_PATH_STATIC)/lib$(MANAGER_LIB_NAME).a $(MANAGER_LIB_PATH_SHARED)/lib$(MANAGER_LIB_NAME).so 
	rm -f ftb_manager_lib.o ftb_event_table.o
	(cd network/network_$(network) && $(MAKE) clean)

ftb_manager_lib.o: ftb_manager_lib.c
	$(CC) $(CFLAGS) -c $<

ftb_event_table.o: ftb_event_table.c
	$(CC) $(CFLAGS) -c $<

ftb_network:
	(cd network/network_$(network) && $(MAKE))
