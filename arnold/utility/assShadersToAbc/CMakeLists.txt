set(UTIL assShadersToAbc)

file(GLOB SRC "*.cpp" "*h" "../../../common/abc*.h" "../../../common/abc*.cpp")


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

include_directories(${CMAKE_SOURCE_DIR}/common)

include_directories(${CMAKE_SOURCE_DIR}/thirdparty/ezOptionParser)

include_directories(${ARNOLD_INCLUDE_DIR})

add_executable(${UTIL} ${SRC})
target_link_libraries(${UTIL} ${ARNOLD_LIBRARY} Alembic jsoncpp_lib_static pystring_lib_static Iex Half)
set_target_properties(${UTIL} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${UTIL} RUNTIME DESTINATION ${DSO_INSTALL_DIR})
else()
	

install(TARGETS ${UTIL} DESTINATION ${DSO_INSTALL_DIR})
endif()

