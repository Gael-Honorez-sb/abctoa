set(UTIL assShadersToAbc)

file(GLOB SRC "*.cpp" "*h" "../../../common/*.h" "../../../common/*.cpp")

include_directories(${Boost_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${PYSTRING_INCLUDE_DIR})
include_directories(${ARNOLD_INCLUDE_DIR})

add_executable(${UTIL} ${SRC})
target_link_libraries(${UTIL} ${ARNOLD_LIBRARY}  ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${PYSTRING_LIBRARIES} ${JSONCPP_LIBRARIES})
set_target_properties(${UTIL} PROPERTIES PREFIX "")

ALEMBIC_SET_PROPERTIES(${UTIL})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${UTIL} RUNTIME DESTINATION ${DSO_INSTALL_DIR})
else()
	

install(TARGETS ${UTIL} DESTINATION ${DSO_INSTALL_DIR})
endif()

