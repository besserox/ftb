all: $(clients)

export CFLAGS_CLIENT  = $(CFLAGS_SRC) -I$(PWD)/include
export CLIENT_LIB_OBJ = $(PWD)/ftb_client_lib.o

CFLAGS = $(CFLAGS_CLIENT)

clean:
	rm -f ftb_client_lib.o
	(cd platforms/Linux && $(MAKE) clean)
	(cd platforms/BGL && $(MAKE) clean)

BGL: ftb_client_lib.o
	(cd platforms/BGL && $(MAKE)) 
	

Linux: ftb_client_lib.o 
	(cd platforms/Linux && $(MAKE))


ftb_client_lib.o: ftb_client_lib.c
	$(CC) $(CFLAGS) -c $<

