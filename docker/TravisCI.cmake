set(CTEST_PROJECT_NAME "Cap")
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")

set(CTEST_SOURCE_DIRECTORY "$ENV{PREFIX}/source/cap")
set(CTEST_BINARY_DIRECTORY "$ENV{PREFIX}/build/cap")

set(CMAKE_ARGS "")
set(CMAKE_ARGS "${CMAKE_ARGS} -D CMAKE_BUILD_TYPE=Debug")
set(CMAKE_ARGS "${CMAKE_ARGS} -D CMAKE_CXX_COMPILER=mpicxx")
set(CMAKE_ARGS "${CMAKE_ARGS} -D BUILD_SHARED_LIBS=ON")
set(CMAKE_ARGS "${CMAKE_ARGS} -D ENABLE_COVERAGE=ON")
set(CMAKE_ARGS "${CMAKE_ARGS} -D ENABLE_FORMAT=ON")
set(CMAKE_ARGS "${CMAKE_ARGS} -D BOOST_DIR=$ENV{BOOST_DIR}")
set(CMAKE_ARGS "${CMAKE_ARGS} -D ENABLE_PYTHON:BOOL=ON")
set(CMAKE_ARGS "${CMAKE_ARGS} -D PYTHON_LIBRARY=$ENV{PYTHON_DIR}/lib/libpython3.5.so")
set(CMAKE_ARGS "${CMAKE_ARGS} -D PYTHON_INCLUDE_DIR=$ENV{PYTHON_DIR}/include/python3.5")
set(CMAKE_ARGS "${CMAKE_ARGS} -D ENABLE_DEAL_II=ON")
set(CMAKE_ARGS "${CMAKE_ARGS} -D DEAL_II_DIR=$ENV{DEAL_II_DIR}")
set(CMAKE_ARGS "${CMAKE_ARGS} -D CAP_DATA_DIR=$ENV{PREFIX}/source/cap-data")
#set(CMAKE_ARGS "${CMAKE_ARGS} -D CMAKE_CXX_FLAGS=\\\"-Wall -Wextra\\\"")
find_program(CTEST_COVERAGE_COMMAND NAMES gcov)
set(CTEST_CONFIGURE_COMMAND "${CMAKE_COMMAND} ${CMAKE_ARGS} ${CTEST_SOURCE_DIRECTORY}")

set(CTEST_SITE "travis-ci")
set(CTEST_BUILD_NAME "$ENV{CTEST_BUILD_NAME}")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "jupyterdocker.ornl.gov/CDash")
set(CTEST_DROP_LOCATION "/submit.php?project=Cap")
set(CTEST_DROP_SITE_CDASH TRUE)

if("$ENV{TRAVIS_PULL_REQUEST}" MATCHES "false") 
    ctest_start("Continuous")
else()
    ctest_start("Experimental")
endif()
ctest_configure()
ctest_build()
ctest_test()
#ctest_coverage()
ctest_submit()
