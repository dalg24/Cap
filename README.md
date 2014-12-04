cap
===
DEPENDENCIES
------------
deal.II compiled with C++11 support. The development sources can be found 
[here](https://github.com/dealii/dealii).

CONFIGURE, BUILD and TEST
-------------------------

    $ export PREFIX=/path/to/some/working/directory/for/the/project
    $ mkdir -p ${PREFIX}/source

Install deal.II

    $ cd ${PREFIX}
    $ git pull https://github.com/dealii/dealii ${PREFIX}/source/dealii
    $ mkdir build/dealii
    $ cd build/dealii
    $ cmake -DCMAKE_INSTALL_PREFIX=${PREFIX}/install/dealii ${PREFIX}/source/dealii
    $ make install    (alternatively $ make -j<N> install)

Configure and build cap

    $ cd ${PREFIX}
    $ git pull https://github.com/dalg24/cap source/cap ${PREFIX}/source/cap
    $ mkdir build/cap
    $ cd build/cap
    $ cmake -DDEAL_II_INSTALL_DIR=${PREFIX}/install/dealii ${PREFIX}/source/cap
    $ make -j<N>

Run tests

    $ ctest -j<N>

AUTHORS
-------
* [Damien Lebrun-Grandie](https://github.com/dalg24)
