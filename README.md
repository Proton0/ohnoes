# ohnoes

macOS's Activity Monitor but without limitations and is a CLI tool

## Supported Platforms

- Linux
- macOS

you guys can make can port this to windows or any platform that supports ncurses, all the platform-specific code is in
`platform.c`

## Compilation
requirements

1. cmake
2. make
3. any C compiler
4. ncurses
--------
how 2 compile easy

if u have issues ask chatgpt


1. clone the repository
```bash
$ git clone https://github.com/proton0/ohnoes.git
```

2. cd into the directory
```bash
$ cd ohnoes
```

3. create a build directory
```bash
$ mkdir build && cd build
```

4. run cmake
```bash
$ cmake ..
```

5. run make
```bash
$ make
```

6. run the executable
```bash
$ ./ohnoes
```
u may need to run `chmod +x ohnoes` or smth idk
