include_guard(GLOBAL)

function(pex_enable_build_info target repo_root)
    if(CMAKE_VERSION VERSION_LESS "3.12")
        find_package(PythonInterp 3 REQUIRED)
        set(_pex_python "${PYTHON_EXECUTABLE}")
    else()
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        set(_pex_python "${Python3_EXECUTABLE}")
    endif()

    get_filename_component(_pex_repo_root "${repo_root}" ABSOLUTE)
    set(_pex_build_info_header "${_pex_repo_root}/build/generated/pex_build_info.h")
    set(_pex_build_info_target "${target}_build_info")

    add_custom_target(${_pex_build_info_target} ALL
        COMMAND "${_pex_python}"
                "${_pex_repo_root}/tools/generate_build_info.py"
                --root "${_pex_repo_root}"
                --output "${_pex_build_info_header}"
        BYPRODUCTS "${_pex_build_info_header}"
        COMMENT "Generating PexCraft Git build metadata"
        VERBATIM
    )
    add_dependencies(${target} ${_pex_build_info_target})

    # The normal source defaults remain empty. Builders force-include this
    # generated header so commit subjects can contain spaces/quotes safely.
    target_compile_options(${target} PRIVATE
        "$<$<COMPILE_LANGUAGE:C>:-include>"
        "$<$<COMPILE_LANGUAGE:C>:${_pex_build_info_header}>"
    )
endfunction()
