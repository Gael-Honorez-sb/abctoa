set(UTIL assShadersToAbc)

file(GLOB SRC "*.cpp" "*h" "../../../common/*.h" "../../../common/*.cpp")

include_directories(${Boost_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${PYSTRING_INCLUDE_DIR})
include_directories(${ARNOLD_INCLUDE_DIR})
include_directories(${CORE_INCLUDE_DIRS})

add_executable(${UTIL} ${SRC})
target_link_libraries(${UTIL} ${CORE_LIBS} ${ARNOLD_LIBS} ${JSONCPP_LIBS} ${PYSTRING_LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})
set_target_properties(${UTIL} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${UTIL} RUNTIME DESTINATION ${DSO_INSTALL_DIR})
else()
	

install(TARGETS ${UTIL} DESTINATION ${DSO_INSTALL_DIR})
endif()

