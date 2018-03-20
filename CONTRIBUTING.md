CONTRIBUTING.md

# Contributing to OpenBMC
We want to make contributing to this project as easy and transparent as
possible.

## Our Development Process
We develop on a private branch internally at Facebook. We regularly update
this github project with the changes from the internal repo. External pull
requests are cherry-picked into our repo and then pushed back out.

## Pull Requests
We actively welcome your pull requests.
1. Fork the repo and create your branch from `helium`.
2. If you've added code that should be tested, add tests
3. If you've changed APIs, update the documentation.
4. Ensure the test suite passes.
5. Make sure your code lints.
6. If you haven't already, complete the Contributor License Agreement ("CLA").

## Contributor License Agreement ("CLA")
In order to accept your pull request, we need you to submit a CLA. You only need
to do this once to work on any of Facebook's open source projects.

Complete your CLA here: <https://code.facebook.com/cla>

## Issues
We use GitHub issues to track public bugs. Please ensure your description is
clear and has sufficient instructions to be able to reproduce the issue.

Facebook has a [bounty program](https://www.facebook.com/whitehat/) for the safe
disclosure of security bugs. In those cases, please go through the process
outlined on that page and do not file a public issue.

## Coding Style
* 2 spaces for indentation rather than tabs
* 80 character line length

## Code of Conduct

Facebook has adopted a Code of Conduct that we expect project participants to adhere to. Please [read the full text](https://code.facebook.com/pages/876921332402685/open-source-code-of-conduct) so that you can understand what actions will and will not be tolerated.

## License
OpenBMC is made up of different packages. Each package contains recipe files
that detail where to fetch source code from third party sources or local
directories. The recipe files themselves are provided under the GPLv2
license, but your use of the code fetched by each recipe file is subject to
the licenses of each respective third party project or as defined in the local
directory.
