set(UTIL assShadersToAbc)

file(GLOB SRC "*.cpp" "*h" "../../../common/abc*.h" "../../../common/abc*.cpp")


include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/jsoncpp/include)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/pystring)

include_directories(${ARNOLD_INCLUDE_DIR})

add_executable(${UTIL} ${SRC})
target_link_libraries(${UTIL} ${ARNOLD_LIBRARY} jsoncpp_lib_static pystring_lib_static)
set_target_properties(${UTIL} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${UTIL} RUNTIME DESTINATION ${DSO_INSTALL_DIR})
else()
	

install(TARGETS ${UTIL} DESTINATION ${DSO_INSTALL_DIR})
endif()

