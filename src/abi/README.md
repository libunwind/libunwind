ABI Baseline Files
==================

This source directory contains the ABI baseline files used to detect
incompatible ABI (application binary interface) changes in libunwind.

These files are generated using the **libabigail** ABI generic analysis and
instrumentation library tools https://sourceware.org/libabigail/

Checking the ABI
----------------

1. Install the **libabigail** tools, at least version 2.0.
   On Ubuntu you can use the following command
    ```
    $ sudo apt install abigail-tools
    ```
    and a similar command with other package managers or you can clone the project
    from upstream and build it yourself.

2. Configure and build libunwind
    ```
    $ mkdir build; cd build
    $ ../configure CFLAGS="-g -Og"
    $ make -j
    ```

3. Run the ABI checks
    ```
    $ make -C src abi-check
    ```
    A report for each .so file will be generated into the corresponding .abidiff
    file in the build-tree's src directory.
