BMSNode Firmware Release Process
================================

`master` is used for making release tags.

The release process uses GitLab CI to run the necessary scripts. Everything
that is done by the CI processing can also be done locally first. Use the
Makefile to do builds and test prior to pushing and attempting a release.

The release process and Makefile are run from the "build" directory. See the
Makefile and `make help` to see available make targets.

The release process does the following high level steps:

* build the firmware
* run unit test and other checks and generate reports
* generate a version number and changelog using this tool:
  https://juhani.gitlab.io/go-semrel-gitlab/
* package up the artifacts into a release package, named by version, that can
  be downloaded from gitlab. The release package contains  firmware binaries,
  listings and a changelog. Other notes and instructions can be added in the
  future.
* once the release is approved, tag the commit used to build the release,
  and add links to the release package on gitlab

Normal Release Process
----------------------

Given that there is a set of changes **on a feature branch** that is based on
`master`:

1. rebase feature branch as needed or before attempting a release
2. annotate commit messages with fix, feat, etc per this:
   https://juhani.gitlab.io/go-semrel-gitlab/commit-message/
3. do whatever local testing is necessary
4. push the feature branch
5. verify release package is correctly built by GitLab CI, and looks correct
6. download release package and use the built firmware to run any necessary
   release functional tests
7. examine generated CHANGELOG
8. if feature branch is okay:
9. squash the feature branch onto `master`
10. edit the squash commit message to reflect the set of commit type
    annotations (feat, fix, etc) to reflect a sensible story about the set
    of changes in the squash (the changes of the feature branch)
11. push the updated `master`
12. verify GitLab CI build is okay
13. download generated release package and verify firmware and CHANGELOG
14. if final release package is okay:
15. Manually trigger the release stage of the GitLab CI pipeline
16. Verify tag was created and has correct releaase notes. Verify GitLab
    release was created.
17. after some time, delete the feature branch and any saved artifacts on
    GitLab. This is to keep down the number of saved artifacts to avoid
    confusion.

Specific Steps
--------------

Givens:

* operating in the "build" directory
* on a feature branch
* all commits made, repo is clean
* all prior feature branches since last release tag have been merged
* testing has been done, there is reasonable certainty that the branch is
  ready for release
* branch has been pushed to GitLab and CI has run without errors

Steps:

1. `make changelog` to generate the CHANGELOG.md
2. review CHANGELOG and verify correct
3. CHANEGLOG will now have modified git status. Re-checkout from repo
4. squash feature branch onto master, or ff merge if branch commit history is
   important
5. push master
6. verify CI is good for latest master push
7. using GitLab CI UI, run the manual "release" job for the last pipeline
8. verify tag was created, and GitLab release with release notes and artifacts
