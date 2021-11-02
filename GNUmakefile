LIB             = libciff.a
CLI             = ciff

SRCS            = ciff.c cli.c
OBJS            = ${SRCS:.c=.o}
LIBS            = -ljpeg

CFLAGS         += -O2 -pipe
CFLAGS         += -Wall
CFLAGS         += -Wstrict-prototypes -Wmissing-prototypes
CFLAGS         += -Wmissing-declarations
CFLAGS         += -Wshadow -Wpointer-arith
CFLAGS         += -Wsign-compare -Wcast-qual

LDFLAGS        += ${LIBS}

PREFIX         ?= /usr/local

ifdef DEBUG
CFLAGS += -g -O0
endif


all: ${LIB} ${CLI}

${LIB}: ${OBJS}
	${AR} rcs ${LIB} ciff.o

${CLI}: ${OBJS}
	${CC} -o ${CLI} ${CFLAGS} ${LDFLAGS} ${OBJS}

.PHONY: install-lib
install-lib: ${LIB}
	install ${LIB} ${PREFIX}/lib/

.PHONY: install-cli
install-cli: ${CLI}
	install ${CLI} ${PREFIX}/bin/

.PHONY: deinstall-lib
deinstall-lib:
	rm -f ${PREFIX}/lib/${LIB}

.PHONY: deinstall-cli
deinstall-cli:
	rm -f ${PREFIX}/bin/${CLI}

.PHONY: install
install: install-lib install-cli

.PHONY: deinstall
deinstall: deinstall-lib deinstall-cli

.PHONY: clean
clean:
	rm -f ${LIB} ${CLI} ${OBJS}
