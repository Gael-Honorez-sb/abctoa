set(MAYAPLUGIN alembicHolder)

file(GLOB SRC "*.cpp" "*h" "cmds/*.cpp" "cmds/*.h")

add_library(${MAYAPLUGIN} SHARED ${SRC})

target_compile_definitions(${MAYAPLUGIN} PUBLIC REQUIRE_IOSTREAM _BOOL)  

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
    target_compile_definitions(${MAYAPLUGIN} PUBLIC LINUX LINUX_64) 
endif()

include_directories(${Boost_INCLUDE_DIR})
include_directories(${MAYA_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIR})
include_directories(${PYSTRING_INCLUDE_DIR})
include_directories(${CORE_INCLUDE_DIRS})

target_link_libraries(${MAYAPLUGIN} ${PYSTRING_LIBS} ${JSONCPP_LIBS} ${Boost_REGEX_LIBRARY} ${MAYA_LIBS} ${CORE_LIBS})
set_target_properties(${MAYAPLUGIN} PROPERTIES PREFIX "")
set_target_properties(${MAYAPLUGIN} PROPERTIES SUFFIX ${MAYA_PLUGIN_SUFFIX})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(${MAYAPLUGIN} OpenGl32)
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
