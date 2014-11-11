set(SHADER arnoldAlembicShader)

file(GLOB SRC "*.cpp" "*h")

include_directories(${Boost_INCLUDE_DIR})
include_directories(${ALEMBIC_INCLUDE_DIR})
include_directories(${ILMBASE_INCLUDE_DIRECTORY})

add_library(${SHADER} SHARED ${SRC})
target_link_libraries(${SHADER} ai ${ALEMBIC_LIBRARIES} ${ALEMBIC_HDF5_LIBS} ${ILMBASE_LIBS} ${ZLIB_LIBRARIES} ${SZIP_LIBRARIES})
set_target_properties(${SHADER} PROPERTIES PREFIX "")

install(TARGETS ${SHADER} DESTINATION ${SHADERS_INSTALL_DIR})