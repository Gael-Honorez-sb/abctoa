set(PROC arnoldAlembicProcedural)

file(GLOB SRC "*.cpp" "*h" "../../../common/abcshaderutils.h" "../../../common/abcshaderutils.cpp")

add_library(${PROC} SHARED ${SRC})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
    target_compile_definitions(${PROC} PUBLIC LINUX LINUX_64) 
endif()

include_directories(${Boost_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${PYSTRING_INCLUDE_DIR})
include_directories(${ARNOLD_INCLUDE_DIR})
include_directories(${CORE_INCLUDE_DIRS})

target_link_libraries(${PROC} ${ARNOLD_LIBS} ${JSONCPP_LIBS} ${PYSTRING_LIBS} ${Boost_REGEX_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${CORE_LIBS})
set_target_properties(${PROC} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${PROC} RUNTIME DESTINATION ${DSO_INSTALL_DIR})
else()
	
install(TARGETS ${PROC} DESTINATION ${DSO_INSTALL_DIR})
endif()

