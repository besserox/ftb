all: $(FTB_LIB_PATH_STATIC)/lib$(FTB_LIB_BGL_CN_NAME).a ftb_zoid_client

CFLAGS = -Wall -g -I$(INC_DIR) -I$(PWD)/zoid_client

ZOID_CN_LIB = zoid_client/libftb_zoid_client_blrts.a


clean:
	rm -f $(FTB_LIB_PATH_STATIC)/lib$(FTB_LIB_BGL_CN_NAME).a
	rm -f ftb_zoid_wrapper.o
	(cd zoid_client && $(MAKE) clean)


ftb_zoid_client:
	@if [ -z "$(ZOID_HOME)" ]; then \
		echo "ZOID_HOME is not set!  Aborting!"; \
		exit 1; \
	fi
	@if [ -z "$(BLRTS_PATH)" ]; then \
		echo "BLRTS_PATH is not set!  Aborting!"; \
		exit 1; \
	fi
	(cd zoid_client && $(MAKE))
	

$(FTB_LIB_PATH_STATIC)/lib$(FTB_LIB_BGL_CN_NAME).a: ftb_zoid_wrapper.o ftb_zoid_client
	mkdir -p $(FTB_LIB_PATH_STATIC)
	cp -p $(ZOID_CN_LIB) $@
	$(AR) -rc $@ ftb_zoid_wrapper.o
	$(RANLIB) $@

ftb_zoid_wrapper.o: ftb_zoid_wrapper.c
	@if [ -z "$(ZOID_HOME)" ]; then \
		echo "ZOID_HOME is not set!  Aborting!"; \
		exit 1; \
	fi
	$(CC) $(CFLAGS) -I$(ZOID_HOME)/include -c $<
