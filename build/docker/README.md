Custom Docker Image for CI
==========================

What is this for?

In order to run the build and test operations for this project, a certain set
of utilities are needed in the docker image. There is no pre-existing image
that has everything needed. To resolve this I can:

- break up CI pipeline into steps with each step using a different image
- add extra `apt-get install` commands in the pipeline script (takes longer)
- prepare my own docker image that has exactly the tools needed

The Dockerfile here can be used to build an image that is used for this
project's CI operations.

The image, once built, needs to be pushed to the gitlab container registry.
These are the steps to follow:

### Login

Using your gitlab credentials:

    (sudo) docker login -u <username> registry.gitlab.com

you will be prompted for the password.

### Build

    (sudo) docker build -t registry.gitlab.com/kroesche/bmsnode .

### Push

    (sudo) docker push registry.gitlab.com/kroesche/bmsnode

### Usage

In the `.gitlab-ci.yml` file, use this image:

    image: registry.gitlab.com/kroesche/bmsnode

Notes
-----

- Depending on your docker setup, you may or may not need `sudo`.

- After the build step, it is a good idea to run it locally and exercise the
  container to make sure it has all the packages you need and executes the
  build correctly.

- If you have 2FA enabled on the account you will need to set up a personal
  access token for authentication.

- It would be a good idea to automate all this as a separate job, but not
  worth it for now.

- For more info, google `gitlab ci container registry`
