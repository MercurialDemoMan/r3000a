CC   = clang #required clang for gnu extensions
INC  = -I./deps/include
LIBS = -L./deps/libs \
	   -lSDL2 \
	   -lSDL2main \
	   -lglu32 \
	   -lopengl32 \
	   -lXInput \
	   -lglew32s \
	   -lglew32
FLG  = -Wall \
	   -Wextra \
	   -pedantic \
	   -g \
	   -O2 \
	   -std=gnu++2a \
	   -m32 \
	   -Wno-initializer-overrides \
	   -Wno-pragma-pack \
	   -Wno-deprecated-volatile \
	   -Wno-gnu-designator \
	   -Wno-c99-designator \
	   -Wno-gnu-anonymous-struct \
	   -Wno-language-extension-token \
	   -Wno-nested-anon-types \
	   -Wno-c99-extensions \
	   -Wno-gnu-case-range \
	   
	   
DEF  = -D__LNX__ -D_CRT_SECURE_NO_WARNINGS
OUT  = build\r3000a

all: $(OUT)

OUT_SRCS := $(wildcard *.cpp)
OUT_OBJS := $(patsubst %.cpp, %.o, $(OUT_SRCS))
OUT_DEPS := $(patsubst %.cpp, %.d, $(OUT_SRCS))

$(OUT): $(OUT_OBJS)
	$(CC) $^ $(LIBS) $(FLG) -o $(OUT)
	
-include $(OUT_DEPS)

./%.o: ./%.cpp
	$(CC) $(FLG) $(DEF) $(INC) -MMD -c $< -o $@
	
clean:	
	del /f $(OUT_OBJS) $(OUT_DEPS) $(OUT)
	
run: $(OUT)
	$(OUT)
	
	
	
	
	
	
