CXXFLAGS = -std=c++17 -Wall -Wextra -Igameplay/include
SRC = main.cpp $(wildcard gameplay/src/*.cpp)

ifeq ($(OS),Windows_NT)
    CXX = g++
    TARGET = siege_supply.exe
    LIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
    RM = del /Q
else
    CXX = clang++
    CXXFLAGS += -I/opt/homebrew/include
    TARGET = siege_supply
    LIBS = -L/opt/homebrew/lib -lSDL2main -lSDL2 -lSDL2_ttf -Wl,-framework,Cocoa
    RM = rm -f
endif

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	$(RM) $(TARGET)

.PHONY: clean
