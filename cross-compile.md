## Introduction
This is a tutorial on how to set up cross-compilation. Our main machine, where we are going to be developing on, can be any Linux machine and the executable should be able to run on a Raspberry Pi.

## Connect to Pi via SSH
- Copy rsa pub keys of your machine to the pi to be able to connect via fingerprint
- On the host machine, type:

```bash
hostname -I
```

in order to find the IP address of the pi.

- Connect from the main machine with one of the following commands:

```bash
ssh hostname.local
#or
ssh username@hostname
#or
ssh username@IP
``` 

## Cross-compiling on arm processors
- Visit the arm gnu toolchain website: [link](https://developer.arm.com/downloads/-/gnu-a)
- Find out the configuration of both your main and host machine by typing:

```bash
gcc -dumpmachine
```

in the command line.
- After downloading the appropriate cross-compiler based on the machines' configurations the next step is to add the bin folder with the compiler's executables to the path.

```bash
nano ~/.bashrc
# edit the following line to include the bin folder of your cross-compiler
export PATH=/path/to/cross-compiler:$PATH
#ctr-o , ctr-x to save and exit nano
source ~/.bashrc # to apply the changes inside the .bashrc file
```

- Now create a simple c file on the main machine to test the compiler:

```c
#include <stdio.h>

int main() {
    // Print "Hello, World!" to the console
    printf("Hello, World!\n");
    return 0;
}
```

- Compile it using the downloaded tools:

```bash
#cross-compiler from linux x86_64 to pi400
aarch64-none-linux-gnu-gcc file.c -o file
```

- Finally, the last step is to transfer this file to our raspberry pi:

```bash
scp /path/to/file/on/main/machine username@hostname:/path/to/copy/to/on/host
./file #to run the executable
```

