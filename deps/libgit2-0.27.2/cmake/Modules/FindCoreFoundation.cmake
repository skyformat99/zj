# Find CoreFoundation.framework
# This will define :
#
# COREFOUNDATION_FOUND
# COREFOUNDATION_LIBRARIES
# COREFOUNDATION_LDFLAGS
#

FIND_PATH(COREFOUNDATION_INCLUDE_DIR NAMES CoreFoundation.h)
FIND_LIBRARY(COREFOUNDATION_LIBRARIES NAMES CoreFoundation)
IF (COREFOUNDATION_INCLUDE_DIR AND COREFOUNDATION_LIBRARIES)
    IF (NOT CoreFoundation_FIND_QUIETLY)
        MESSAGE("-- Found CoreFoundation ${COREFOUNDATION_LIBRARIES}")
    ENDIF()
    SET(COREFOUNDATION_FOUND TRUE)
    SET(COREFOUNDATION_LDFLAGS "-framework CoreFoundation")
ENDIF ()

IF (CoreFoundation_FIND_REQUIRED AND NOT COREFOUNDATION_FOUND)
    MESSAGE(FATAL "-- CoreFoundation not found")
ENDIF()

MARK_AS_ADVANCED(
    COREFOUNDATION_INCLUDE_DIR
    COREFOUNDATION_LIBRARIES
)
