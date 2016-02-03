FUNCTION(ADD_CAP_BOOST_TEST TEST_NAME)
    ADD_EXECUTABLE(${TEST_NAME}.exe ${PROJECT_SOURCE_DIR}/test/${TEST_NAME}.cc)
    TARGET_LINK_LIBRARIES(${TEST_NAME}.exe cap)
    TARGET_LINK_LIBRARIES(${TEST_NAME}.exe ${Boost_MPI_LIBRARY})
    TARGET_LINK_LIBRARIES(${TEST_NAME}.exe ${Boost_SERIALIZATION_LIBRARY})
    TARGET_LINK_LIBRARIES(${TEST_NAME}.exe ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    TARGET_LINK_LIBRARIES(${TEST_NAME}.exe ${Boost_TIMER_LIBRARY})
    TARGET_LINK_LIBRARIES(${TEST_NAME}.exe ${Boost_SYSTEM_LIBRARY})
    TARGET_LINK_LIBRARIES(${TEST_NAME}.exe ${Boost_CHRONO_LIBRARY})
    IF(ENABLE_DEAL_II)
        TARGET_LINK_LIBRARIES(${TEST_NAME}.exe deal_II)
    ENDIF()
    IF(ENABLE_GSL)
        TARGET_LINK_LIBRARIES(${TEST_NAME}.exe ${GSL_LIBRARIES})
    ENDIF()
    IF(ENABLE_TASMANIAN)
        TARGET_LINK_LIBRARIES(${TEST_NAME}.exe ${TASMANIAN_LIBRARIES})
    ENDIF()
    IF(ARGN)
        SET(NUMBER_OF_PROCESSES_TO_EXECUTE ${ARGN})
    ELSE()
        SET(NUMBER_OF_PROCESSES_TO_EXECUTE 1)
    ENDIF()
    FOREACH(PROCS ${NUMBER_OF_PROCESSES_TO_EXECUTE})
        ADD_TEST(
            NAME ${TEST_NAME}_cpp_${PROCS}_procs
            COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${PROCS} ./${TEST_NAME}.exe
        )
    ENDFOREACH()
ENDFUNCTION()

FUNCTION(COPY_CAP_INPUT_FILE INPUT_FILE)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${INPUT_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/data/${INPUT_FILE}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${CMAKE_CURRENT_SOURCE_DIR}/data/${INPUT_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${INPUT_FILE}
        COMMENT "Copying ${INPUT_FILE}"
    )
    ADD_CUSTOM_TARGET(
        ${INPUT_FILE} ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${INPUT_FILE}
    )
ENDFUNCTION()
