# Executable name as required by the subject 
NAME        := ircserv

# Compiler and flags
CXX         := c++
# -std=c++98 is mandatory 
CXXFLAGS    := -Wall -Wextra -Werror -std=c++98 -Iinc

# Directories
SRC_DIR     := src
OBJ_DIR     := obj

# Source files list (add more as the project grows)
SRC_FILES   :=	main.cpp Server.cpp Channel.cpp \
				commands/Join.cpp commands/Nick.cpp commands/Pass.cpp commands/Privmsg.cpp commands/User.cpp \
				commands/Kick.cpp commands/Part.cpp commands/Quit.cpp
SRC         := $(addprefix $(SRC_DIR)/, $(SRC_FILES))
OBJ         := $(addprefix $(OBJ_DIR)/, $(SRC_FILES:.cpp=.o))
DEP         := $(OBJ:.o=.d)

# Default rule
all: $(NAME)

# Linking object files
$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinc -MMD -MP -c $< -o $@

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Include dependency files to track header modifications
-include $(DEP)

# Remove object and dependency files
clean:
	rm -rf $(OBJ_DIR)

# Remove objects and the executable
fclean: clean
	rm -f $(NAME)

# Rebuild everything from scratch
re: fclean all

# Prevent conflicts with files of the same name 
.PHONY: all clean fclean re