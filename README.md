# Project 2

This project includes directories and files structured as follows:

```
└── Project_2/
    ├── P1/
    │   └── process1.c
    ├── P2/
    │   └── process2.c
    ├── P3/
    │   └── process3.c
    ├── P4_coord/
    │   └── process4.c
    ├── Makefile
    └── Failure_coord.c
```

## Overview

`Failure_coord.c` is responsible to overlook failure of cohorts it ensures no two cohorts fail in a round

The server and client files need to be run on designated machines because the IP addresses and port numbers are hardcoded as follows:
- `process1` on `dc01`
- `process2` on `dc02`
- `process3` on `dc03`
- `process4` on `dc04`
- `Failure_coord` on `dc05`

## Compilation Instructions

To compile the project:

1. Navigate to the project's root directory:
   ```
   cd Project_2/
   ```

2. Run the following command to compile all `.c` files:
   ```
   make
   ```
3. To compile Failure_coord.c:
  ```
  gcc -Wall -g -std=c99 -o manager Failure_coord.c
  ```

This will compile all the `.c` files. 
