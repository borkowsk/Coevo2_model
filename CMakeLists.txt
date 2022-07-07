# CMAKE configuration for COEVO-lution2 simulation project
######################################################################
# @Author: Wojciech Borkowski
#

cmake_minimum_required(VERSION 2.9)

project(coevolution2)

set(WBRTM "../../public/symShell2andRTM/RTM/")
set(SYMSHELL "../../public/symShell2andRTM/SYMSHELL/")

add_subdirectory( ${SYMSHELL} ${SYMSHELL} )

include_directories( "${SYMSHELL}"
                     "${WBRTM}/INCLUDE"
                )

add_executable( coevo2
                "src/Coewo200N.cxx"
                "src/troficinfo.cpp"
                "src/klonalinfo.cpp"
                "src/kladystyka.cpp"
                "src/ekologia.cpp"
                )
                
target_link_libraries( coevo2 
     "-L${SYMSHELL}"
     wbsyshX11 X11 Xpm
     rt
     )                
     

