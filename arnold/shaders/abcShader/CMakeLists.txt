set(SHADER arnoldAlembicShader)

set(SRC abcshader.cpp loader.cpp ../../../common/abcshaderutils.h ../../../common/abcshaderutils.cpp)


include_directories(${ARNOLD_INCLUDE_DIR})
include_directories(${PYSTRING_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/pystring)

add_library(${SHADER} SHARED ${SRC})

target_link_libraries(${SHADER} ${ARNOLD_LIBRARY} pystring_lib_static)
set_target_properties(${SHADER} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${SHADER} RUNTIME DESTINATION ${SHADERS_INSTALL_DIR})
else()
	install(TARGETS ${SHADER} DESTINATION ${SHADERS_INSTALL_DIR})
endif()
