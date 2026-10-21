CC       = gcc
CXX      = g++
CFLAGS   = -std=c11   -O2 -Wall -Wextra -Iengine/include -Iengine/src
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Iengine/include -Igameplay/include

ENGINE_SRC = $(wildcard engine/src/*.c)
GAME_SRC   = main.cpp $(wildcard gameplay/src/*.cpp)
ENGINE_OBJ = $(ENGINE_SRC:.c=.o)
GAME_OBJ   = $(GAME_SRC:.cpp=.o)

ifeq ($(OS),Windows_NT)
    TARGET = siege_supply.exe
    LIBS   = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
    RM     = del /Q
    CLEAN  = $(subst /,\,$(ENGINE_OBJ) $(GAME_OBJ)) $(TARGET)
else
    UNAME := $(shell uname -s)
    ifeq ($(UNAME),Darwin)
        CXX = clang++
        CC  = clang
    endif
    HAS_PKG := $(shell pkg-config --exists sdl2 SDL2_ttf 2>/dev/null && echo yes)
    CXXFLAGS += -I/opt/homebrew/include -I/usr/local/include
    ifeq ($(HAS_PKG),yes)
        CXXFLAGS += $(shell pkg-config --cflags sdl2 SDL2_ttf)
        LIBS      = $(shell pkg-config --libs sdl2 SDL2_ttf)
    else
        LIBS      = -L/opt/homebrew/lib -L/usr/local/lib -lSDL2main -lSDL2 -lSDL2_ttf
    endif
    TARGET = siege_supply
    RM     = rm -f
    CLEAN  = $(ENGINE_OBJ) $(GAME_OBJ) $(TARGET)
endif

$(TARGET): $(ENGINE_OBJ) $(GAME_OBJ)
	$(CXX) $^ -o $@ $(LIBS)

$(GAME_OBJ): $(wildcard gameplay/include/*.h) $(wildcard engine/include/*.h)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(CLEAN)

.PHONY: clean