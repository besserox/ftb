all: ftb_agent ftb_manager_lib ftb_client_lib ftb_util

export CFLAGS_SRC              = $(CFLAGS_FTB) -I$(PWD)/include -I$(PWD)/manager_lib/include
export UTIL_OBJ                = $(PWD)/util/ftb_util.o
export MANAGER_LIB_PATH_SHARED = $(PWD)/manager_lib/lib/shared
export MANAGER_LIB_PATH_STATIC = $(PWD)/manager_lib/lib/static
export MANAGER_LIB_NAME        = ftbm

clean:
	(cd agent && $(MAKE) clean)
	(cd util && $(MAKE) clean)
	(cd manager_lib && $(MAKE) clean)
	(cd client_lib && $(MAKE) clean)

ftb_agent: ftb_manager_lib 
	(cd agent && $(MAKE))

ftb_manager_lib: ftb_util
	(cd manager_lib && $(MAKE))

ftb_client_lib: ftb_manager_lib
	(cd client_lib && $(MAKE))

ftb_util: 
	(cd util && $(MAKE))
