*** Settings ***
Library         Process
Suite Setup     Build Host Test Executable

*** Variables ***
${ROOT}             ${CURDIR}${/}..
${TEST_BINARY}      ${ROOT}${/}build${/}button_logic_test.exe

*** Test Cases ***
Short Press Is Reported Once
    Run Button Logic Test    short-press

Long Press Is Reported After 500 Milliseconds
    Run Button Logic Test    long-press

Press Duration Boundary Is 500 Milliseconds
    Run Button Logic Test    threshold-boundary

Idle And Held Button Behave Correctly
    Run Button Logic Test    idle-and-held-button

Press Edge Is Reported Once Per Press
    Run Button Logic Test    pressed-edge-once

Short And Long Events Are Independent
    Run Button Logic Test    independent-events

*** Keywords ***
Build Host Test Executable
    ${result}=    Run Process    make    -C    ${ROOT}    build/button_logic_test.exe
    Should Be Equal As Integers    ${result.rc}    0
    ...    Host test build failed.\n${result.stdout}\n${result.stderr}

Run Button Logic Test
    [Arguments]    ${scenario}
    ${result}=    Run Process    ${TEST_BINARY}    ${scenario}    cwd=${ROOT}
    Should Be Equal As Integers    ${result.rc}    0
    ...    ${result.stdout}\n${result.stderr}
    Should Contain    ${result.stdout}    PASS: ${scenario}
