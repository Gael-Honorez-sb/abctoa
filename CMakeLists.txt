cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share)

project(abcToA)

set(ABCTOA_MAJOR_VERSION 0)
set(ABCTOA_MINOR_VERSION 9)
set(ABCTOA_PATCH_VERSION 0)
set(ABCTOA_VERSION "${ABCTOA_MAJOR_VERSION}.${ABCTOA_MINOR_VERSION}.${ABCTOA_PATCH_VERSION}")

set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_VERBOSE_MAKEFILE FALSE)
set(CMAKE_SKIP_RPATH TRUE)

# Fix to stop crash on OSX < 10.9
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)

# Compiler flags
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Disable some of the bullshit warnings MSVC wants to barf
    add_definitions( "-W3 -MP -D_CRT_SECURE_NO_WARNINGS -wd4005 -wd4996 -wd4305 -wd4244 -nologo" )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

IF( "${CMAKE_SYSTEM_NAME}" MATCHES "Linux" )
	add_definitions(-D_LINUX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fvisibility=hidden -fPIC")
endif()


# check if we have a local cmake include file and include it if we do
# this is useful for setting our arnold location as an alternative to
# environment variables
if(EXISTS ${CMAKE_SOURCE_DIR}/local.cmake)
    message(STATUS "Using local.cmake")
    include(${CMAKE_SOURCE_DIR}/local.cmake)
endif()

# Find Arnold SDK
find_package(Arnold REQUIRED)
include_directories(${ARNOLD_INCLUDE_DIRS})
link_directories(${ARNOLD_LIBRARY_DIR})

# Find maya
find_package(Maya REQUIRED)

# Find Boost
SET(Boost_USE_STATIC_LIBS 1)
find_package(Boost REQUIRED COMPONENTS regex thread date_time system chrono atomic)

# Find ILMBASE
find_package(IlmBase REQUIRED)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if(NOT ILMBASE_USE_STATIC_LIBS)
		add_definitions( "-DOPENEXR_DLL -DHALF_EXPORTS")
	endif()
endif()


# Find alembic
find_package(Alembic REQUIRED)

# Find HDF5
find_package(AlembicHDF5 REQUIRED)

find_package(SZIP)

# Find JSONCPP
find_package(JsonCPP REQUIRED)

# Find Pystring
find_package(PyString REQUIRED)

find_package(AlembicZlib REQUIRED)

configure_file(
    "${PROJECT_SOURCE_DIR}/exportArnoldDefinitions.in.py"
    "${PROJECT_SOURCE_DIR}/exportArnoldDefinitions.py"
)

if (NOT DEFINED INSTALL_DIR)
    if (DEFINED INSTALL_ROOT)
        set(INSTALL_DIR "${INSTALL_ROOT}/abcToA-${ABCTOA_VERSION}")
        message("INSTALL_ROOT defined. Adding versions automatically:\n\t${INSTALL_DIR}")
    else()
		set(INSTALL_ROOT "${CMAKE_BINARY_DIR}/dist")
        set(INSTALL_DIR "${INSTALL_ROOT}/abcToA-${ABCTOA_VERSION}")
		
        message("INSTALL_DIR not defined. Defaulting to:\n\t${INSTALL_DIR}")
    endif()
else()
    message("Installing to:\n\t${INSTALL_DIR}")
endif()

# Set up installation paths
set(DSO_INSTALL_DIR ${INSTALL_DIR}/procedurals)
set(TRANSLATOR_INSTALL_DIR ${INSTALL_DIR}/extensions)
set(SHADERS_INSTALL_DIR ${INSTALL_DIR}/shaders)
set(PLUGINS_INSTALL_DIR ${INSTALL_DIR}/plug-ins)
set(PYSIDE_INSTALL_DIR ${INSTALL_DIR}/pyside)

message("DSO binaries will be installed to:\n\t${DSO_INSTALL_DIR}")
message("Shaders will be installed to: \n\t${SHADERS_INSTALL_DIR}")
message("Maya Plug-ins will be installed to: \n\t${PLUGINS_INSTALL_DIR}")
message("MtoA Translators will be installed to: \n\t${TRANSLATOR_INSTALL_DIR}")
message("Icons will be installed to: \n\t${INSTALL_DIR}/icons")
message("Scripts will be installed to: \n\t${INSTALL_DIR}/scripts")
message("PySide will be installed to: \n\t${PYSIDE_INSTALL_DIR}")
message("Samples will be installed to: \n\t${INSTALL_DIR}/samples")

# Set the list of subdirectories to recurse into to find stuff to build
set(SUBDIRECTORIES 
        arnold/procedurals/alembicProcedural
		arnold/shaders/abcShader
		maya/alembicHolder
		maya/abcMayaShader
		mtoa/ABCViewer
		mtoa/abcShader
)

include_directories(common)

# loop over subdirectories
foreach(SUBDIR ${SUBDIRECTORIES})
    add_subdirectory(${SUBDIR})
endforeach()

# add top-level files
INSTALL(FILES README INSTALL DESTINATION ${INSTALL_DIR})

# add scripts & icons
INSTALL(DIRECTORY icons DESTINATION ${INSTALL_DIR})
INSTALL(DIRECTORY maya/scripts DESTINATION ${INSTALL_DIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if(${MAYA_VERSION} EQUAL "2013")
		MESSAGE(STATUS "Pyside Windows for Maya 2013")
		INSTALL(DIRECTORY contrib/pyside113_py26_qt471maya2013_win64/lib DESTINATION ${PYSIDE_INSTALL_DIR})
		INSTALL(DIRECTORY contrib/pyside113_py26_qt471maya2013_win64/python DESTINATION ${PYSIDE_INSTALL_DIR})
	endif()
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		MESSAGE(STATUS "Pyside Linux for Maya")
		INSTALL(DIRECTORY contrib/pyside112_py26_qt471_linux/lib DESTINATION ${PYSIDE_INSTALL_DIR})
		INSTALL(DIRECTORY contrib/pyside112_py26_qt471_linux/python DESTINATION ${PYSIDE_INSTALL_DIR})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		MESSAGE(STATUS "Pyside Linux for MacOS")
		INSTALL(DIRECTORY contrib/pyside112_py26_qt471_mac/lib DESTINATION ${PYSIDE_INSTALL_DIR})
		INSTALL(DIRECTORY contrib/pyside112_py26_qt471_mac/python DESTINATION ${PYSIDE_INSTALL_DIR})

endif()

INSTALL(DIRECTORY maya/samples DESTINATION ${INSTALL_DIR})


# configure other files with version numbers etc.

configure_file (
  "${CMAKE_SOURCE_DIR}/maya/alembicHolder/version.h.in"
  "${CMAKE_SOURCE_DIR}/maya/alembicHolder/version.h"
  )

if(${MAYA_VERSION} EQUAL "2013")
  configure_file (
  "${CMAKE_SOURCE_DIR}/abcToA.mod.2013.in"
  "${PROJECT_SOURCE_DIR}/abcToA.mod"
  )
else()
  configure_file (
  "${CMAKE_SOURCE_DIR}/abcToA.mod.in"
  "${PROJECT_SOURCE_DIR}/abcToA.mod"
  )
endif()

configure_file(
    "${PROJECT_SOURCE_DIR}/INSTALL.in"
    "${PROJECT_SOURCE_DIR}/INSTALL"
)
  
# 
configure_file(
    "${PROJECT_SOURCE_DIR}/package.in.py"
    "${PROJECT_SOURCE_DIR}/package.py"
)

