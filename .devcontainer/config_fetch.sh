#!/usr/bin/env bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set -x

REPO_DIR="/workspaces/ori"
# check the workspace exists
if [ -e "$REPO_DIR" ]; then
    git remote -v | grep origin | grep -q 'git@github.com'
    if [[ "$?" == 0 ]] ; then
        # ensure we fetch through ssh for git packages
        # if we cloned through ssh (presumably we don't have
        # https configured for package fetch)

        # Go fetches packages from github. Since ori is currently a private project
        # using ssh will fetch with authentication.
        git config --global url.git@github.com:.insteadOf https://github.com/
    else
        # ensure we fetch through https for git packages
        # if we cloned through https, since we must have https
        # auth configured
        git config --global --unset url.git@github.com:.insteadOf
    fi
fi
