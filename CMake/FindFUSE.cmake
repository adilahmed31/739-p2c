find_path (FUSE_INCLUDE_DIR fuse.h
/usr/local/include/fuse3)


find_library (FUSE_LIBRARIES NAMES fuse refuse PATHS /usr/local/lib/x86_64-linux-gnu/)


include ("FindPackageHandleStandardArgs")
find_package_handle_standard_args ("FUSE" DEFAULT_MSG
        FUSE_INCLUDE_DIR FUSE_LIBRARIES)

mark_as_advanced (FUSE_INCLUDE_DIR FUSE_LIBRARIES)

