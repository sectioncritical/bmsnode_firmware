Release Checklist
=================

* all feature branches merged to master and pushed
* CI passes build and test stages
* CI log for build job looks ok
* Ci log for test job looks ok
* test/reports artifacts:
    * test/reports/check - cppcheck report is okay (no unexpected errors)
    * test/reports/bmstest-junit.html report shows no failed unit tests
    * test/reports/bmstest-coverage.html shows TBD acceptable coverage
* download build artifacts `bmsnode-x.y.z.tar.gx`
* use DFU to load package image onto TBD test boards
  `make DFU ADDR=n PKG=x.y.x`
* perform TBD release acceptance tests
* examine release notes in the package
* if all prior steps pass, run the "release" job on GitLab CI
* after release job runs, on gitlab.com examine the newly commited tag, and
  the generate release for correctness
* download release binaries and compare to binaries from tested package and
  ensure they are the same
