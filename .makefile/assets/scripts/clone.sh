#!/bin/bash -e
##==================================================================================
## MIT License
##
## Copyright (c) 2019 Quentin Quadrat <lecrapouille@gmail.com>
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
##==================================================================================

### This script wraps git clone to avoid arguments to be passed.
###
### Arguments:
### $1: project name
### $2: repository name
### $3: destination path
### $4: reference
### $5: recursive

readonly GITHUB_URL="https://github.com"

# Parse command line arguments
PROJECT_NAME=$1
REPO=$2
DEST_PATH=$3
shift 3

function fatal()
{
    echo -e "\033[35mFailure! $1\033[00m"
    exit 1
}

CLONE_ARGS=("$@")

# Check if repository should be cloned recursively
RECURSIVE=false
if [[ "$REPO" == *":recurse"* ]]; then
    RECURSIVE=true
    REPO=${REPO/:recurse/}
fi

# Parse repository and reference
REPO_NAME=${REPO%@*}
REF=${REPO#*@}

# If no reference specified, use default branch
if [ "$REPO_NAME" = "$REPO" ]; then
    REF=""
fi

echo -e "\033[35m*** Cloning: \033[36m$GITHUB_URL/$REPO_NAME\033[00m => \033[33m$PROJECT_NAME\033[00m"
if [ -n "$REF" ]; then
    echo -e "\033[35m*** Using reference: \033[36m$REF\033[00m"
fi
if [ "$RECURSIVE" = true ]; then
    echo -e "\033[35m*** Cloning recursively\033[00m"
    CLONE_ARGS+=("--recursive")
fi

# Extract repo name for directory
DIR_NAME=${REPO_NAME##*/}
REPO_PATH="$DEST_PATH/$DIR_NAME"
EXPECTED_REMOTE="$GITHUB_URL/$REPO_NAME"

# Update existing repository when destination exists
if [ -d "$REPO_PATH" ]; then
    echo -e "\033[35m*** Updating existing repository: \033[36m$REPO_PATH\033[00m"
    if [ ! -d "$REPO_PATH/.git" ]; then
        fatal "Destination path exists but is not a git repository: $REPO_PATH"
    fi

    CURRENT_REMOTE=$(git -C "$REPO_PATH" remote get-url origin 2>/dev/null || echo "")

    if [ -n "$CURRENT_REMOTE" ] && [ "$CURRENT_REMOTE" != "$EXPECTED_REMOTE" ]; then
        fatal "Existing repository remote '$CURRENT_REMOTE' differs from expected '$EXPECTED_REMOTE'"
    fi

    (
        cd "$REPO_PATH"

        if [ -z "$CURRENT_REMOTE" ]; then
            git remote add origin "$EXPECTED_REMOTE" > /dev/null
        fi

        git fetch origin --tags --prune > /dev/null 2>&1

        if [ -n "$REF" ]; then
            if [[ "$REF" =~ ^[0-9a-f]{40}$ ]]; then
                git fetch origin "$REF" > /dev/null 2>&1
                git checkout -B "sha1-$REF" "$REF" > /dev/null
                echo -e "\033[35m*** Fetched SHA1: \033[36m$REF\033[00m"
            else
                git fetch origin "$REF" > /dev/null 2>&1 || true
                if git show-ref --verify --quiet "refs/heads/$REF"; then
                    git checkout "$REF" > /dev/null 2>&1
                    git pull --ff-only origin "$REF" > /dev/null 2>&1
                elif git show-ref --verify --quiet "refs/remotes/origin/$REF"; then
                    git checkout -B "$REF" "origin/$REF" > /dev/null 2>&1
                else
                    git checkout "$REF" > /dev/null 2>&1
                fi
                echo -e "\033[35m*** Checked out branch: \033[36m$REF\033[00m"
            fi
        else
            git pull --ff-only --no-tags > /dev/null 2>&1
            echo -e "\033[35m*** Pulled default branch\033[00m"
        fi
    )
    exit 0
fi

# Clone with specified reference if provided
if [ -n "$REF" ]; then
    # Check if ref is a SHA1 (40 characters hex)
    if [[ "$REF" =~ ^[0-9a-f]{40}$ ]]; then
        # For SHA1, use partial clone and create a branch
        git clone --filter=blob:none "$GITHUB_URL/$REPO_NAME" "$DEST_PATH/$DIR_NAME" "${CLONE_ARGS[@]}" > /dev/null
        (cd "$DEST_PATH/$DIR_NAME" && \
            git fetch origin "$REF" > /dev/null && \
            git checkout -b "sha1-$REF" "$REF" > /dev/null)
    else
        # For branches and tags, clone with specific reference
        git clone "$GITHUB_URL/$REPO_NAME" --branch "$REF" "$DEST_PATH/$DIR_NAME" "${CLONE_ARGS[@]}" > /dev/null || \
        git clone "$GITHUB_URL/$REPO_NAME" "$DEST_PATH/$DIR_NAME" "${CLONE_ARGS[@]}" > /dev/null && \
        (cd "$DEST_PATH/$DIR_NAME" && git checkout "$REF" > /dev/null)
    fi
else
    # For default branch, use shallow clone
    git clone "$GITHUB_URL/$REPO_NAME" --depth=1 "$DEST_PATH/$DIR_NAME" "${CLONE_ARGS[@]}" > /dev/null
fi