#!/bin/bash

files="
include/uapi/linux/sched.h
include/linux/sched.h
kernel/sched/sched.h
kernel/sched/core.c
kernel/sched/ts.c
kernel/sched/rt.c
kernel/sched/Makefile
"

tar -cvf submit_this_file.tar $files
