    FROM ubuntu:latest

RUN apt-get update -y && apt-get install -y \
    git \
    g++ \
    make \
    fish \
    zsh
COPY . /Desktop/webserve
