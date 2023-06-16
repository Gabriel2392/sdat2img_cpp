# sdat2img
Convert sparse Android data image (.dat) into filesystem ext4 image (.img)

This is a C++ equivalent of the original sdat2img tool, which was originally written in Python by xpirt, luxi78, and howellzhu.

**Note:** newer Google's [Brotli](https://github.com/google/brotli) format (`system.new.dat.br`) must be decompressed to a valid sparse data image before using `sdat2img` binary.

## Requirements
This project requires `clang` to build. Please ensure that `clang` is installed on your system before attempting to build the project.

## Build
```
make
```

## Usage
```
./sdat2img <transfer_list> <system_new_file> [system_img]
```
- `<transfer_list>` = input, system.transfer.list from rom zip
- `<system_new_file>` = input, system.new.dat from rom zip
- `[system_img]` = output ext4 raw image file



## Example
This is a simple example on a Linux system: 
```
~$ ./sdat2img vendor.transfer.list vendor.new.dat vendor.img
```

- OR

```
~$ ./sdat2img system.transfer.list system.new.dat
```