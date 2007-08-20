PREFIX = ftb_zoid_client
CFLAGS = $(CFLAGS_CLIENT) -I.. -I$(ZOID_HOME)/include
CFLAGS_SHARED = -fPIC -mpowerpc -pthread
LDFLAGS = -shared

all: $(PREFIX)_impl.so $(FTB_LIB_PATH_SHARED)/$(PREFIX)_impl.so

$(FTB_LIB_PATH_SHARED)/$(PREFIX)_impl.so: $(PREFIX)_impl.so
	cp -p $(PREFIX)_impl.so $@

$(PREFIX)_impl.so: $(UTIL_OBJ) ftb_zoid_client.o
	$(CC) $(CFLAGS_SHARED) $(LDFLAGS) -o $@ $(CLIENT_LIB_OBJ) $(UTIL_OBJ) ftb_zoid_client.o $(MANAGER_LIB_PATH_STATIC)/lib$(MANAGER_LIB_NAME).a

ftb_zoid_client.o: ftb_zoid_client.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(FTB_LIB_PATH_SHARED)/$(PREFIX)_impl.so
	rm -f $(PREFIX)_impl.so *.o *~

distclean: clean

