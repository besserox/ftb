all: $(bindir)/ftb_agent

CFLAGS = $(CFLAGS_SRC) -L$(MANAGER_LIB_PATH_STATIC) 
LIBS   = -l$(MANAGER_LIB_NAME) $(LIBS_PTHREAD)

clean:
	rm -f $(bindir)/ftb_agent

$(bindir)/ftb_agent: ftb_agent.c $(MANAGER_LIB_PATH_STATIC)/lib$(MANAGER_LIB_NAME).a
	$(CC) $(CFLAGS) -o $@ ftb_agent.c $(LIBS) 
