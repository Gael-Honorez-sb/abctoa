set(PROC arnoldAlembicMaterialOperator)

file(GLOB SRC "*.cpp" "*h" "../../../common/abcshaderutils.h" "../../../common/abcshaderutils.cpp" "../../../common/PathUtil.h" "../../../common/PathUtil.cpp")

include_directories(${CMAKE_SOURCE_DIR}/thirdParty/jsoncpp/include)
include_directories(${CMAKE_SOURCE_DIR}/thirdParty/pystring)
include_directories(${CMAKE_SOURCE_DIR}/alembic/lib)
include_directories(${CMAKE_BINARY_DIR}/alembic/lib)
include_directories(${ARNOLD_INCLUDE_DIR})

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Half)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Iex)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/IexMath)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/IlmThread)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Imath)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/thirdParty/openEXR/IlmBase/config)

include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${ALEMBIC_ILMBASE_INCLUDE_DIRECTORY})

link_directories(${ARNOLD_LIBRARY_DIR})

add_library(${PROC} SHARED ${SRC})
target_link_libraries(${PROC} ai Alembic jsoncpp_lib_static pystring_lib_static Iex Half)
set_target_properties(${PROC} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${PROC} RUNTIME DESTINATION ${DSO_INSTALL_DIR})
else()
	

install(TARGETS ${PROC} DESTINATION ${DSO_INSTALL_DIR})
endif()

