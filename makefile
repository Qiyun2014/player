
CC = g++
CFLAGS  := -Wall -03 -std=c++0x

OPENCV_INC_ROOT = /usr/local/Cellar/opencv@2/2.4.13.7_5/include/opencv2
OPENCV_LIB_ROOT = /usr/local/Cellar/opencv@2/2.4.13.7_5/lib

OBJS = player.o
LIB = libPlayer.a

OPENCV_INC = -I $(OPENCV_INC_ROOT)

INCLUDE_PATH = $(OPENCV_INC)

LIB_PATH = -L $(OPENCV_LIB_ROOT)

# Depend lib name
OPENCV_LIB = -lopencv_objdetect -lopencv_core -lopencv_highgui -lopencv_imgproc

all : $(LIB)

# gen .o file
%.o : %.cpp
    $(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_PATH) $(LIB_PATH) $(OPENCV_LIB)


# gen static file
$(LIB) : $(OBJS)
    rm -rf $@
    ar cr $@ $(OBJS)
    rm -rf $(OBJS)


tags :
    ctags -R *

clean:
    rm -rf $(OBJS) $(TARGET) $(LIB)




SRCS=main.cpp\
        src/player/stream_parse.cpp\
        src/player/stream_parse.h\
        src/player/player.cpp\
        src/player/player.h\
        src/player/Decode/decoder.cpp\
        src/player/Decode/decoder.h\
        src/player/Display/GLRender.cpp\
        src/player/Display/GLRender.h\
        src/player/Decode/AudioDecoder.cpp\
        src/player/Decode/AudioDecoder.h\
        src/player/Decode/VideoDecoder.cpp\
        src/player/Decode/VideoDecoder.h\

# 将所有的源文件编译成.o文件
OBJS=$(SRCS:.cpp=.o)

# 定义最后执行生成的文件变量值
EXEC=miancpp

# 开始执行，等同于执行 g++ -o maincpp .o
start:$(OBJS)
      $(CC) -o $(EXEC) $(OBJS)
      .cpp.o
      $(CC) -o $@ -c $< -DMYLINUX

# 执行make clean指令
clean:
    rm -rf $(OBJS)