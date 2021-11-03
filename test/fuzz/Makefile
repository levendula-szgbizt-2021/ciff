AFLCC           = afl-clang
AFLFUZZ         = afl-fuzz

ROOT_DIR        = ../..
INPUT_DIR       = afl_in
OUTPUT_DIR      = afl_out

BIN             = ciff
INPUT           = random


all: fuzz

fuzz: ${BIN} ${INPUT}
	@mkdir -p ${OUTPUT_DIR}
	@${AFLFUZZ} -i ${INPUT_DIR} -o ${OUTPUT_DIR} -- ./${BIN}

compile:
	@make -C ${ROOT_DIR} clean
	@make -C ${ROOT_DIR} CC=${AFLCC} ciff
	@cp ${ROOT_DIR}/ciff ${BIN}

${INPUT}:
	@mkdir -p ${INPUT_DIR}/
	dd if=/dev/urandom of=${INPUT_DIR}/${INPUT} bs=1m count=1

.PHONY: fuzz compile
