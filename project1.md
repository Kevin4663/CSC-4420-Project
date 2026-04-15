# [cite_start]Wayne State University [cite: 1]
## [cite_start]CSC 4420 - Winter 2026 [cite: 1]
[cite_start]**Computer Operating Systems: Lab Project** [cite: 2]

---

## [cite_start]Objective [cite: 3]
[cite_start]Your task in this assignment is to implement a simulated file system. [cite: 4] [cite_start]The simulated file system (**simfs**) is stored in a single Unix file. [cite: 5] [cite_start]There is only one directory in this file system, and there are limits on the number of files (**MAXFILES**) it can store and the amount of space (**MAXBLOCKS**) it can occupy. [cite: 6]

[cite_start]The key file system structures are already defined; your job is to add functions: the ability to create a file, delete a file, write to a file, and read from a file. [cite: 7] [cite_start]In completing this assignment, we expect you will learn about how file systems are implemented, will become familiar with both binary and ASCII file I/O, and will become comfortable using arrays of structs and characters. [cite: 8]

---

## [cite_start]What to Submit [cite: 9]
* [cite_start]Submit all your source code, header files, and Makefile to your repository under the `starter_code` directory. [cite: 10]
* [cite_start]Please do not submit `.o` files or executable files. [cite: 11]
* [cite_start]Do not modify the `Makefile` or `simfstypes.h`. [cite: 11]

---

## [cite_start]Academic Integrity and Collaboration Policy [cite: 12]
[cite_start]This assignment is designed to evaluate your understanding and ability to implement system concepts. [cite: 13] [cite_start]You are allowed to use external resources, including online materials, documentation, and AI tools (e.g., for "vibe coding" or assistance). [cite: 14] [cite_start]However, you are fully responsible for the code you submit and must be able to explain and justify all aspects of your implementation. [cite: 15]

[cite_start]Submissions will be evaluated based on: [cite: 16]
* [cite_start]**Correctness**, through test cases [cite: 18]
* [cite_start]**Code quality** and adherence to specifications [cite: 19]
* [cite_start]**Your understanding**, which may be assessed through discussion or presentation [cite: 20]

[cite_start]The use of external code (including AI-generated code) is permitted, but: [cite: 21]
* [cite_start]You must understand what your code does [cite: 22]
* [cite_start]You may be asked to walk through or modify your implementation [cite: 23]
* [cite_start]Inability to explain your solution may result in loss of credit [cite: 23]

[cite_start]This Project will make up **30% of your final grade**. [cite: 24] [cite_start]**No late submissions** will be accepted after the due date, as final grades must be reported on time. [cite: 25]

[cite_start]Your code must adhere strictly to the specifications below; failure to do so may result in point deductions. [cite: 26] [cite_start]Please read the next section on the **simfs structure** carefully, as your implementation must conform precisely to the defined format. [cite: 27]

---

## [cite_start]Background: simfs structure [cite: 28]
[cite_start]**simfs** should be thought of as an array of blocks. [cite: 29] [cite_start]Each block is a contiguous chunk of **BLOCKSIZE** bytes. [cite: 20] [cite_start]Metadata is stored at the beginning of the file in the first (or first few) blocks. [cite: 30]

[cite_start]The simulated file system contains two types of metadata: **file entries (fentries)** and **file nodes (fnodes)**. [cite: 31]

### [cite_start]A **fentry** contains: [cite: 32]
* [cite_start]**file name**: An array to store a name of maximum length 11. [cite: 33]
* [cite_start]**size**: An unsigned short integer giving the size of the actual file. [cite: 34] [cite_start]Note that a file might not use all of the space in the blocks allocated to it. [cite: 35]
* [cite_start]**firstblock**: An index into the array of fnodes. [cite: 37] [cite_start]The specified fnode knows where the first block of data in the file is stored and how to get information about the next (second) block of data. [cite: 37]

### [cite_start]An **fnode** contains: [cite: 38]
* [cite_start]**blockindex**: The index of the data block storing the file data associated with this fnode. [cite: 40] [cite_start]The magnitude of the index is always the index of the fnode in its array; it is negative if the data block is not in use. [cite: 41, 42]
* [cite_start]**nextblock**: An index into the array of fnodes. [cite: 43] [cite_start]The specified fnode contains info about the next block of data in the file. [cite: 43] [cite_start]This value is -1 if there is no next block in the file. [cite: 44]

### [cite_start]Layout and Storage [cite: 45]
[cite_start]There are a fixed number of fentries and a fixed number of fnodes. [cite: 45] [cite_start]The array of fentries is stored at the very beginning of the file, followed immediately after by the array of fnodes. [cite: 46] [cite_start]The number of blocks required to store the fentries and fnodes depends on **MAXFILES** and **MAXBLOCKS**. [cite: 47]

**Example:**
[cite_start]Suppose the maximum number of files is 4 and the maximum number of fnodes is 6. [cite: 49] [cite_start]The block size is set to 128. [cite: 49] The fentry and fnode arrays take 88 bytes:
[cite_start]$$(16 \times 4) + (4 \times 6) = 88$$ [cite: 49]
[cite_start]Since these fit in a single block, the first fnode is in use (block 0 contains metadata), and file data begins in the second block (index 1). [cite: 50]

> [cite_start]**Note**: It is a good idea to set the maximum number of files and blocks so that they fit in exactly N blocks to avoid wasting space. [cite: 81] [cite_start]However, you may not assume this is the case; there may be wasted bytes between metadata and the start of file data. [cite: 82, 83] [cite_start]Additionally, data blocks for a particular file will not necessarily be contiguous. [cite: 84, 85]

---

## [cite_start]Starter Code [cite: 86]
[cite_start]You may not modify the structs defined in `simfstypes.h` or the `Makefile`, but you may modify any other code. [cite: 87, 88]

* [cite_start]**Makefile**: Used for building the program. [cite: 91] [cite_start]Do not modify this file. [cite: 93]
* [cite_start]**simfstypes.h**: Contains type definitions and file system parameters like **MAXFILES**. [cite: 94, 95] [cite_start]Do not commit changes to this file; it will be replaced during testing. [cite: 98, 99]
* [cite_start]**simfs.h**: Contains shared function prototypes. [cite: 100] [cite_start]You may add more prototypes here. [cite: 101]
* [cite_start]**simfs.c**: The main program that parses arguments and calls command functions. [cite: 102, 103] [cite_start]You will need to modify the switch statement for new commands. [cite: 104]
* [cite_start]**initfs.c**: Contains `initfs`, which initializes the file system structure. [cite: 105, 106] [cite_start]**Warning**: It will overwrite existing files. [cite: 107]
* [cite_start]**printfs.c**: A convenience function to print metadata in a human-readable format. [cite: 111, 112]
* [cite_start]**simfs_ops.c**: Where you will do the majority of your work, including implementing the four required operations. [cite: 114, 116]

---

## [cite_start]Your Task [cite: 117]
[cite_start]You will implement the following operations, which must work for any positive, valid values of **BLOCKSIZE**, **MAXFILES**, or **MAXBLOCKS**. [cite: 118]

### [cite_start]1. Creating a file (`createfile`) [cite: 120]
* [cite_start]**Argument**: A simulated file name. [cite: 121]
* **Action**: Create an empty file using the first available fentry. [cite: 122]
* [cite_start]**Error**: Emit an error if there are not enough resources. [cite: 123]

### [cite_start]2. Deleting a file (`deletefile`) [cite: 124]
* **Argument**: A simulated file name. [cite: 125]
* [cite_start]**Action**: Remove the file and free any blocks used. [cite: 126]
* [cite_start]**Security**: Overwrite the file data with zeroes to avoid malicious use of old data. [cite: 127]

### [cite_start]3. Reading a file (`readfile`) [cite: 128]
* [cite_start]**Arguments**: file name, start (offset), length. [cite: 129, 130, 131]
* **Action**: Print the requested data to **stdout**. [cite: 132]
* [cite_start]**Error**: Emit an error to **stderr** if the request cannot be completed (e.g., start position > file size). [cite: 133, 134]

### [cite_start]4. Writing a file (`writefile`) [cite: 136]
* **Arguments**: file name, start (offset), length. [cite: 137, 138, 139]
* [cite_start]**Action**: Write data from **stdin** to the file. [cite: 140]
* [cite_start]**Error**: Emit an error if the start position is larger than the current size or if there are not enough free blocks. [cite: 141, 142, 143]
* **Atomicity**: In any error case, no changes must be made to the filesystem; it cannot be left in an inconsistent state. [cite: 144, 145]
* [cite_start]**Initialization**: Whenever a new data block is allocated, initialize it to binary zeros. [cite: 146]

---

## [cite_start]Error Handling [cite: 148]
* Your program should never crash (e.g., segmentation fault). [cite: 149]
* [cite_start]Check the return values of all library calls and verify resource availability before modifying the file system. [cite: 150]
* [cite_start]Use **valgrind** to check memory usage. [cite: 151]
* Error messages must be printed to **stderr**, include the name of the operation being attempted, describe the issue, and the program must exit with a non-zero value. [cite: 152, 153]
* [cite_start]No messages should be printed to **stderr** that are not errors. [cite: 154]