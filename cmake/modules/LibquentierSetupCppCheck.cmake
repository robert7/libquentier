if(NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")    # clang is not supported by cppcheck
  include(CppcheckTargets)

  ## using only source files for cppcheck as a first attempt
  set(SOURCE_FILE_REGEX "\\.cpp")

  MACRO(ADD_CPPCHECK_TEST FILE_TO_TEST)
    string( REGEX MATCH ${SOURCE_FILE_REGEX} is_source_file ${FILE_TO_TEST} )
    if(is_source_file)
      add_cppcheck_sources(${FILE_TO_TEST} ${FILE_TO_TEST} FAIL_ON_WARNINGS)
    endif(is_source_file)
  ENDMACRO()

  set(CPPCHECK_INCLUDEPATH_ARG ${LIBQUENTIER_CPPCHECKABLE_INCLUDE_DIRS})

  list(REMOVE_DUPLICATES LIBQUENTIER_CPPCHECKABLE_SOURCES)
  foreach(i ${LIBQUENTIER_CPPCHECKABLE_SOURCES})
    add_cppcheck_test(${i})
  endforeach()
endif()
