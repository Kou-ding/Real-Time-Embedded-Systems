### Introduction
This is a tutorial on how to set up cross-compilation. We are going to be building our project on an x86_64 Linux machine and the executable should be able to run on a Raspberry Pi aarch64.

### Connect to Pi via SSH
#### Part 1
Copy rsa pub keys of your machine to the pi to be able to connect via fingerprint

#### Part 2
On the host machine, type:
```bash
hostname -I
```
in order to find the IP address of the pi.

#### Part 3
Connect from the main machine with one of the following commands:
```bash
ssh hostname.local
#or
ssh username@hostname
#or
ssh username@IP
``` 

### Cross-compiling on arm processors
#### Part 1
Visit the arm gnu toolchain website: [link1](https://developer.arm.com/downloads/-/gnu-a) or the musl website: [link2](https://musl.cc/). Both contain toolchains that allow us to cross-compile. Musl has the benefit of not needing to link everything with glibc so it is preferred.

#### Part 2
Find out the configuration of both your main and host machine by typing:
```bash
gcc -dumpmachine
#or 
uname -m
```
in the command line.

#### Part 3
After downloading the appropriate cross-compiler based on the machines' configurations the next step is to add the bin folder with the compiler's executables to the path.
```bash
nano ~/.bashrc
# edit the following line to include the bin folder of your cross-compiler
export PATH=/path/to/cross-compiler:$PATH
#ctr-o , ctr-x to save and exit nano
source ~/.bashrc # to apply the changes inside the .bashrc file
```

#### Part 4
Now create a simple c file on the main machine to test the compiler:
```c
#include <stdio.h>
int main() {
    // Print "Hello, World!" to the console
    printf("Hello, World!\n");
    return 0;
}
```

#### Part 5
Compile it using the downloaded tools:
```bash
#cross-compiler from linux x86_64 to aarch64
aarch64-linux-musl-gcc file.c -o file -static
```

#### Part 6
Finally, the last step is to transfer this file to our raspberry pi:
```bash
# copy the executable
scp /path/to/file/on/build/machine username@hostname:/path/to/copy/to/on/host/machine
# run the executable
./file
```

### Advanced cross-compilation
#### Part 1
When it comes to linking libraries to our programs so that we have all the dependencies necessary to execute them, we have two options:
- Static linking and
- Dynamic linking.

The difference between the two is that in static linking the dependencies are included inside the executable while in dynamic linking they are separate and loaded by the operating system when the program runs. This is why we are going to be using **static linking** so that we can just run the program on the host machine without having to worry about how to link libraries from there when they might even not be available.

#### Part 2
Aiming to integrate the necessary libraries inside the executable, we build the libraries by ourselves. First, we acquire the source files.
```bash
#jansson
git clone git@github.com:akheron/jansson.git

#zlib
git clone git@github.com:madler/zlib.git

#openssl
git clone git@github.com:openssl/openssl.git

#libwebsockets
git clone git@github.com:warmcat/libwebsockets.git

```

#### Part 3
Now we have to build each library based on the host system (Raspberry Pi). The following scripts go through configuring, making and installing our dependencies.

- jansson
```bash
sudo dnf install libtool
autoupdate 
autoreconf -fi
CC=/home/kou/coding/Real-Time-Embedded-Systems/musl/aarch64-linux-musl-cross/bin/aarch64-linux-musl-gcc ./configure --build x86_64-pc-linux-gnu --host aarch64-linux-gnu --prefix /home/kou/coding/Real-Time-Embedded-Systems/jansson-build LDFLAGS="-static"
# make
make
# install 
make install
```

- zlib
```bash
# when inside the zlib directory
CC=/home/kou/coding/Real-Time-Embedded-Systems/musl/aarch64-linux-musl-cross/bin/aarch64-linux-musl-gcc ./configure --build x86_64-pc-linux-gnu --host aarch64-linux-gnu --prefix /home/kou/coding/Real-Time-Embedded-Systems/zlib-build LDFLAGS="-static"
# make
make
# install 
make install
```

- openssl
```bash
# when inside the openssl directory
CC=/home/kou/coding/Real-Time-Embedded-Systems/musl/aarch64-linux-musl-cross/bin/aarch64-linux-musl-gcc ./Configure linux-aarch64 --prefix=/home/kou/coding/Real-Time-Embedded-Systems/openssl-build no-shared -fPIC
# make
make
# install
make install
```

- libwebsockets
```bash
# when inside the libwebsocket directory
mkdir build 
cd build
# "\" is used to state that the command continues on the next line
# its purpose is better readability
cmake .. \
-DCMAKE_SYSTEM_NAME=Linux \
-DCMAKE_SYSTEM_PROCESSOR=aarch64 \
-DCMAKE_C_COMPILER=/home/kou/coding/Real-Time-Embedded-Systems/musl/aarch64-linux-musl-cross/bin/aarch64-linux-musl-gcc \
-DCMAKE_CXX_COMPILER=/home/kou/coding/Real-Time-Embedded-Systems/musl/aarch64-linux-musl-cross/bin/aarch64-linux-musl-g++ \
-DCMAKE_FIND_ROOT_PATH="/home/kou/coding/Real-Time-Embedded-Systems/musl/aarch64-linux-musl-cross/aarch64-linux-musl;/home/kou/coding/Real-Time-Embedded-Systems/openssl-build;/home/kou/coding/Real-Time-Embedded-Systems/zlib-build" \
-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
-DCMAKE_INSTALL_PREFIX=/home/kou/coding/Real-Time-Embedded-Systems/libwebsockets-build \
-DLWS_WITH_SSL=ON \
-DLWS_WITHOUT_TESTAPPS=ON \
-DLWS_WITHOUT_TEST_SERVER=ON \
-DLWS_WITHOUT_TEST_CLIENT=ON \
-DLWS_WITHOUT_EXTENSIONS=ON \
-DOPENSSL_ROOT_DIR=/home/kou/coding/Real-Time-Embedded-Systems/openssl-build \
-DOPENSSL_INCLUDE_DIR=/home/kou/coding/Real-Time-Embedded-Systems/openssl-build/include \
-DOPENSSL_CRYPTO_LIBRARY=/home/kou/coding/Real-Time-Embedded-Systems/openssl-build/lib/libcrypto.a \
-DOPENSSL_SSL_LIBRARY=/home/kou/coding/Real-Time-Embedded-Systems/openssl-build/lib/libssl.a \
-DOPENSSL_USE_STATIC_LIBS=TRUE \
-DZLIB_ROOT=/home/kou/coding/Real-Time-Embedded-Systems/zlib-build \
-DZLIB_LIBRARY=/home/kou/coding/Real-Time-Embedded-Systems/zlib-build/lib/libz.a \
-DZLIB_INCLUDE_DIR=/home/kou/coding/Real-Time-Embedded-Systems/zlib-build/include \
-DLWS_WITH_SHARED=OFF
# make
make
# install 
make install
```

#### Step 4 
Lastly, we link them inside the makefile to statically produce a program that runs on the host machine.

```makefile
CROSSLDFLAGS=-static \
-I/home/kou/coding/Real-Time-Embedded-Systems/libwebsockets-build/include \
-I/home/kou/coding/Real-Time-Embedded-Systems/jansson-build/include \
-I/home/kou/coding/Real-Time-Embedded-Systems/openssl-build/include \
-I/home/kou/coding/Real-Time-Embedded-Systems/zlib-build/include \
-L/home/kou/coding/Real-Time-Embedded-Systems/libwebsockets-build/lib \
-L/home/kou/coding/Real-Time-Embedded-Systems/jansson-build/lib \
-L/home/kou/coding/Real-Time-Embedded-Systems/openssl-build/lib \
-L/home/kou/coding/Real-Time-Embedded-Systems/zlib-build/lib \
-lwebsockets -lssl -lcrypto -lz -ljansson -ldl
```

The full Makefile is inside the repository for further reference.