SWIG_ADD_MODULE(pacman_perl perl ../pacman.i)
SET_SOURCE_FILES_PROPERTIES(pacman.i PROPERTIES SWIG_FLAGS "-includeall")
include_directories(${PERL_INCLUDE_PATH})
SWIG_LINK_LIBRARIES(pacman_perl pacman)
install(TARGETS ${SWIG_MODULE_pacman_perl_REAL_NAME} DESTINATION lib)
