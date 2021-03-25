=====================================
OpenBMC User Interface Review Process
=====================================

Overview
========

Previously OpenBMC User Interfaces (RESTAPI and CLI) may be added/changed
without notifying the relevant stakeholders, and this has been leading to
unpleasant experiences, such as breaking the existing use cases, or even
corrupt the OpenBMC system.

The document proposes guidelines to improve the OpenBMC User Interface
Reivew Process.

Scope
=====

"User Interface" in this document refers to REST APIs and CLI tools which are
intended to be called from outside OpenBMC. People should follow the guidlines
in this document when User Interfaces need to be changed, including:

* adding a new User Interface.

* deleting an existing User Interface.

* changing the input arguments of an existing User Interface.

* changing the behavior of an existing User Interface.

* changing the output/response of an existing User Interface.

Services and commands that are designed to be used inside OpenBMC are not User
Interfaces, and they are not covered by this document.

Review Process
==============

When people send out DIFF(s) or Pull Requests which involve OpenBMC User
Interface changes, they should:

1) Post a RFC in "OpenBMC Dev" group to raise requirements and connect
   stakeholders from different teams.

2) CIT test must be added/updated in the diff (or stacked diffs) to ensure
   the User Interface meets the requirement, and won’t be broken unexpectedly
   in the future.

   NOTE::

        you don't have to wait for all the stateholders to explicitly accept
        the diff(s). As long as nobody raises concerns, you are okay to land
        the diffs.

3) Bring the topic to the next "bi-weekly openbmc sync-up meeting", and this
   is to help connecting stakeholders who missed the post and diffs.

Defining Requirements - Best Practices
======================================

Stakeholders for high risk changes will tend to span different teams and
knowledge domains (e.g. not everyone will have participated in all meetings
regarding the change or be familiar with all moving parts involved).

To that end, documenting which teams need the new API and why (what external
motivations drive them to need this change) is vital to provide context and
reduce communications overhead, especially when onboarding new members and
stakeholders.

If the new interfaces are replacing existing ones, it’s also good to provide
examples mapping how the current logic maps to the new interface (as people
familiar with the previous logic will likely understand the new interface
faster). This also has the advantage of making explicit whatever advantages
of the new approach.

Defining Functional Requirements
--------------------------------

Functional requirements are true/false statements which should map to the
expected inputs/outputs (functions) the system is expected to handle, for
example:

* The function MUST accept X, Y, Z as arguments

* new_script.sh MUST exit with code != 0 on errors and print a user-readable
  reason in the last line of stderr with format X

* flashy MUST check if all preconditions to safely flash the device are OK
  before proceeding with an upgrade

Note that these are all true/false statements that directly translate to the
expected logic of the application.

Defining Non-functional Requirements
------------------------------------

Non-functional requirements (e.g. SLAs, resource usage) are true/false
statements that don’t directly map to application functionality but are no
less important aspects of the application. For example:

* The application will initially run in, at most, 32 tupperware server hosts
  of type X

* The p99 response time for this endpoint should be less than X seconds

* Unit test coverage must be > 90%

* This endpoint should be called at most X times/minute

* Peak RSS should be less than X MB over Y hours of uptime

As a general note about requirements, how many and how specific those are is
up to the person writing them and the target audience. What’s important is
that they should be as easily verifiable as possible and reflect important
aspects/risks of the proposed change.

As with most models, optimising for usefulness (e.g. finding the ones that
everyone needs to be aware of, and are likely to lead a constructive
discussion) tends to work best.
