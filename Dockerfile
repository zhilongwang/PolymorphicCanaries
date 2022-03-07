FROM debian:buster

RUN apt update
RUN apt install software-properties-common -y
RUN apt-get install gnupg2 -y
RUN add-apt-repository -s -y 'deb http://ftp.us.debian.org/debian/ jessie main contrib non-free'
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 7638D0442B90D010 
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys CBF8D6FD518E17E1
RUN apt-get update
RUN apt install gcc-4.9 g++-4.9 gcc-4.9-plugin-dev build-essential git -y
WORKDIR "/home/"
RUN git clone https://github.com/zhilongwang/PolymorphicCanaries.git


# WORKDIR "/home/PolymorphicCanaries/GCC_PLUGIN/"
# RUN make clean 
# RUN make

WORKDIR "/home/PolymorphicCanaries/testsets"
RUN apt-get install libpcre3 libpcre3-dev zlib1g-dev wget -y
RUN wget http://nginx.org/download/nginx-1.4.0.tar.gz
RUN tar -xvf nginx-1.4.0.tar.gz
WORKDIR "/home/PolymorphicCanaries/testsets/nginx-1.4.0"
RUN CC=gcc-4.9 ./configure
RUN sed -Ei 's|CFLAGS =|CFLAGS =-fstack-protector-all -fplugin=/home/PolymorphicCanaries/GCC_PLUGIN/PolymorphicCanaries.so|g' objs/Makefile
RUN make -f objs/Makefile -j32
