INC_DIR=./inc
OBJ_DIR=./obj
LIB_DIR=./lib
SRC_DIR=./src
BIN_DIR=./bin

CUTLERY_PATH=../cutlery

CC=gcc
RM=rm -f

TARGET=libexecutor.a

CFLAGS=-I${INC_DIR} -I${CUTLERY_PATH}/inc

LIBRARIES=-L${CUTLERY_PATH}/bin -lcutlery

${OBJ_DIR}/%.o : ${SRC_DIR}/%.c ${INC_DIR}/%.h
	${CC} ${CFLAGS} -c $< -o $@

${BIN_DIR}/$(TARGET) : ${OBJ_DIR}/example.o
	ar rcs $@ ${OBJ_DIR}/*.o ${LIBRARIES}

all: ${BIN_DIR}/$(TARGET)

clean :
	$(RM) $(BIN_DIR)/* $(OBJ_DIR)/*