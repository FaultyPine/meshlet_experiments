@echo off

tup init
tup variant configs/desktop.config
tup variant configs/web.config
tup