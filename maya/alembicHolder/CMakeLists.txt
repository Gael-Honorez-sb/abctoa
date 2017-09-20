set(MAYAPLUGIN alembicHolder)

file(GLOB SRC "*.cpp" "*h" "cmds/*.cpp" "cmds/*.h" "../../common/PathUtil.cpp" "../../common/PathUtil.h")


add_library(${MAYAPLUGIN} SHARED ${SRC})

include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/jsoncpp/include)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/pystring)
include_directories(${CMAKE_SOURCE_DIR}/alembic/lib)
include_directories(${CMAKE_BINARY_DIR}/alembic/lib)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Half)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Iex)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/IexMath)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/IlmThread)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdParty/openEXR/IlmBase/Imath)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/thirdParty/openEXR/IlmBase/config)


target_link_libraries(${MAYAPLUGIN} ${MAYA_LIBRARIES} Alembic jsoncpp_lib_static pystring_lib_static Iex Half)


MAYA_SET_PLUGIN_PROPERTIES(${MAYAPLUGIN})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(${MAYAPLUGIN} OpenGl32 wbemuuid)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(new_link_flags "-framework OpenGL -framework AGL")
	get_target_property(existing_link_flags ${MAYAPLUGIN} LINK_FLAGS)
	if(existing_link_flags)
		set(new_link_flags "${existing_link_flags} ${new_link_flags}")
	endif()
	set_target_properties(${MAYAPLUGIN} PROPERTIES LINK_FLAGS ${new_link_flags})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${MAYAPLUGIN} RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR})
else()
	install(TARGETS ${MAYAPLUGIN} DESTINATION ${PLUGINS_INSTALL_DIR})
endif()
