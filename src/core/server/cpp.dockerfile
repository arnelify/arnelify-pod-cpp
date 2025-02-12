FROM gcc:14.2.0

WORKDIR /var/www/cpp

RUN apt-get update -y && apt-get upgrade -y
RUN apt-get install sudo nano clang ccache -y
RUN apt-get install libjsoncpp-dev -y

EXPOSE 3001