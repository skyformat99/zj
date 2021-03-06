# - Append compiler flag to CMAKE_C_FLAGS if compiler supports it
# ADD_C_FLAG_IF_SUPPORTED(<flag>)
#  <flag> - the compiler flag to test
# This internally calls the CHECK_C_COMPILER_FLAG macro.

INCLUDE(CheckCCompilerFlag)

MACRO(ADD_C_FLAG_IF_SUPPORTED _FLAG)
    STRING(TOUPPER ${_FLAG} UPCASE)
    STRING(REGEX REPLACE "^-" "" UPCASE_PRETTY ${UPCASE}) 
    CHECK_C_COMPILER_FLAG(${_FLAG} IS_${UPCASE_PRETTY}_SUPPORTED)

    IF(IS_${UPCASE_PRETTY}_SUPPORTED)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_FLAG}")
    ENDIF()
ENDMACRO()
