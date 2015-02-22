set(SHADER arnoldAlembicShader)

set(SRC abcshader.cpp loader.cpp ../../../common/abcshaderutils.h ../../../common/abcshaderutils.cpp)


include_directories(${Boost_INCLUDE_DIR})
include_directories(${ALEMBIC_INCLUDE_DIR})
include_directories(${ILMBASE_INCLUDE_DIRECTORY})

add_library(${SHADER} SHARED ${SRC})
target_link_libraries(${SHADER} ai ${ALEMBIC_LIBRARIES} ${ALEMBIC_HDF5_LIBS} ${ILMBASE_LIBS} ${ZLIB_LIBRARIES} ${SZIP_LIBRARIES})
set_target_properties(${SHADER} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${SHADER} RUNTIME DESTINATION ${SHADERS_INSTALL_DIR})
else()
	install(TARGETS ${SHADER} DESTINATION ${SHADERS_INSTALL_DIR})
endif()
