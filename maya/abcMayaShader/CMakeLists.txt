set(MAYAPLUGIN abcMayaShader)

file(GLOB SRC "*.cpp" "*h")

add_library(${MAYAPLUGIN} SHARED ${SRC})

target_compile_definitions(${MAYAPLUGIN} PUBLIC REQUIRE_IOSTREAM _BOOL)  

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
    target_compile_definitions(${MAYAPLUGIN} PUBLIC LINUX LINUX_64) 
endif()

include_directories(${Boost_INCLUDE_DIR})
include_directories(${MAYA_INCLUDE_DIR})
include_directories(${CORE_INCLUDE_DIRS})
include_directories(${MTOA_INCLUDE_DIR})
include_directories(${PYSTRING_INCLUDE_DIR})
include_directories(${ARNOLD_INCLUDE_DIR})

target_link_libraries(${MAYAPLUGIN} ${PYSTRING_LIBS} ${JSONCPP_LIBS} ${ARNOLD_LIBS} ${MTOA_LIBS} ${MAYA_LIBS} ${CORE_LIBS})
set_target_properties(${MAYAPLUGIN} PROPERTIES PREFIX "")
set_target_properties(${MAYAPLUGIN} PROPERTIES SUFFIX ${MAYA_PLUGIN_SUFFIX})
        
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${MAYAPLUGIN} RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR})
else()
	install(TARGETS ${MAYAPLUGIN} DESTINATION ${PLUGINS_INSTALL_DIR})
endif()
