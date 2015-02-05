set(MAYAPLUGIN alembicHolder)

file(GLOB SRC "*.cpp" "*h" "cmds/*.cpp" "cmds/*.h")


add_definitions(${MAYA_DEFINITIONS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${ALEMBIC_INCLUDE_DIR})
include_directories(${ILMBASE_INCLUDE_DIRECTORY})
include_directories(${MAYA_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})

add_library(${MAYAPLUGIN} SHARED ${SRC})
target_link_libraries(${MAYAPLUGIN} ${ALEMBIC_LIBRARIES} ${ALEMBIC_HDF5_LIBS} ${ILMBASE_LIBS} ${ZLIB_LIBRARIES} ${SZIP_LIBRARIES} ${MAYA_LIBRARIES} ${JSONCPP_LIBRARIES} ${Boost_REGEX_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} Foundation OpenMaya OpenMayaRender OpenMayaUI OpenMayaAnim OpenMayaFX)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(${MAYAPLUGIN} OpenGl32)
endif()

set_target_properties(${MAYAPLUGIN} PROPERTIES PREFIX "")
set_target_properties(${MAYAPLUGIN} PROPERTIES SUFFIX "${MAYA_EXTENSION}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${MAYAPLUGIN} RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR})
else()
	install(TARGETS ${MAYAPLUGIN} DESTINATION ${PLUGINS_INSTALL_DIR})
endif()
