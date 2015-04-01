set(PROC arnoldAlembicProcedural)

file(GLOB SRC "*.cpp" "*h" "../../../common/*.h" "../../../common/*.cpp")

include_directories(${ALEMBIC_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${ILMBASE_INCLUDE_DIRECTORY})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${PYSTRING_INCLUDE_DIR})


add_library(${PROC} SHARED ${SRC})
target_link_libraries(${PROC} ai ${ALEMBIC_LIBRARIES} ${ALEMBIC_HDF5_LIBS} ${JSONCPP_LIBRARIES} ${PYSTRING_LIBRARIES} ${ILMBASE_LIBS} ${ZLIB_LIBRARIES} ${SZIP_LIBRARIES} ${Boost_REGEX_LIBRARY} ${Boost_ATOMIC_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY})
set_target_properties(${PROC} PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(TARGETS ${PROC} RUNTIME DESTINATION ${DSO_INSTALL_DIR})
else()
	

install(TARGETS ${PROC} DESTINATION ${DSO_INSTALL_DIR})
endif()

