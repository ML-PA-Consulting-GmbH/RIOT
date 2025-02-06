#!/bin/bash

# This script lists all the packages that are used for the build of an
# application image. It is expected to be run inside the application directory.

set -e

read -r -d '' AWK_CUT_SBOM_PKG_INFO <<"EOF" || :
BEGIN{o=0};
{
  if ($0 == "<<<< END SBOM INPUT") {
    o = 0;
  };
  if (o==1) {
    print $0;
  };
  if ($0 == ">>>> BEGIN SBOM INPUT") {
    o = 1;
  }
}
EOF

echo "+++ Producing package info for the application"
SBOM_PKG_INFO=$(make info-sbom 2>/dev/null | awk "${AWK_CUT_SBOM_PKG_INFO}")

STRACE_LOG=$(mktemp)
STRACE_FILE_RESULTS=$(mktemp)
SBOM_INPUT=$(mktemp)

function cleanup {
    [[ -e ${STRACE_LOG} ]] && rm -f ${STRACE_LOG} || :
    [[ -e ${STRACE_FILE_RESULTS} ]] && rm -f ${STRACE_FILE_RESULTS} || :
    [[ -e ${SBOM_INPUT} ]] && rm -f ${SBOM_INPUT} || :
}

trap cleanup EXIT

echo "+++ Running build with strace to get the list of files used for the build"
make -j clean
strace -f -e trace=openat,open -e quiet=attach,exit,path-resolution,personality,thread-execve,superseded -o ${STRACE_LOG} make -j

function get_files_from_strace {
    echo "{ \"files\": ["
    num_files=0
    for fil in $(cat /tmp/strace.log \
                 | grep -E '(open\(|openat\()' \
                 | sed -E 's/[^"]*"(.*)"[^"]*/\1/' \
                 | grep -iE '.*\.(h|hpp|c|cpp|hxx|cxx|c++|s|asm)$' \
                 | sort -u
                )
    do
        [[ ! -e $fil ]] && continue
        [[ ${num_files} -gt 0 ]] && echo -n "," || :
        echo "\"${fil}\""
        num_files=$(( ${num_files} + 1 ))
    done
    echo "]}"
}

get_files_from_strace ${STRACE_LOG} > ${STRACE_FILE_RESULTS}

# merge pkg info and strace results with jq into one json object
jq -s '.[0] * .[1]' <(echo ${SBOM_PKG_INFO}) ${STRACE_FILE_RESULTS} > ${SBOM_INPUT}
