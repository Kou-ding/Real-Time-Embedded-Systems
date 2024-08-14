### Introduction
This is a tutorial on how to set up cross-compilation. Our main machine, where we are going to be developing on, can be any Linux machine and the executable should be able to run on a Raspberry Pi.

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
Visit the arm gnu toolchain website: [link](https://developer.arm.com/downloads/-/gnu-a)

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
#cross-compiler from linux x86_64 to pi400
aarch64-none-linux-gnu-gcc file.c -o file
```

#### Part 6
Finally, the last step is to transfer this file to our raspberry pi:
```bash
scp /path/to/file/on/main/machine username@hostname:/path/to/copy/to/on/host
./file #to run the executable
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
#zlib
wget https://zlib.net/current/zlib.tar.gz
mkdir zlib && mv zlib.tar.gz zlib
cd zlib
tar -xf zlib.tar.gz

#openssl
git clone git@github.com:openssl/openssl.git

#libwebsockets
git clone https://libwebsockets.org/repo/libwebsockets

```

#### Part 3
Now we have to build each library based on the host system (Raspberry Pi). This is done by: 
- openssl
```bash
# when inside the openssl directory
CC=/home/kou/coding/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc ./Configure linux-aarch64 --prefix=/home/kou/coding/Real-Time-Embedded-Systems/openssl --openssldir=/home/kou/coding/Real-Time-Embedded-Systems/openssl LDFLAGS="-static"
make 
sudo make install
```

- zlib
```bash
# when inside the zlib directory
CC=/home/kou/coding/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc ./configure --static --prefix=/home/kou/coding/Real-Time-Embedded-Systems/zlib
make 
sudo make install
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
-DCMAKE_C_COMPILER=/home/kou/coding/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc \
-DCMAKE_CXX_COMPILER=/home/kou/coding/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-g++ \
-DOPENSSL_ROOT_DIR=/home/kou/coding/Real-Time-Embedded-Systems/openssl \
-DOPENSSL_LIBRARIES=/home/kou/coding/Real-Time-Embedded-Systems/openssl/lib \
-DOPENSSL_INCLUDE_DIR=/home/kou/coding/Real-Time-Embedded-Systems/openssl/include \
-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
-DCMAKE_INSTALL_PREFIX=/home/kou/coding/Real-Time-Embedded-Systems/libwebsockets \
-DLWS_WITH_STATIC=ON \
-DLWS_WITH_SHARED=OFF

make

make install
```
