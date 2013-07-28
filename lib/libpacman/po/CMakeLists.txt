#translations
file (STRINGS "LINGUAS" languages)
foreach(lang IN LISTS languages)
  GETTEXT_PROCESS_PO_FILES(${lang} ALL INSTALL_DESTINATION share/locale PO_FILES ${lang}/libpacman.po)
endforeach(lang)

install(TARGETS pacman DESTINATION lib)
install(FILES pacman.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pacman.pc DESTINATION lib/pkgconfig)

