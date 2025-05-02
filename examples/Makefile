EXE = gui no-gui

BIN_DIR = ./bin
ACCESS_DIR = ./access
SRC_DIR = ./src
IMGUI_DIR = ./include/imgui

SOURCES = $(SRC_DIR)/stb_impl.cpp \
		  $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp \
		  $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp \
		  $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
		  $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer2.cpp

OBJS = $(addprefix $(BIN_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I../. -I./include -I./include/stb
CXXFLAGS += -g -Wall -Wformat -Ofast
LIBS = -lSDL2_image

ifeq ($(UNAME_S), Linux)
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl `sdl2-config --libs`
	CXXFLAGS += `sdl2-config --cflags`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin)
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/local/lib
	CXXFLAGS += `sdl2-config --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`
	CXXFLAGS += `pkg-config --cflags sdl2`
	CFLAGS = $(CXXFLAGS)
endif

all: $(addprefix $(BIN_DIR)/, $(EXE))
	@echo Build complete for $(ECHO_MESSAGE)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Generic compile rule
$(BIN_DIR)/%.o: %.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR)/%.o: $(IMGUI_DIR)/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR)/%.o: $(IMGUI_DIR)/backends/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR)/stb_impl.o: $(SRC_DIR)/stb_impl.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR)/gui.o: $(SRC_DIR)/gui.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR)/no-gui.o: $(SRC_DIR)/no-gui.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR)/gui: $(BIN_DIR)/gui.o $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

$(BIN_DIR)/no-gui: $(BIN_DIR)/no-gui.o $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean-exe:
	rm -f $(addprefix $(BIN_DIR)/, $(EXE)) $(addprefix $(BIN_DIR)/, $(addsuffix .o, $(EXE)))

clean:
	rm -rf $(BIN_DIR)
