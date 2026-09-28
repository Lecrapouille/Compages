###################################################
# Location of the project directory and Makefiles
#
P := .
M := ../../..
BUILD := ../../build

###################################################
# Project definition
#
PROJECT_NAME := TutorialMyMakefile
PROJECT_VERSION := 0.1.0
TARGET_NAME := HelloWorld01
TARGET_DESCRIPTION := Hello Word
COMPILATION_MODE := release

include $(M)/project/Makefile

###################################################
# Compile the project
#
INCLUDES := src
SRC_FILES := $(wildcard src/*.cpp)
DEFINES += -DFOO -UBAR

include $(M)/rules/Makefile