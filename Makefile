CXX = clang++
CXXFLAGS = -std=c++17 -I/opt/homebrew/include -Igameplay/include
LDFLAGS = -L/opt/homebrew/lib -lSDL2main -lSDL2 -Wl,-framework,Cocoa

TARGET = siege_supply

SRC = main.cpp gameplay/src/gameplay_ui.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)