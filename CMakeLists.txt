########################################################################
# USRP based radar streaming relay and client
# Copyright (c) 2012 Neratec Solutions AG
########################################################################

cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

PROJECT(usrp_radar_relay)

########################################################################
# Find the library for the USRP Hardware Driver
########################################################################

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_UHD uhd)

FIND_PATH(
	UHD_INCLUDE_DIRS
	NAMES uhd/config.hpp
	HINTS $ENV{UHD_DIR}/include
		${PC_UHD_INCLUDEDIR}
	PATHS /usr/local/include /usr/include
)

FIND_LIBRARY(
	UHD_LIBRARIES
	NAMES uhd
	HINTS $ENV{UHD_DIR}/lib
		${PC_UHD_LIBDIR}
	PATHS /usr/local/lib /usr/lib
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UHD DEFAULT_MSG UHD_LIBRARIES UHD_INCLUDE_DIRS)
MARK_AS_ADVANCED(UHD_LIBRARIES UHD_INCLUDE_DIRS)

if(NOT UHD_FOUND)
	message(FATAL_ERROR "UHD libraries required, but not found!\n"
		"Please install them according to "
		"http://files.ettus.com/uhd_docs/manual/html/build.html"
	)
endif(NOT UHD_FOUND)

########################################################################
# Using json-c static libary as external project
########################################################################

# prevent external projects to be cleaned
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM TRUE)

ExternalProject_Add(libjson
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_projects/json-c-0.9
	CONFIGURE_COMMAND <SOURCE_DIR>/configure
		--prefix=${CMAKE_BINARY_DIR} --enable-static --disable-shared
)

# add imported target
add_library(imp_json STATIC IMPORTED)
# point the imported target at the real file
set_property(TARGET imp_json PROPERTY IMPORTED_LOCATION ./lib/libjson.a)

########################################################################
# radar_server and radar_client
########################################################################
set(RSERVER radar_server)
set(RCLIENT radar_client)

ADD_DEFINITIONS(-O2 -Wall -g3 -Iinclude -Llib)
ADD_DEFINITIONS(-I${UHD_INCLUDE_DIRS})

set(common_SRCS
	Debug.cpp
	RadarJsonRpc.cpp
)
ADD_LIBRARY(usrp_common STATIC ${common_SRCS})

ADD_EXECUTABLE(${RSERVER}
	RadarServer.cpp
	RadarRpcServer.cpp
	UsrpRadarRelay.cpp
	UsrpRadarRelay_OnDemand.cpp
	UsrpRadarRelay_Continuous.cpp
)
TARGET_LINK_LIBRARIES(${RSERVER} usrp_common ${UHD_LIBRARIES} imp_json)

ADD_EXECUTABLE(${RCLIENT}
	RadarClient.cpp
	RadarRpcClient.cpp
)
TARGET_LINK_LIBRARIES(${RCLIENT} usrp_common imp_json)
