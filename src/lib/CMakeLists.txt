
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
endif()

# Create a library called "mmwave_com_frame"
# The extension is already found. Any number of sources could be listed here.
set(LIB_SOURCES frame_layer.c crc_16.c)
add_library(mmwave_com_frame ${LIB_SOURCES})
install(TARGETS mmwave_com_frame DESTINATION lib)
install (FILES inc/frame_layer.h inc/frame_layer_types.h inc/crc_16.h inc/payload_ids.h DESTINATION include/mmwave)

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories(mmwave_com_frame PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
  $<INSTALL_INTERFACE:include/mmwave>  # <prefix>/include/mylib
)

if(COMMAND add_sp_lint)
 add_sp_lint(lib ${LIB_SOURCES})
endif(COMMAND add_sp_lint)
