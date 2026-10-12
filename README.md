# BSDSF24M017-OS-A02

# OS Assignment 02: Re-engineering the ls Utility

This repository contains my implementation of the Unix `ls` command in C for my Operating Systems course. 

## Features
* Standard directory listing
* Long listing format (`-l`)
* Column display (down then across)
* Horizontal column display (`-x`)
* Alphabetical sorting
* Colorized output based on file type
* Recursive directory listing (`-R`)

## How to Build
To compile the program, open your terminal in the root directory of the project and run:
make

# Basic listing
./bin/ls

# Long listing format
./bin/ls -l

# Horizontal display
./bin/ls -x

# Recursive listing
./bin/ls -R

# Specify a directory
./bin/ls /etc


