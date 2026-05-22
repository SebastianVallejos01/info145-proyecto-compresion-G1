# Compilador y banderas
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3

# Archivo de salida
TARGET = main

# Archivos fuente
SRCS = main.cpp

# Archivos objeto
OBJS = $(SRCS:.cpp=.o)

# Regla principal
all: $(TARGET)

#Cómo construir el ejecutable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Cómo construir los archivos objeto
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regla para limpiar los archivos compilados
clean:
	rm -f $(OBJS) $(TARGET)