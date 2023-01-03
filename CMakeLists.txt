# CMAKE configuration for COEVO-lution2 simulation project
######################################################################
# @author	: Wojciech Borkowski
# @date	: 2023-01-03 (last modification)

cmake_minimum_required(VERSION 3.0)

project(coevolution2)

set(CMAKE_CXX_STANDARD 11) # See: https://matgomes.com/setting-cpp-standard-cmake/
#set(CMAKE_CXX_STANDARD_REQUIRED On)
#set(CMAKE_CXX_EXTENSIONS Off)


IF (WIN32)
  # set stuff for windows
  message( "MS Windows platform assumed.\n\tInside C/C++ code automatic _MSC_VER variable will be used!" )
  set( WBSLIBS   "../../public/symShell2andRTM" )
  set( WBRTM     "${WBSLIBS}/RTM")
  set( SYMSHELL  "${WBSLIBS}/SYMSHELL" )
  set( SYMSCLSS  "${WBSLIBS}/SYMSHELL_CLASES" )
  add_compile_definitions( _CRT_SECURE_NO_WARNINGS )
  add_definitions( -DUSE_WINDOWS )
  #set( WINDOWS_ONLY_SRC  "${WBRTM}/BASE/wb_assert.c" )
ELSE()
  # set stuff for other systems
  message( "Linux/unix platform assumed.")
  set( WBSLIBS   "../../public/symShell2andRTM" )
  set( WBRTM     "${WBSLIBS}/RTM")
  set( SYMSHELL  "${WBSLIBS}/SYMSHELL" )
  set( SYMSCLSS  "${WBSLIBS}/SYMSHELL_CLASES" )
  add_definitions( -DHAVE_X11 -DUSE_ZENITY ) # -DUSE_X11  DUSE_CONSOLE_ONLY -DDEF_MAXTHREADS=16 -DMULTITR
  #add_compile_definitions( ... )
  #set( X_ONLY_SRC  "" )
ENDIF()

#add_subdirectory( ${} ${} )
add_subdirectory( "${SYMSHELL}/" "${SYMSHELL}/" )
add_subdirectory( "${SYMSCLSS}/" "${SYMSCLSS}/" )
add_subdirectory( "${WBRTM}/"   "${WBRTM}/"  )

include_directories( "${SYMSCLSS}/DATACLSS" "${SYMSCLSS}/SIMULCLSS" "${SYMSCLSS}/VISUALCLSS"
                     "${WBRTM}/INCLUDE"  "${SYMSHELL}"
                    )

add_executable( coevo2
                "src/Coewo200N.cxx"
                "src/troficinfo.cpp"
                "src/klonalinfo.cpp"
                "src/kladystyka.cpp"
                "src/ekologia.cpp"
                ${WINDOWS_ONLY_SRC}
                ${X_ONLY_SRC}
                )
                
#target_link_libraries( coevo2 
#     "-L${SYMSHELL}"
#     wbsyshX11 X11 Xpm rt
#     )                
     
IF (WIN32)
#target_compile_options( coevo2  PRIVATE ...)
target_link_libraries( coevo2
#       "-L ${MYLIBS}"
        wbsyshclss wbsyshLight  wbrtm
#       ${CMAKE_THREAD_LIBS_INIT}
     )
ELSE()
target_compile_options( coevo2  PRIVATE "${CMAKE_CXX_FLAGS}" "-pthread" "-fpermissive")
target_link_libraries( coevo2
#        "-L ${MYLIBS}"
        wbsyshclss wbsyshLight  wbrtm
        X11 Xpm
        ${CMAKE_THREAD_LIBS_INIT}
        pthread
        rt
     )
ENDIF()



