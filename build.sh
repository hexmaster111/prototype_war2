#!/bin/bash
set -xe
cc minigame_war2.c  -DCPLIB_IMPL -ggdb -lraylib -lm -lpthread -ldl -ominigame_war2 -Wall -Wno-unused-function #-fsanitize=address
