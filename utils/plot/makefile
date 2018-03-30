TARGET  := plot
SRCDIR  := ./
INCDIR  := ../../include
SRC     := plot.cpp
OBJDIR  := obj
OBJ     := $(SRC:%.cpp=$(OBJDIR)/%.o)
OBJEXT  := ../../obj/src/GTM.o

CC      := g++
CFLAGS  := -I$(SRCDIR) -I$(INCDIR) -Wall -Wextra -O2 -std=c++11
LDFLAGS := -lFFTW3 -lsndfile

$(TARGET): $(OBJ)
	@echo "\033[1;33mLinking\033[0m $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(OBJEXT)
	@echo "\033[1;32mDone!\033[0m"

$(OBJDIR)/%.o: %.cpp
	@echo "\033[1;33mCompiling\033[0m $<"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "\033[1;33mRemoving\033[0m $(OBJDIR)"
	-@rm -r $(OBJDIR)

mrproper: clean
	@echo "\033[1;33mRemoving\033[0m $(TARGET)"
	-@rm $(TARGET)

all: mrproper $(TARGET)
