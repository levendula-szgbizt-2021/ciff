LIB             = libciff.a
CLI             = ciff

SRCS            = ciff.c cli.c
OBJS            = ${SRCS:.c=.o}

CFLAGS         += -O2 -pipe
CFLAGS         += -Wall
CFLAGS         += -Wstrict-prototypes -Wmissing-prototypes
CFLAGS         += -Wmissing-declarations
CFLAGS         += -Wshadow -Wpointer-arith
CFLAGS         += -Wsign-compare -Wcast-qual

.ifdef DEBUG
CFLAGS += -g -O0
.endif


all: ${LIB} ${CLI}

${LIB}: ${OBJS}
	${AR} rcs ${LIB} ciff.o

${CLI}: ${OBJS}
	${CC} -o ${CLI} ${CFLAGS} ${LDFLAGS} ${OBJS}

.PHONY: clean
clean:
	rm -f ${LIB} ${CLI} ${OBJS}
