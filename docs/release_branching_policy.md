# Release Branching Policy

The term *develop* applies to any develop branch - be it *develop-6.7*, *develop-7.0*, *develop* or any other development branch that might appear in the future.

## DP state

As long as the release built from a *develop* branch bears the DP label, a new release of this version does not require the creation of a new branch (the branch from which the release is built is still in development).

Any bug fix is made against the *develop* branch.

## RC state

When the first RC is released, a new branch release-*x.y.z* is created, branch off at the tag of this first RC release. That allows the merging of any new bug fix or feature implementation to be done without affecting the release branch.

Only regressions (bugs introduced in the LiveCode *x.y.z*) can be fixed against release-*x.y.z*; any other bug fix must be made against the develop branch, in order to avoid an everlasting regression-fixing state.

Merging release-*x.y.z* branch into its develop branch must be done after each new RC release; there is no harm in doing it more often though.



## Multiple develop branches

It might occur that several active develop branches exists in the repo, from which releases are built. 

Let's consider branches develop-*A*, develop-*B*, where *B* is a greater version that *A*.

The following up-merges have to be done in order to assure that regressions fixes are present in both *A* and *B*.

### *A* in DP state, *B* in DP state

Before building a release for *B*:

  1. for any submodule, develop-*A* must be merged into develop-*B*
  
  2. develop-*A* must be merged into develop-*B*, and the submodule references updated


### *A* in RC state, *B* in DP state

Before building a release for *B*:

  1. for any submodule, release-*A.y.z* must be merged into develop-*B*
  
  2. release-*A.y.z* must be merged into develop-*B*, and the submodule references updated


### *A* in RC state, *B* in RC state

  1. for any submodule, release-*A.y.z* must be merged into release-*B.y.z*

  2. release-*A.y.z* is merged up into release-*B.y.z*, and the submodules references updated

----

The commit message resulting from an upmerge should keep track of the merge conflicts, and have a list of any unreviewed change applied during the merge, in order to make the reviewing an easier task.
