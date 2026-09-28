#!/bin/bash -e
##=============================================================================
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
##==============================================================================

### This script is called by (make download-external-libs). It will read the
### manifest file and clone the repositories set in each line. It will then wait
### for all the repositories to be cloned before exiting.
### Manifest file format: one repository per line: user/repo[:recurse][@ref]
### Examples:
###   angeluriot/Dimension3D@84b20021f08aa89755fae83c39fc59a815f54033
###   SFML/imgui-sfml:recurse@2.6.x
###
### Note: repositories are GitHub only.

###############################################################################
### Input of this script:
### $1: target (your application name).
### $2: manifest file containing list of repositories to clone.
###############################################################################
GITHUB_URL="https://github.com"
PROJECT_NAME=$1
MANIFEST_FILE=$2

function fatal()
{
    echo -e "\033[35mFailure! $1\033[00m"
    exit 1
}

###############################################################################
# Force git to be completely silent
###############################################################################
export GIT_TERMINAL_PROMPT=0
export GIT_QUIET=1
export GIT_SSH_COMMAND="ssh -o LogLevel=ERROR"
export GIT_TRACE=0
export GIT_TRACE_SETUP=0
export GIT_TRACE_PERFORMANCE=0
export GIT_TRACE_PACKET=0
export GIT_TRACE_PACKFILE=0
export GIT_TRACE_PACK_ACCESS=0
export GIT_TRACE_PERFORMANCE=0
export GIT_TRACE_SETUP=0

###############################################################################
# Post-clone operations: fetch, checkout and update submodules if recursive
###############################################################################
function post_clone_operations()
{
    local dir_name=$1
    local ref=$2
    local recursive=$3

    (cd "$dir_name" && {
        if [[ "$ref" =~ ^[0-9a-f]{40}$ ]]; then
            # SHA1: fetch, checkout and update submodules if recursive
            git fetch --quiet origin "$ref" && \
            git checkout --quiet -b "sha1-$ref" "$ref" 2> /dev/null && \
            ([ "$recursive" = true ] && git submodule update --init --recursive --quiet || true)
        else
            # Branch/tag: checkout and update submodules if recursive
            git checkout --quiet "$ref" && \
            ([ "$recursive" = true ] && git submodule update --init --recursive --quiet || true)
        fi
    })
}

###############################################################################
# Clone repository
###############################################################################
function clone_repo()
{
    local repo=$1
    shift
    local clone_args=("$@")

    # Check if repository should be cloned recursively
    local recursive=false
    if [[ "$repo" == *":recurse"* ]]; then
        recursive=true
        repo=${repo/:recurse/}
        clone_args+=("--recursive")
    fi

    # Parse repository and reference
    local repo_name=${repo%@*}
    local ref=${repo#*@}

    # If no reference specified, use default branch
    if [ "$repo_name" = "$repo" ]; then
        ref=""
    fi

    # Determine clone message
    local clone_msg="Cloning"
    [ "$recursive" = true ] && clone_msg="Cloning recursive"

    local ref_msg="[HEAD]"
    [ -n "$ref" ] && ref_msg="[$ref]"

    # Extract repo name for directory
    local dir_name=${repo_name##*/}
    local repo_path="$dir_name"
    local expected_remote="$GITHUB_URL/$repo_name"

    if [ -d "$repo_path" ]; then
        echo -e "\033[35m*** Updating: \033[36m$expected_remote\033[00m => \033[33m$PROJECT_NAME \033[37m$ref_msg\033[00m"
        if [ ! -d "$repo_path/.git" ]; then
            fatal "Destination path exists but is not a git repository: $repo_path"
        fi

        local current_remote
        current_remote=$(git -C "$repo_path" remote get-url origin 2>/dev/null || echo "")

        if [ -n "$current_remote" ] && [ "$current_remote" != "$expected_remote" ]; then
            fatal "Existing repository remote '$current_remote' differs from expected '$expected_remote'"
        fi

        (
            cd "$repo_path"

            if [ -z "$current_remote" ]; then
                git remote add origin "$expected_remote" > /dev/null
            fi

            git fetch --quiet origin --tags --prune

            if [ -n "$ref" ]; then
                if [[ "$ref" =~ ^[0-9a-f]{40}$ ]]; then
                    git fetch --quiet origin "$ref"
                    git checkout --quiet -B "sha1-$ref" "$ref" 2> /dev/null || git checkout --quiet "sha1-$ref"
                    echo -e "\033[35m*** Fetched SHA1: \033[36m$ref\033[00m"
                else
                    git fetch --quiet origin "$ref" || true
                    if git show-ref --verify --quiet "refs/heads/$ref"; then
                        git checkout --quiet "$ref"
                        git pull --quiet --ff-only origin "$ref"
                    elif git show-ref --verify --quiet "refs/remotes/origin/$ref"; then
                        git checkout --quiet -B "$ref" "origin/$ref"
                    else
                        git checkout --quiet "$ref"
                    fi
                    echo -e "\033[35m*** Checked out branch: \033[36m$ref\033[00m"
                fi
            else
                git pull --quiet --ff-only --no-tags
                echo -e "\033[35m*** Pulled default branch\033[00m"
            fi

            if [ "$recursive" = true ]; then
                git submodule update --init --recursive --quiet
            fi
        )
        return
    fi

    echo -e "\033[35m*** $clone_msg: \033[36m$GITHUB_URL/$repo_name\033[00m => \033[33m$PROJECT_NAME \033[37m$ref_msg\033[00m"

    # Clone repository
    if [ -n "$ref" ]; then
        if [[ "$ref" =~ ^[0-9a-f]{40}$ ]]; then
            # SHA1: partial clone
            git clone --quiet --filter=blob:none "$GITHUB_URL/$repo_name" "${clone_args[@]}" 2> /dev/null
        else
            # Branch/tag: try specific branch first, fallback to default
            git clone --quiet "$GITHUB_URL/$repo_name" --branch "$ref" "${clone_args[@]}" 2> /dev/null || \
            git clone --quiet "$GITHUB_URL/$repo_name" "${clone_args[@]}"
        fi

        # Post-clone operations
        post_clone_operations "$dir_name" "$ref" "$recursive"
    else
        # Default branch: shallow clone
        git clone --quiet "$GITHUB_URL/$repo_name" --depth=1 "${clone_args[@]}"
    fi
}

if [ -z "$PROJECT_NAME" ]; then
    fatal "Project name is required"
fi

if [ -z "$MANIFEST_FILE" ]; then
    fatal "Manifest file is required"
fi

if [ ! -f "$MANIFEST_FILE" ]; then
    fatal "Manifest file '$MANIFEST_FILE' not found"
fi

# Read and process each line from the manifest file
while IFS= read -r repo || [ -n "$repo" ]; do
    # Skip empty lines and comments
    [[ -z "$repo" || "$repo" =~ ^[[:space:]]*# ]] && continue

    # Remove any leading/trailing whitespace
    repo=$(echo "$repo" | xargs)

    # Skip lines starting with 'cloning' (for backward compatibility)
    [[ "$repo" =~ ^cloning[[:space:]] ]] && repo=${repo#cloning }

    if [ -n "$repo" ]; then
        clone_repo "$repo"
    fi
done < "$MANIFEST_FILE"

# Wait for all background processes to complete
wait
