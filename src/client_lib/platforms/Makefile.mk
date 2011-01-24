if PLATFORM_IS_LINUX
include src/client_lib/platforms/linux/Makefile.mk
endif
if PLATFORM_IS_CRAY
include src/client_lib/platforms/linux/Makefile.mk
endif
if PLATFORM_IS_BGP
include src/client_lib/platforms/linux/Makefile.mk
#include src/client_lib/platforms/bg/Makefile.mk
endif
