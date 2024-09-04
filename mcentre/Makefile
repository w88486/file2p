# Makefile for the project

CC = g++  # 指定编译器为 g++
CFLAGS = -g -Wall -std=c++11  # 编译选项：显示所有警告并使用 C++11 标准 生成调试信息
SOURCES = $(wildcard *.cpp) $(wildcard kernel/*.cpp)  # 所有 .cpp 文件，包括 kernel 目录下的 .cpp 文件
EXECUTABLE = filep2pcentre  # 最终生成的可执行文件名称
MODOULES = hiredis # 依赖的模块

all: $(EXECUTABLE)  # 主目标，依赖于所有 .cpp 文件
$(EXECUTABLE): $(SOURCES)  # 生成可执行文件的规则
	$(CC) $(CFLAGS) -o $@ $^ -l $(MODOULES) # 编译并链接所有 .cpp 文件生成可执行文件

clean:  # 清理目标
	rm -f $(EXECUTABLE)  # 删除所有 .o 文件和可执行文件

.PHONY: all clean  # 声明虚拟目标
