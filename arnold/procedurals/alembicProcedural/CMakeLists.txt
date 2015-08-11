set(PROC arnoldAlembicProcedural)

file(GLOB SRC "*.cpp" "*h" "../../../common/*.h" "../../../common/*.cpp")

include_directories(${Boost_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${PYSTRING_INCLUDE_DIR})


add_library(${PROC} SHARED ${SRC})
target_link_libraries(${PROC} ai ${JSONCPP_LIBRARIES} ${PYSTRING_LIBRARIES} ${Boost_REGEX_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY})
set_target_properties(${PROC} PROPERTIES PREFIX "")

ALEMBIC_SET_PROPERTIES(${PROC})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${PROC} RUNTIME DESTINATION ${DSO_INSTALL_DIR})
else()
	

install(TARGETS ${PROC} DESTINATION ${DSO_INSTALL_DIR})
endif()

