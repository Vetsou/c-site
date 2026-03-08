# Static server in C
This is a simple HTTP server implemented in C using sockets. It doesn't support most of the HTTP standard, but it is enough for serving static files.

## About
Inspired by the **[cerveurus](https://github.com/Kiyoshika/cerveurus)** project.
The HTTP parser was written by studying **[picohttpparser](https://github.com/h2o/picohttpparser/blob/master/picohttpparser.c)** as a reference.
This project was created for learning purposes and is not intended for use as a real server.
You are free to modify, fix, or use this code for any purpose.

## Usage
Clone the repo and then:
```bash
# Setup the project
make setup

# Build the project
make build

# Run the server
# You can simply use 'make run' as it will automatically set up and build the project
make run
```