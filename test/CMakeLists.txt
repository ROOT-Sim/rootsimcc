# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
include_directories(.)

function(test_program name)
    add_executable(test_${name} ${ARGN})
    target_link_libraries(test_${name} reflect)
    add_test(NAME test_${name} COMMAND test_${name})
    set_tests_properties(test_${name} PROPERTIES TIMEOUT 50)
endfunction()
