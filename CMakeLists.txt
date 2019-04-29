# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.

cmake_minimum_required (VERSION 2.8.11)
project (RTP_TEST)

if(JACK_LIBRARIES AND JACK_INCLUDE_DIRS)

# in cache already
set(JACK_FOUND TRUE)

else()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_JACK jack)
endif(PKG_CONFIG_FOUND)

find_path(JACK_INCLUDE_DIR
  NAMES
    jack/jack.h
  PATHS
    ${_JACK_INCLUDEDIR}
)
  
find_library(JACK_LIBRARY
  NAMES
    jack
  PATHS
    ${_JACK_LIBDIR}
)

set(JACK_INCLUDE_DIRS
  ${JACK_INCLUDE_DIR}
)

set(JACK_LIBRARIES
  ${JACK_LIBRARY}
)

find_package_handle_standard_args(Jack DEFAULT_MSG JACK_LIBRARIES JACK_INCLUDE_DIRS)

# show the JACK_INCLUDE_DIRS and JACK_LIBRARIES variables only in the advanced view
mark_as_advanced(JACK_INCLUDE_DIR JACK_LIBRARY JACK_INCLUDE_DIRS JACK_LIBRARIES)

endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

include_directories(/opt/local/include)
link_directories(/opt/local/lib)

endif()

# Recurse into the "Hello" and "Demo" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (librtp)
add_subdirectory (libspeex)

# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx". The extensions are automatically found.
add_executable (client client.c codec.c codec_speex.c codec_lpcm.c audio_alsa.c audio_jack.c helper.c)
add_executable (server server.c codec.c codec_speex.c codec_lpcm.c audio_alsa.c audio_jack.c helper.c)

# Link the executable to the Hello library. Since the Hello library has
# public include directories we will use those link directories when building
# helloDemo

#target_link_libraries (client LINK_PUBLIC rtp)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
target_link_libraries (client rtp speex jack)      
target_link_libraries (server rtp speex jack)      
endif()


if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

find_package(ALSA REQUIRED)                
#if (ALSA_FOUND && JACK_FOUND)                                                
    #set(COMPILE_DEFINITIONS=ENABLE_ALSA)
include_directories(${ALSA_INCLUDE_DIRS})                  
include_directories(${JACK_INCLUDE_DIRS})                  
target_link_libraries (client rtp speex m ${ALSA_LIBRARIES} ${JACK_LIBRARIES})      
target_link_libraries (server rtp speex m ${ALSA_LIBRARIES} ${JACK_LIBRARIES})      
#endif(ALSA_FOUND)  

endif()
