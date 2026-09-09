set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "qtscribe")
set(CPACK_PACKAGE_VENDOR "Vidhan")
set(CPACK_PACKAGE_CONTACT "Vidhan <rautvidhan31@gmail.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Vidhan31/qtscribe")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern Wayland speech-to-text transcription assistant powered by Qt 6")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/qtscribe")

set(CPACK_DEBIAN_PACKAGE_NAME "qtscribe")
set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Vidhan <rautvidhan31@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Modern Wayland speech-to-text transcription assistant powered by Qt 6.\n QtScribe provides global speech-to-text voice dictation with streaming\n audio recording, dynamic audio level visualization, and seamless Wayland\n virtual keystroke injection via keyinjectord.")

# Since we bundle private Qt 6.11 libraries in /opt/qtscribe/lib, disable dpkg-shlibdeps
# to prevent it from looking for private Qt symbols in distro system directories.
# Instead, define explicit curated runtime dependencies for Ubuntu 24.04:
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.38), libstdc++6 (>= 13.2.0), libgcc-s1 (>= 3.3.1), libgl1, libegl1, libvulkan1, libfontconfig1, libfreetype6, libglib2.0-0t64, libdbus-1-3, libasound2t64, libpulse0, libwayland-client0, libwayland-cursor0, libwayland-egl1, libxkbcommon0, libsecret-1-0, libevdev2, libcap2, libcap2-bin, wl-clipboard")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/prerm;${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postrm"
)

set(CPACK_DEBIAN_DEBUGINFO_PACKAGE OFF)
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
