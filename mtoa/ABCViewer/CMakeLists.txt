set(MTOATRANSLATOR abcViewer)

file(GLOB SRC "*.cpp" "*h")
add_definitions(${MAYA_DEFINITIONS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${MAYA_INCLUDE_DIRS})
include_directories(${MTOA_INCLUDE_DIRS})

add_library(${MTOATRANSLATOR} SHARED ${SRC})
target_link_libraries(${MTOATRANSLATOR} ${MTOA_LIBS} Foundation OpenMaya OpenMayaRender OpenMayaUI OpenMayaAnim OpenMayaFX ai)
set_target_properties(${MTOATRANSLATOR} PROPERTIES PREFIX "")

install(TARGETS ${MTOATRANSLATOR} RUNTIME DESTINATION ${TRANSLATOR_INSTALL_DIR})