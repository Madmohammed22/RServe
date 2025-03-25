SRC = server.cpp generateResponse.cpp post.cpp
OBJ = $(SRC:.cpp=.o)
NAME = webserver
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror  -g3 #-std=c++98 #-fsanitize=address

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f ${OBJ}

fclean: clean
	rm -f ${NAME}
re: fclean all

.PHONY: all clean fclean re