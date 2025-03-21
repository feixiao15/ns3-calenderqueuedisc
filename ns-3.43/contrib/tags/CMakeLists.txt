check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/tags-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME tags
    SOURCE_FILES model/tags.cc
                 helper/tags-helper.cc
    HEADER_FILES model/tags.h
                 helper/tags-helper.h
    LIBRARIES_TO_LINK ${libcore}
    TEST_SOURCES test/tags-test-suite.cc
                 ${examples_as_tests_sources}
)
