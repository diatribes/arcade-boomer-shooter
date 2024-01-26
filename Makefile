TARGET = arcade-boomer-shooter 
LIBS = -lm `pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf`
CFLAGS= -Wall -std=c99 -pedantic -I ./include `pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf`
#CFLAGS= -Wall -O3 -std=c99 -pedantic -I ./include `pkg-config --cflags --libs sdl2`
LDFLAGS =
CC= gcc

.PHONY: default all clean

default: clean $(TARGET)
all: default

#OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
#HEADERS = $(wildcard *.h)

SRC = $(wildcard src/*.c) $(wildcard src/*/*.c)
OBJ = $(addprefix obj/,$(notdir $(SRC:.c=.o)))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.c | obj
	$(CC) $< -c $(CFLAGS) -o $@

obj/%.o: src/*/%.c | obj
	$(CC) $< -c $(CFLAGS) -o $@

obj:
	mkdir obj

.PRECIOUS: $(TARGET) $(OBJ)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -Wall $(LIBS) -o $@

clean:
	-rm -f ./obj/*.o
	-rm -f $(TARGET)

