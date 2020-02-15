#!/bin/bash
g++ -Werror -fPIC -shared -o libWrapperEngine.so wrapper.c uni_log.c -lengine_c -L.