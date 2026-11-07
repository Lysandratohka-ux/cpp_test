set(bundle "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Infinity.app")
set(helper "${bundle}/Contents/Helpers/QtWebEngineProcess")
if(EXISTS "${helper}")
 execute_process(COMMAND otool -l "${helper}" OUTPUT_VARIABLE load_commands COMMAND_ERROR_IS_FATAL ANY)
 if(NOT load_commands MATCHES "@executable_path/../Frameworks")
  execute_process(COMMAND install_name_tool -add_rpath "@executable_path/../Frameworks" "${helper}" COMMAND_ERROR_IS_FATAL ANY)
 endif()
endif()
# Deploying dylibs changes their load commands and invalidates upstream signatures.
# Sign the final nested bundle only after all binaries and resources are in place.
execute_process(COMMAND codesign --force --deep --sign - "${bundle}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND codesign --verify --deep --strict "${bundle}" COMMAND_ERROR_IS_FATAL ANY)
message(STATUS "Infinity final bundle: nested signatures verified")
