# - Find SZIP library
# Find the native SZIP includes and library
# This module defines
#  SZIP_INCLUDE_DIR, where to find hdf5.h, etc.
#  SZIP_LIBRARIES, libraries to link against to use SZIP.
#  SZIP_FOUND, If false, do not try to use SZIP.
# also defined, but not for general use are
#  SZIP_LIBRARY, where to find the SZIP library.

FIND_PATH(SZIP_INCLUDE_DIR szlib.h PATHS ${SZIP}/include)
SET(SZIP_NAMES ${SZIP_NAMES} sz libszip szip)
FIND_LIBRARY(SZIP_LIBRARY NAMES ${SZIP_NAMES} PATHS ${SZIP}/lib )

# handle the QUIETLY and REQUIRED arguments and set SZIP_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SZIP DEFAULT_MSG SZIP_LIBRARY SZIP_INCLUDE_DIR)

IF(SZIP_FOUND)
  SET( SZIP_LIBRARIES ${SZIP_LIBRARY} )
ENDIF(SZIP_FOUND)
