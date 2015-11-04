#!/usr/bin/env python
# Copyright (C) 2015 LiveCode Ltd.
#
# This file is part of LiveCode.
#
# LiveCode is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3 as published by the Free
# Software Foundation.
#
# LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with LiveCode.  If not see <http://www.gnu.org/licenses/>.

import subprocess
import re
import sys
import os.path
import argparse

GIT='git'
BRANCH_RE='^develop.*'
TMP_BRANCH_FORMAT='submodule-tmp/{remote}/{branch}'

verbosity = 1
dry_run = True

################################################################
# Helper functions
################################################################

def msg_debug(message):
    if verbosity >= 1:
        print('debug: {}'.format(message))

def msg_normal(message):
    if verbosity >= 0:
        print(message)

def git_cmd(path, cmd):
    args = [GIT] + cmd
    msg_debug(' '.join(args))
    return subprocess.check_output(args, cwd=path)

def create_commit(path):
    message = 'Auto-update submodule pointers'
    cmd = ['commit', '--all', '--message', message]
    git_cmd(path, cmd)

def push_branch(path, remote, src, dest):
    cmd = ['push', remote, '{}:{}'.format(src,dest)]
    git_cmd(path, cmd)

################################################################
# Get info about the local/remote repositories
################################################################

def is_repo_clean(path):
    cmd = ['status', '--porcelain']
    output = git_cmd(path, cmd)
    return len(output.splitlines()) == 0

def get_branches(path, remote):
    cmd = ['branch', '--list', '--remotes', '{}/*'.format(remote)]
    output = git_cmd(path, cmd)

    prefix = '{}/'.format(remote)
    def process_branch(branch):
        return branch.split()[0].replace(prefix,'')

    return [process_branch(x) for x in output.splitlines()]

def get_candidate_branches(path, remote):
    expr = re.compile(BRANCH_RE)
    return [x
            for x in get_branches(path, remote)
            if expr.match(x) is not None]

################################################################
# Submodule manipulation
################################################################

def init_submodules(path):
    cmd = ['submodule', 'update', '--init']
    git_cmd(path, cmd)

# Clean up stale submodules.  This is needed to cope with the fact
# that prebuilts is a submodule in some branches but not in others.
# Stale submodules are detected when a submodule exists in .git/config
# but not in .submodules
def clean_submodules(path):
    init_submodules(path)

    cmd = ['config', '--local', '--list']
    mainconfig = git_cmd(path, cmd)

    cmd = ['config', '--file', '.gitmodules', '--list']
    moduleconfig = git_cmd(path, cmd)

    expr = re.compile('^submodule\.([^\.]*)\.')

    current_submodules = []
    for config in moduleconfig.splitlines():
        match = expr.match(config)
        if match is None:
            continue
        if match.group(1) in current_submodules:
            continue
        current_submodules.append(match.group(1))

    known_submodules = []
    for config in mainconfig.splitlines():
        match = expr.match(config)
        if match is None:
            continue
        if match.group(1) in known_submodules:
            continue
        known_submodules.append(match.group(1))

    cleaned = False
    for mod in known_submodules:
        if mod in current_submodules:
            continue

        # Okay, the module is (probably) stale, so blow it away
        msg_debug('removing {}'.format(mod))
        subprocess.check_call(['rm', '-rf', mod], cwd=path)

        cleaned = True

    # If anything was deleted, reset the tree
    if cleaned:
        cmd = ['reset', '--hard']
        git_cmd(path, cmd)

def sync_submodules(path):
    # Use the .gitmodules file to get a list of submodules that
    # can actually be synced
    cmd = ['config', '--file', '.gitmodules', '--list']
    moduleconfig = git_cmd(path, cmd)

    expr = re.compile('^submodule\.([^\.]*)\.branch')
    for config in moduleconfig.splitlines():
        match = expr.match(config)
        if match is None:
            continue

        # Modules which have remote branch information get
        # synchronised
        submodule = match.group(1)
        cmd = ['submodule', 'update', '--remote', submodule]
        git_cmd(path, cmd)

################################################################
# Branch manipulation
################################################################

def checkout_working_branch(path, remote, branch):
    working_name = TMP_BRANCH_FORMAT.format(remote=remote, branch=branch)
    cmd = ['branch', '--force', working_name, '{}/{}'.format(remote, branch)]
    git_cmd(path, cmd)

    cmd = ['checkout', '--force', working_name]
    git_cmd(path, cmd)

    cmd = ['clean', '--force', '-x', '-d']
    git_cmd(path, cmd)

    clean_submodules(path)

    return working_name

################################################################
# Top-level worker functions
################################################################

def process_branch(path, remote, branch):
    # Checkout a new local branch to work in
    tmp_branch = checkout_working_branch(path, remote, branch)

    # Update submodules to match the remote branches
    sync_submodules(path)

    # If the working tree is clean, skip to the next branch
    if is_repo_clean(path):
        msg_normal('ok - {} # SKIP no changes'.format(branch))
        return

    create_commit(path)

    if not dry_run:
        push_branch(path, remote, tmp_branch, branch)
        msg_normal('ok - {}'.format(branch))
    else:
        msg_normal('ok - {} # SKIP dry-run mode'.format(branch))

def process_repository(path, remote, branches=None):
    # Make sure all submodules have been initialised and checked out
    init_submodules(path)

    # Fetch from the remote, removing any branches that don't exist
    # any more.
    cmd = ['fetch', '--prune', remote]
    git_cmd(path, cmd)

    # Process each branch
    if branches is None:
        branches = get_candidate_branches(path, remote)

    for branch in branches:
        process_branch(path, remote, branch)

################################################################
# Main program
################################################################

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Automatically update submodule pointers.',
        epilog="""
Automatically update all submodule pointers in the branches of the
specified REMOTE repository.  With `-n', doesn't push any changes to
the remote repository.  Use `-v' to get debugging info about what the
tool is doing.  If you don't specify a BRANCH, all `develop' branches
will be updated.
""")
    parser.add_argument('remote', help='name of remote repo to update',
                        metavar='REMOTE', default='origin', nargs='?')
    parser.add_argument('branches', help='name of a remote branch to update',
                        metavar='BRANCH', nargs='*')
    parser.add_argument('-n', help='disable pushing changes to remote repo',
                        action='store_true', dest='dry_run')
    parser.add_argument('-C', metavar='PATH', help='run in a different directory',
                        nargs=1, default='.', dest='repo')
    parser.add_argument('-v', help='increase debugging info',
                        action='count', default=0, dest='verbosity')

    args = parser.parse_args()

    verbosity = args.verbosity
    dry_run = args.dry_run
    repo_path = args.repo
    remote = args.remote
    if len(args.branches) > 0:
        branches = args.branches
    else:
        branches = None

    if not is_repo_clean(repo_path):
        print(
"""Working tree is not clean.

You must run this tool in a clean repository""")
        sys.exit(1)

    process_repository(repo_path, remote, branches)
