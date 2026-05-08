# Static server in C
This is a simple HTTP server implemented in C using sockets. It doesn't support most of the HTTP standard, but it is enough for serving static files.

## About
Inspired by the **[cerveurus](https://github.com/Kiyoshika/cerveurus)** project.
The HTTP parser was written by studying **[picohttpparser](https://github.com/h2o/picohttpparser/blob/master/picohttpparser.c)** as a reference.
This project was created for learning purposes and is not intended for use as a real server.
You are free to modify, fix, or use this code for any purpose.

## Configuration
You can use environment variables to customize the server.  
You can also place a `.env` file in the root folder to load environment variables for the server process only.

Example `.env` file:

```bash
# Path where logs should be saved on the local machine.
# Defaults to './logs.log' if not defined.
RT_LOGS_FILE_PATH=./my/logs/path/logs.log

# Path to the folder that will be hosted by the server.
# Defaults to './static' if not defined.
RT_STATIC_FILES_PATH=./path/my/site
```

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