FROM gcc:14.2.0

WORKDIR /var/www/pod

RUN apt-get update -y && apt-get upgrade -y
RUN apt-get install sudo tzdata nano wget gnupg curl zip unzip -y
RUN apt-get install inotify-tools -y

RUN ln -s /usr/bin/gfortran-12 /usr/bin/gfortran
RUN apt-get install libjsoncpp-dev libboost-all-dev libmagic-dev -y

ENV KAFKAJS_NO_PARTITIONER_WARNING=1
ENV TZ=Europe/Kiev

EXPOSE 3001
# EXPOSE 3002
# EXPOSE 8433