# Release branching policy

The term `develop` applies to any develop branch - be it *develop-6.7*, *develop-7.0*, *develop* or any other development branch that might appear in the future.

## DP state

As long as the release built from a develop branch bears the DP label, a new release of this version does not require the creation of a new branch (the branch from which the releases are built is still in development).

Any bug fix should be made against the develop branch.

## RC state

When the first RC is released, a new branch release-*x.y.z* is created from the tag of this RC release. That allows the merging of any new bug fix or feature implementation to be done without affecting the release branch.

Only regressions (bugs introduced in the version *x.y.z*) can be fixed against release-*x.y.z*; any other bug fix must be made against the develop branch, in order to avoid an everlasting regression-fixing state.

Merging release-*x.y.z* branch into its develop branch should be done at least after each new RC release (but can also be done at any time).

## Multiple develop branches

Let's consider branches develop-A, develop-B and their respective release-*A* and release-*B* equivalent, where B is a greater version that A.

### DP state

Before building any building any release from develop-*B*:
  * for any submodule, develop-*A* must be merged into develop-*B*
  * develop-*A* must be merged into develop-*B*, and the submodule updated to the result of their merge.

The commit message resulting from a develop-to-develop merge should keep track of the conflicts, and have a list of any refactoring applied, to make the reviewing an easier task.

### RC state

Some considerations must also be taken when develop-*A* (and potentially develop-*B*) branch is in at an RC state:

Before a release built from release-*B*:
  * release-*A* is merged up into release-*B*

After the release:
  * release-*A* is merged into develop-*A*
  * release-*B* is merged into develop-*B* (if release-*B* exists)
  * develop-*A* is merged into develop-*B*
 
This workflow will minimise the size of the merges to review, and make sure that all the branches are updated appropriately.
