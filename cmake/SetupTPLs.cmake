#### Message Passing Interface (MPI) #########################################
find_package(MPI REQUIRED)

#### Boost ###################################################################
if(DEFINED BOOST_DIR)
    set(BOOST_ROOT ${BOOST_DIR})
endif()
set(Boost_COMPONENTS
    mpi
    serialization
    unit_test_framework
    chrono
    timer
    system
    filesystem
    iostreams
    regex
)
if(ENABLE_PYTHON)
    set(Boost_COMPONENTS python ${Boost_COMPONENTS})
endif()
find_package(Boost 1.59.0 REQUIRED COMPONENTS ${Boost_COMPONENTS})

#### Python ##################################################################
if(ENABLE_PYTHON)
    find_package(PythonInterp REQUIRED)
    find_package(PythonLibs REQUIRED)
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "from mpi4py import get_include; print(get_include())"
        OUTPUT_VARIABLE MPI4PY_INCLUDE_DIR
    )
    find_path(MPI4PY_INCLUDE_DIR mpi4py/mpi4py.h 
        PATH ${MPI4PY_INCLUDE_DIR}
        NO_DEFAULT
    )
    if(MPI4PY_INCLUDE_DIR)
        message("MPI4PY_INCLUDE_DIR=${MPI4PY_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "mpi4py not found.")
    endif()
endif()

#### deal.II #################################################################
if(ENABLE_DEAL_II)
    find_package(deal.II 8.4 REQUIRED PATHS ${DEAL_II_DIR})
    add_definitions(-DWITH_DEAL_II)
    # If deal.II was configured in DebugRelease mode, then if Cap was configured
    # in Debug mode, we link against the Debug version of deal.II. IF Cap was
    # configured in Release mode, we link against the Release version of deal.II
    string(FIND "${DEAL_II_LIBRARIES}" "general" SINGLE_DEAL_II)
    if (${SINGLE_DEAL_II} EQUAL -1)
        if(CMAKE_BUILD_TYPE MATCHES "Release")
            set(DEAL_II_LIBRARIES ${DEAL_II_LIBRARIES_RELEASE})
        else()
            set(DEAL_II_LIBRARIES ${DEAL_II_LIBRARIES_DEBUG})
      endif()
    endif()
  find_package(OpenMP)
  if(OPENMP_FOUND)
    set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}"
      )
    set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}"
      )
  else()
    message(SEND_ERROR "Could not find OpenMP required by cuSolver")
  endif()
endif()
