cmake_minimum_required (VERSION 3.2)
project(KatanaExampleOps)

set(KATANA_HOME "${CMAKE_CURRENT_LIST_DIR}/../../.." CACHE PATH "Path to Katana")
list(INSERT CMAKE_MODULE_PATH 0 "${KATANA_HOME}/plugins/Src/cmake")

find_package(Katana PATHS "${KATANA_HOME}/plugin_apis/cmake" REQUIRED)

if (NOT MSVC)
    add_compile_options(-Wall)
endif ()

# Ensure we don't compile DSOs with undefined symbols.
if (CMAKE_SYSTEM_NAME MATCHES Linux)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
endif ()

if (MSVC)
    # Prevent Boost auto-linking.
    add_definitions(-DBOOST_ALL_NO_LIB)
endif ()

# Find dependencies.
find_package(Boost 1.55 COMPONENTS regex system thread)
#find_package(HDF5 COMPONENTS C HL)
find_package(OpenEXR)
find_package(ZLIB)
find_package(Alembic) # Alembic comes last, as it requires all of the above.

#-------------------------------------------------------------------------------

if (ALEMBIC_FOUND AND Boost_FOUND AND OPENEXR_FOUND
        AND ZLIB_FOUND)
    add_subdirectory(AlembicIn)
else ()
    message("Not compiling AlembicIn Op as not all dependencies were found.")
endif ()

if (Boost_FOUND)
    add_subdirectory(AttributeFile_In)
else ()
    message("Not compiling AttributeFile_In Op as Boost was not found.")
endif ()

if (OPENEXR_FOUND)
    add_subdirectory(RigidBind)
    add_subdirectory(SkinEnvelope)
else ()
    message("Not compiling RigidBind or SkinEnvelope Ops as OpenEXR was not found.")
endif ()

add_subdirectory(AttributeCopy)
add_subdirectory(AttributeSet)
add_subdirectory(CubeMaker)
add_subdirectory(GeoScaler)
add_subdirectory(HelloWorld)
add_subdirectory(HierarchyCopy)
add_subdirectory(Messer)
add_subdirectory(Prune)
add_subdirectory(SphereMaker)
add_subdirectory(SphereMakerMaker)
add_subdirectory(StaticSceneCreate)
add_subdirectory(SubdividedSpace)
