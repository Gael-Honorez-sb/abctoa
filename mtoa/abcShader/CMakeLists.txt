set(MTOATRANSLATOR abcShader)

file(GLOB SRC "*.cpp" "*h")

add_library(${MTOATRANSLATOR} SHARED ${SRC})

target_compile_definitions(${MTOATRANSLATOR} PUBLIC REQUIRE_IOSTREAM _BOOL)  

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
    target_compile_definitions(${MTOATRANSLATOR} PUBLIC LINUX LINUX_64) 
endif()

include_directories(${MAYA_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${MTOA_INCLUDE_DIR})
include_directories(${ARNOLD_INCLUDE_DIR})


target_link_libraries(${MTOATRANSLATOR} ${MTOA_LIBS} ${MAYA_LIBS} ${ARNOLD_LIBS})
set_target_properties(${MTOATRANSLATOR} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${MTOATRANSLATOR} RUNTIME DESTINATION ${TRANSLATOR_INSTALL_DIR})
else()
	install(TARGETS ${MTOATRANSLATOR} DESTINATION ${TRANSLATOR_INSTALL_DIR})
endif()
