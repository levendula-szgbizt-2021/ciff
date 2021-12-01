AFLCC           = afl-clang
AFLFUZZ         = afl-fuzz

ROOT_DIR        = ../..
INPUT_DIR       = afl_in
OUTPUT_DIR      = afl_out

BIN             = ciff


all: ciff fuzz

fuzz: ${BIN} ${INPUT}
	@mkdir -p ${OUTPUT_DIR}
	@${AFLFUZZ} -i ${INPUT_DIR} -o ${OUTPUT_DIR} -- ./${BIN}

ciff:
	@make -C ${ROOT_DIR} clean
	@make -C ${ROOT_DIR} CC=${AFLCC} ${BIN}
	@cp ${ROOT_DIR}/${BIN} ${BIN}

.PHONY: fuzz
