TESTS != ls test-*

all: ${TESTS}
	@for t in ${TESTS}; do \
	    echo \\n================ $$t ================; \
	    ./$$t; \
	done
