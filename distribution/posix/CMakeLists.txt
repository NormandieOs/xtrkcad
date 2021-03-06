# Setup packaging ...
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "XTrackCAD is a CAD program for designing model railroad layouts.")
set(CPACK_GENERATOR "DEB" "RPM" "STGZ")
set(CPACK_SOURCE_GENERATOR "TGZ" "TBZ2" "ZIP")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_LICENSE "GPL")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}////xtrkcad-desc.txt")
set(CPACK_PACKAGE_FILE_NAME "xtrkcad-setup-${XTRKCAD_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")

set(CPACK_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} )
set(CPACK_PACKAGING_INSTALL_PREFIX ${CPACK_INSTALL_PREFIX})

# get the specific configuration for each packager
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/package.txt)

set(CPACK_PACKAGE_VENDOR "XTrackCAD Fork Project")

# common dependencies for all Linux builds
set(CPACK_RPM_PACKAGE_REQUIRES "glibc gtk2 zip zlib")

set(CPACK_RPM_BUILDREQUIRES 
    "gcc, cmake >= 2.4.7, pkgconfig, gtk2-devel" 
    "gettext, gettext-devel, glibc-devel"
)

# Add dependency to webkitgtk if configured
if(NOT XTRKCAD_USE_BROWSER)
    set(CPACK_RPM_PACKAGE_REQUIRES 
        "${CPACK_RPM_PACKAGE_REQUIRES} webkitgtk")
#    message(AUTHOR_WARNING "Add dependency to webkitgtk for viewing help!")
endif()

# exclude these directories from the rpm
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
	"/usr/local"
	"/usr/local/share"
	"/usr/local/bin" 
	"/usr/share/locale"
	"/usr/share/locale/*"
	"/usr/local/share/locale"
	"/usr/local/share/locale/*"
	"/usr/share/applications"
	"/usr/share/pixmaps"
)

set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_CURRENT_SOURCE_DIR}////changelog.txt")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_DEBIAN_PACKAGE_NAME "XTrkCad")

set(CPACK_DEBIAN_PACKAGE_VERSION "1:${XTRKCAD_VERSION}")

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "zip, xdg-utils")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "xtrkcad@sillub.com")

set(CPACK_PACKAGE_CONTACT "xtrkcad@sillub.com")

set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "XTrkCad supports any scale, has libraries of popular brands of \
turnouts and sectional track (plus you add your own easily), can \
automatically use spiral transition curves when joining track \
XTrkCad lets you manipulate track much like you would with actual \
flex-track to modify, extend and join tracks and turnouts. \
Additional features include tunnels, 'post-it' notes, on-screen \
ruler, parts list, 99 drawing layers, undo/redo commands, \
benchwork, 'Print to BitMap', elevations, train simulation and \
car inventory. \
")


set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/graphics")

set(CPACK_DEBIAN_PACKAGE_COMPRESSION "gzip")

set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "xtrkcad.org")

set(CPACK_PACKAGE_EXECUTABLES "xtrkcad;XTrkCad-5.2.1-Beta")

set(CPACK_CREATE_DESKTOP_LINKS "xtrkcad")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst;${CMAKE_CURRENT_SOURCE_DIR}/prerm")

include(CPack)

