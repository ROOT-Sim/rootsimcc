find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_XML YES)
    set(DOXYGEN_PROJECT_LOGO img/rs.png)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET flatdoc.css inline-params.css)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ../README.md)
    doxygen_add_docs(doc
            "../src"
            "../README.md"
            ALL
            COMMENT "Generate documentation")
endif()
