set(MAYAPLUGIN abcMayaShader)

file(GLOB SRC "*.cpp" "*h")
add_definitions(${MAYA_DEFINITIONS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${ALEMBIC_INCLUDE_DIR})
include_directories(${ILMBASE_INCLUDE_DIRECTORY})
include_directories(${MAYA_INCLUDE_DIRS})
include_directories(${MTOA_INCLUDE_DIRS})
include_directories(${PYSTRING_INCLUDE_DIR})

add_library(${MAYAPLUGIN} SHARED ${SRC})
target_link_libraries(${MAYAPLUGIN} ${ALEMBIC_LIBRARIES} ${ALEMBIC_HDF5_LIBS} ${ILMBASE_LIBS} ${ZLIB_LIBRARIES} ${SZIP_LIBRARIES} ${PYSTRING_LIBRARIES} ${MTOA_LIBS} ${Boost_REGEX_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} Foundation OpenMaya OpenMayaRender OpenMayaUI OpenMayaAnim OpenMayaFX ai)

set_target_properties(${MAYAPLUGIN} PROPERTIES PREFIX "")
set_target_properties(${MAYAPLUGIN} PROPERTIES SUFFIX "${MAYA_EXTENSION}")

install(TARGETS ${MAYAPLUGIN} DESTINATION ${PLUGINS_INSTALL_DIR})