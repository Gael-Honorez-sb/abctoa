set(SHADER arnoldAlembicShader)

set(SRC abcshader.cpp loader.cpp ../../../common/abcshaderutils.h ../../../common/abcshaderutils.cpp)

include_directories(${CMAKE_SOURCE_DIR}/alembic/lib)
include_directories(${CMAKE_BINARY_DIR}/alembic/lib)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Half)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Iex)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/IexMath)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/IlmThread)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Imath)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/thirdParty/openEXR/IlmBase/config)

include_directories(${CMAKE_SOURCE_DIR}/common)

include_directories(${ARNOLD_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/pystring)
link_directories(${ARNOLD_LIBRARY_DIR})

add_library(${SHADER} SHARED ${SRC})

target_link_libraries(${SHADER} ai Alembic pystring_lib_static Iex Half)
set_target_properties(${SHADER} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${SHADER} RUNTIME DESTINATION ${SHADERS_INSTALL_DIR})
else()
	install(TARGETS ${SHADER} DESTINATION ${SHADERS_INSTALL_DIR})
endif()