CFLAGS=-c -std=c99 -Wall -m64 -Ofast -flto -march=native -funroll-loops -DLINUX -D_GNU_SOURCE -D_VSYNC
LDFLAGS=-lEGL -lGLESv2 -lX11
SRCS=main.c eglx11.c
OBJS=$(SRCS:.c=.o)
TARGET=egl-1

all: $(SRCS) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -fr $(OBJS) $(TARGET)
