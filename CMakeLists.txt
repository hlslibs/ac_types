cmake_minimum_required(VERSION 3.16)

set(AC_TYPES_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PANDA_INSTALL_NAMESPACE}/ac_types")
set(AC_TYPES_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${PANDA_INSTALL_NAMESPACE}")
set(AC_TYPES_INCLUDE_UTIL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${PANDA_INSTALL_NAMESPACE}/utils")

set(AC_TYPES_DATA_FILES
   LICENSE
   LICENSE_INSTALL
   NOTICE
   pdfdocs/ac_datatypes_ref.pdf
   pdfdocs/ac_datatypes_relnotes.pdf
)

set(AC_TYPES_HEADERS
   include/ac_channel.h
   include/ac_complex.h
   include/ac_fixed.h
   include/ac_float.h
   include/ac_int.h
   include/ac_sc.h
   include/ap_fixed.h
   include/ap_int.h
   include/ap_shift_reg.h
   include/hls_stream.h
)
set(AC_TYPES_UTIL
   include/utils/x_hls_utils.h
)


install(FILES ${AC_TYPES_DATA_FILES}
        DESTINATION "${AC_TYPES_INSTALL_DIR}")
install(FILES ${AC_TYPES_HEADERS}
        DESTINATION "${AC_TYPES_INCLUDE_DIR}")
install(FILES ${AC_TYPES_UTIL}
        DESTINATION "${AC_TYPES_INCLUDE_UTIL_DIR}")
