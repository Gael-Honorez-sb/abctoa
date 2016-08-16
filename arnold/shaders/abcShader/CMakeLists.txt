set(SHADER arnoldAlembicShader)

set(SRC abcshader.cpp loader.cpp ../../../common/abcshaderutils.h ../../../common/abcshaderutils.cpp)

add_library(${SHADER} SHARED ${SRC})

target_compile_definitions(${SHADER} PUBLIC REQUIRE_IOSTREAM _BOOL)  

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
    target_compile_definitions(${SHADER} PUBLIC LINUX LINUX_64) 
endif()

include_directories(${Boost_INCLUDE_DIR})
include_directories(${ARNOLD_INCLUDE_DIR})
include_directories(${CORE_INCLUDE_DIRS})
include_directories(${PYSTRING_INCLUDE_DIR})

target_link_libraries(${SHADER} ${ARNOLD_LIBS} ${PYSTRING_LIBS} ${CORE_LIBS})
set_target_properties(${SHADER} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${SHADER} RUNTIME DESTINATION ${SHADERS_INSTALL_DIR})
else()
	install(TARGETS ${SHADER} DESTINATION ${SHADERS_INSTALL_DIR})
endif()
