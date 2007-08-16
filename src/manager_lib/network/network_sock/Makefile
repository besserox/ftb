all: $(bindir)/ftb_database_server ftb_bootstrap_udp.o ftb_network_tcp.o

CFLAGS = $(CFLAGS_MANAGER)

clean:
	rm -f $(bindir)/ftb_database_server
	rm -f ftb_bootstrap_udp.o ftb_network_tcp.o

$(bindir)/ftb_database_server: ftb_database_server_udp.c
	$(CC) $(CFLAGS) -o $@ $< $(UTIL_OBJ)

ftb_bootstrap_udp.o: ftb_bootstrap_udp.c
	$(CC) $(CFLAGS) -c $<

ftb_network_tcp.o: ftb_network_tcp.c
	$(CC) $(CFLAGS) -c $<
