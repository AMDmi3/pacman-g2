#  - macro_asciidoc2man(inputfile outputfile)
#
#  Create a manpage with asciidoc.
#  Example: macro_asciidoc2man(foo.txt foo.1)
#
# Copyright (c) 2006, Andreas Schneider, <mail@cynapses.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(ASCIIDOC2MAN _a2m_input _a2m_output _a2m_install_dir)

    message("+++ ${ASCIIDOC_A2X_EXECUTABLE} -D ${CMAKE_CURRENT_BINARY_DIR} -f manpage ${CMAKE_CURRENT_SOURCE_DIR}/${_a2m_input}")

    execute_process(
      COMMAND  ${ASCIIDOC_A2X_EXECUTABLE} -D ${CMAKE_CURRENT_BINARY_DIR} -f manpage ${CMAKE_CURRENT_SOURCE_DIR}/${_a2m_input}
      RESULT_VARIABLE A2M_MAN_GENERATED
    )

    message("+++ A2M_MAN_GENERATED: ${A2M_MAN_GENERATED}")
    if (A2M_MAN_GENERATED EQUAL 0)
      find_file(A2M_MAN_FILE
        NAME  ${_a2m_output}
        PATHS ${CMAKE_CURRENT_BINARY_DIR}
        NO_DEFAULT_PATH
      )

      if (A2M_MAN_FILE)
        message("+++ found file")
        install(
          FILES ${CMAKE_CURRENT_BINARY_DIR}/${_a2m_output}
          DESTINATION ${_a2m_install_dir}
        )
      endif (A2M_MAN_FILE)
    endif (A2M_MAN_GENERATED EQUAL 0)

endmacro(ASCIIDOC2MAN _a2m_input _a2m_file)
