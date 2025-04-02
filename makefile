SRC_DIR1 = helper

SRC_DIR2 = methods

SRC = server.cpp $(SRC_DIR2)/get.cpp $(SRC_DIR2)/post.cpp $(SRC_DIR2)/delete.cpp \
	$(SRC_DIR1)/generateResponse.cpp \
	$(SRC_DIR1)/getContentType.cpp $(SRC_DIR1)/parseRequest.cpp \
	$(SRC_DIR1)/establishingServer.cpp \
	$(SRC_DIR1)/processMethodNotAllowed.cpp

OBJ = $(SRC:.cpp=.o)

NAME = webserver

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror  -g3 -std=c++98

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
