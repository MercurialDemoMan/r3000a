disass:
	clang -O0 -std=c++2a disass.cpp ../PSXExecutable.cpp ../File.cpp -o disass

psxexe:
	clang -O0 -std=c++2a psxexe.cpp ../PSXExecutable.cpp ../File.cpp -o psxexe

cpu:
	clang -O0 -std=gnu++20 -Wno-pragma-pack -I../deps/include -L../deps/lib -lSDL2 -lOpenGL -D__LNX__ cpu.cpp ../PSXExecutable.cpp ../File.cpp ../CPU.cpp ../MMU.cpp ../DMA.cpp ../GPU.cpp ../Renderer.cpp ../ShaderProgram.cpp -Wno-initializer-overrides -o cpu

all: cpu disass psxexe

clean:
	rm disass psxexe cpu
