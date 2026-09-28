###################################################
# Location of the project directory and Makefiles
#
P := ../..
M := ../../../../..
BUILD := ../../build

###################################################
# Project definition
#
PROJECT_NAME := TutorialMyMakefile
PROJECT_VERSION := 0.1.0
TARGET_NAME := foo
TARGET_DESCRIPTION := library foo
COMPILATION_MODE := release

include $(M)/project/Makefile

###################################################
# Compile the library
#
INCLUDES := $(P)/include
LIB_FILES := foo.cpp

include $(M)/rules/Makefile
