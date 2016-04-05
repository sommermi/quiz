# Makefile für das Multiplayer-Quiz
# Stefan Gast, 2013, 2014, 2015, 2016
###############################################################################

#################
# Konfiguration #
#################

OUTPUT_TARGETS = bin/server bin/loader
DIALECT_OPTS = -Wall -std=gnu99

ARCH = $(shell uname -m)
CCARCH_32 = -march=i686
CCARCH_64 =

ifndef CFLAGS
CFLAGS = -pipe -ggdb
ifeq ($(ARCH),i686)
CFLAGS += $(CCARCH_32)
else
ifeq ($(ARCH),x86_64)
CFLAGS += $(CCARCH_64)
endif
endif
endif

# Folgende Zeile aktivieren, wenn vorgegebene Module des Servers verwendet werden sollen!
#LIBQUIZSERVER_PROVIDED = server/provided/libquizserverprovided-$(ARCH).a

CLEANFILES = server/*.o server/*.dep \
	     loader/*.o loader/*.dep \
	     common/*.o common/*.dep
MRPROPERFILES = $(OUTPUT_TARGETS)

###############################################################################

################################################
# Module der Programme Server, Client und Loader
################################################

SERVER_MODULES=server/catalog.o \
	       server/clientthread.o \
	       server/login.o \
	       server/main.o \
	       server/rfc.o \
	       server/score.o \
	       server/user.o \
	       common/util.o

LOADER_MODULES=loader/browse.o \
	       loader/load.o \
	       loader/main.o \
	       loader/parser.o \
	       loader/util.o \
	       common/util.o

###############################################################################

########################################
# Target zum Kompilieren aller Programme
########################################

.PHONY: all
all: $(OUTPUT_TARGETS)

###############################################################################

##################################################
# Includes für eventuell generierte Abhängigkeiten
##################################################

-include $(SERVER_MODULES:.o=.dep)
-include $(LOADER_MODULES:.o=.dep)

###############################################################################

#######################
# Targets zum Aufräumen
#######################

.PHONY: clean
clean:
	@for i in $(CLEANFILES) ; do \
		[ -f "$$i" ] && rm -v "$$i" ;\
	done ;\
	exit 0

.PHONY: mrproper
mrproper: clean
	@for i in $(MRPROPERFILES) ; do \
		[ -f "$$i" ] && rm -v "$$i" ;\
	done ;\
	exit 0

###############################################################################

##########################################
# Targets zum Linken von Server und Loader
##########################################

bin/server: $(SERVER_MODULES) $(LIBQUIZSERVER_PROVIDED)
	$(CC) -pthread -o $@ $^ -lrt

bin/loader: $(LOADER_MODULES)
	$(CC) -pthread -o $@ $^ -lrt

###############################################################################

###########################################################
# Kompilieren der Module und Erzeugen der Abhängigkeiten
# (siehe http://scottmcpeak.com/autodepend/autodepend.html)
###########################################################

%.o: %.c
	$(CC) -c -I. $(CFLAGS) $(DIALECT_OPTS) -pthread -o $@ $<
	@$(CC) -MM -I. $(CFLAGS) -pthread $< > $*.dep
	@mv -f $*.dep $*.dep.tmp
	@sed -e 's|.*:|$*.o:|' < $*.dep.tmp > $*.dep
	@sed -e 's/.*://' -e 's/\\$$//' < $*.dep.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.dep
	@rm -f $*.dep.tmp
