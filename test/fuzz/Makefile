AFLCC           = afl-clang
AFLFUZZ         = afl-fuzz

ROOT_DIR        = ../..
INPUT_DIR       = afl_in
OUTPUT_DIR      = afl_out

BIN             = ciff
INPUT           = ${INPUT_DIR}/in_danger


all: ciff fuzz

fuzz: ${BIN} ${INPUT}
	@mkdir -p ${OUTPUT_DIR}
	@${AFLFUZZ} -i ${INPUT_DIR} -o ${OUTPUT_DIR} -- ./${BIN}

${INPUT}:
	@mkdir -p ${INPUT_DIR}
	@${ROOT_DIR}/test/gentest \
	    -c 'Very Dangerous' \
	    -t danger -t stripes \
	    -o ${INPUT} \
	    danger 200 200

ciff:
	@make -C ${ROOT_DIR} clean
	@make -C ${ROOT_DIR} CC=${AFLCC} ${BIN}
	@cp ${ROOT_DIR}/${BIN} ${BIN}

clean:
	rm -rf ${BIN} ${INPUT_DIR} ${OUTPUT_DIR}

.PHONY: fuzz clean
