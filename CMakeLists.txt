cmake_minimum_required(VERSION 3.13)
project(gios_spr_19_pr1 C)

set(CMAKE_C_STANDARD 11)

include_directories(gflib)
include_directories(mtgf)

add_executable(gios_spr_19_pr1
        echo/echoclient.c
        echo/echoserver.c
        gflib/content.c
        gflib/content.h
        gflib/gf-student.h
        gflib/gfclient-student.h
        gflib/gfclient.c
        gflib/gfclient.h
        gflib/gfclient_download.c
        gflib/gfserver-student.h
        gflib/gfserver.c
        gflib/gfserver.h
        gflib/gfserver_main.c
        gflib/workload.c
        gflib/workload.h
        mtgf/content.c
        mtgf/content.h
        mtgf/gf-student.h
        mtgf/gfclient-student.h
        mtgf/gfclient.h
        mtgf/gfclient_download.c
        mtgf/gfserver-student.h
        mtgf/gfserver.h
        mtgf/gfserver_main.c
        mtgf/handler.c
        mtgf/log.h
        mtgf/steque.c
        mtgf/steque.h
        mtgf/workload.c
        mtgf/workload.h
        transfer/transferclient.c
        transfer/transferserver.c gflib/perform.c gflib/face.c)
