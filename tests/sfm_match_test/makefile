# TinyEngine Makefile
# Compiler Configuration

CC = g++ -std=c++17
CF = -Wfatal-errors -O3
LF = -I/usr/local/include -L/usr/local/lib -I/usr/include/opencv4

# General Linking

TINYLINK = -lpthread -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lGLEW -lboost_system -lboost_filesystem
OPENCVLINK = -lopencv_core -lopencv_calib3d

# OS Specific Linking

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)			#Detect GNU/Linux
TINYOS = -lX11 -lGL
endif
ifeq ($(UNAME), Darwin)			#Detext MacOS
TINYOS = -framework OpenGL
endif

all: main.cpp
			$(CC) main.cpp $(CF) $(LF) -lTinyEngine $(OPENCVLINK) $(TINYOS) $(TINYLINK) -o main

# Build Embedded Form

DAT = shader			#data directory to embed
.PHONY: embedded
embedded: CEF = $(shell c-embed $(DAT)) c-embed.o -include /usr/local/include/c-embed.h -DCEMBED_TRANSLATE
embedded:
	$(CC) main.cpp $(CF) $(LF) -lTinyEngine $(TINYOS) $(TINYLINK) -o main
	rm c-embed.o
