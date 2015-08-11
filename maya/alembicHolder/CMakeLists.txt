set(MAYAPLUGIN alembicHolder)

file(GLOB SRC "*.cpp" "*h" "cmds/*.cpp" "cmds/*.h")


add_library(${MAYAPLUGIN} SHARED ${SRC})

include_directories(${Boost_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${PYSTRING_INCLUDE_DIR})


target_link_libraries(${MAYAPLUGIN} ${PYSTRING_LIBRARIES} ${MAYA_LIBRARIES} ${JSONCPP_LIBRARIES} ${Boost_REGEX_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY})
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(${MAYAPLUGIN} OpenGl32)
endif()

MAYA_SET_PLUGIN_PROPERTIES(${MAYAPLUGIN})
ALEMBIC_SET_PROPERTIES(${MAYAPLUGIN})


if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${MAYAPLUGIN} RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR})
else()
	install(TARGETS ${MAYAPLUGIN} DESTINATION ${PLUGINS_INSTALL_DIR})
endif()
