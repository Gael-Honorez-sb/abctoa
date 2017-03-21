set(MAYAPLUGIN abcMayaShader)

file(GLOB SRC "*.cpp" "*h")

add_library(${MAYAPLUGIN} SHARED ${SRC})

MAYA_SET_PLUGIN_PROPERTIES(${MAYAPLUGIN})

include_directories(${CMAKE_BINARY_DIR}/alembic/lib)
include_directories(${CMAKE_SOURCE_DIR}/alembic/lib)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Half)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Iex)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/IexMath)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/IlmThread)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Imath)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/thirdParty/openEXR/IlmBase/config)

include_directories(${MTOA_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/pystring)
include_directories(${ARNOLD_INCLUDE_DIR})

target_link_libraries(${MAYAPLUGIN} pystring_lib_static ${MTOA_LIBRARY} ${MAYA_LIBRARIES} ${ARNOLD_LIBRARY})




if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${MAYAPLUGIN} RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR})
else()
	install(TARGETS ${MAYAPLUGIN} DESTINATION ${PLUGINS_INSTALL_DIR})
endif()
