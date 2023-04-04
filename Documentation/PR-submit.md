# PR organization and submit guideline
This guideline talking about:
* Pre-checks / minimum requirement expectains before submitting the PR.
* How to organize PR be independent and with right size/scope 
* Why stacked PRs is **not** supported nor is needed

## Pre-check before submitting PR
1. All CI build and test jobs run against the PR shall succeed. The CI build job
   is building: current main branch + your PR code. Build failed PR will NOT be
   able to land. 
2. No lint issues, all lint issues w/o fixing need comments provide legitimate
   reason why we shall ignore this lint issue now. I.e. It is a linter bug
   reporting false positive signals, while it is preferred to provide solutions
   to reduce this noise in the future.
3. Including proper test plan and test result. Unit test included is preferred,
   especially for hardware independent common codes.


## Organization of PR
### PR, Commit and Diff
PR is referring to the **github pull request**, it normally consists of: title,
summary, test plan and code. The code normally consists of one or multiple **git
commit(s)** on one branch. Each PR will get imported into META as **Phabricator
Diff**, after imported all commits consisting the PR will be squashed together
and landed as one commit in META’s internal main git repository and synced to
ODM’s git repository on main trunk.

### No Combo PR
~~Combo_PR~~ which stacking two PRs will be rejected
```
 --+------------------------- main
   |
   +- (c1) - (c2) - (C3) - (C4) PR_branch
               ^             ^
             PR1            PR2 (PR2 stack on PR1)
```
Please submit two independent PRs
```
 --+------------------------- main
   |\
   | + - (C3) - (C4) PR2_branch
   +- (c1) - (c2) PR1_branch
```

Logically each PR shall be implementing only one thing, I.e. add one feature,
fix one bug, improve one part etc. A common practice is checking whether the PR
title can describe what the PR is implementing. So don’t try to combine codes
doing different things in one jumbo PR. i.e. ( combine “Add a group of new
GPIOs” with “change some GPIO name” as one combo PR). Although it is quite
tempting to submit PRs touching the same file as a combo one, this kind of combo
PR will bring trouble in the case we need to revert some changes, i.e. later
don’t want to change the GPIO name. Or just check whether some change has been
included in the release or troubleshooting some regression.

Also it is tempting for you to keep on working on one branch for a milestone
(i.e. Power on, EVT build etc). And suddenly find that it is not easy to split
the changes made on the same branch into multiple PRs, though you know that the
changes shall be submitted as multiple PRs. So

* Always think about whether you shall create a dedicated **new branch** from
  the current main trunk before starting to work on something different.
* Don’t be afraid of multiple PRs merging into trunk creating conflicts. Merging
  PRs shall never be trouble or need a lot of code changes or efforts. If so,
  this normally signals a bad design here. Either you have two features coupled
  with each other unnecessary with bad interfaces or an extra new PR is required
  to take care of some common function.

### Split big PR into commits
~~Giant Commit~~ which contains 500+ lines of significant changes
```
 --+------------------------- main
   |
   +- ( GIANT...COMMIT) PR_branch
```
Please split them into increasemental commits
```
 --+------------------------- main
   |
   +- (c1) - (c2) - (c3) - (c4) PR_branch
```
We prefer one PR consisting of small, incremental commits, this can help code
reviewers follow your implementation progress when reviewing the PR. A decent
size of commit is less than 200 lines significant code changes. It is NOT
required the code will build on each commit within a PR.

The exception is that you are importing third party package. But it is required
that the first big importing PR only contains original version of third part
code. Submit following on PRs including your changes to the original code after
the first importing PR is landed.

### No stacked PRs
```
 --+------------------------- main
   |
   +- (c1) - (c2) + PR1_branch
                  |
                  +- (c3) - (c4) PR2_branch
                  |
                  +- (c5) - (c6) PR3_branch
```
It is common you are working on f1 and f2, and notice that f2 has quite decent
dependencies on common code of f1. This is the situation previously mentioned
*“an extra new PR is required to take care of some common function”*. As the
diagram showed, in this situation you will submit PR1 as a common function,
**after PR1 landed**, then submit PR2 and PR3 at the same time. But you may want
to have the three PRs get reviewed and collect feedback early to save
uncecessary code review turn around time. Send email to maintainer to
request a preview of your WIP PR_branches, **do not** submit stacked PRs.

All of these general guideline apply to both OpenBMC and OpenBIC projects.

Maintainers email:
* openbmc_compute@meta.com


