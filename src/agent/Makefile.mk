sbin_PROGRAMS += ftb_agent

ftb_agent_SOURCES = $(top_srcdir)/src/agent/ftb_agent.c
ftb_agent_CFLAGS = -I${top_srcdir}/src/util
ftb_agent_LDADD = libftbm.la
