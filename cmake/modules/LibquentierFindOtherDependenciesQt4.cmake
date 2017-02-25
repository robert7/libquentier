if("${LIBQUENTIER_FIND_PACKAGE_ARG}" STREQUAL "")
  set(LIBQUENTIER_FIND_PACKAGE_ARG "QUIET")
else()
  set(LIBQUENTIER_FIND_PACKAGE_ARG "")
endif()

LibquentierFindPackageWrapper(QEverCloud-qt4 ${LIBQUENTIER_FIND_PACKAGE_ARG})
LibquentierFindPackageWrapper(QtKeychain ${LIBQUENTIER_FIND_PACKAGE_ARG})
LibquentierFindPackageWrapper(qt4-mimetypes ${LIBQUENTIER_FIND_PACKAGE_ARG})
