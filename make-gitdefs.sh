#!/bin/bash

file='./client/gitdefs.inc'

if [ ! -d './.git/' ]; then
    echo "#define NO_VERSIONCONTROL 1" >> '$file.tmp'

    echo "Source tree not under version control, creating dummy gitdefs file..."
    rm -f $file
    mv '$file.tmp' $file

    exit 0
fi

function shorten_tag {
    local IFS='-'
    local tag_array=
    read -a tag_array <<< "$1"

    unset tag_array[2] tag_array[3]
    echo "${tag_array[*]}"
}

GIT_TAG_SHORT=`git describe --abbrev=4`
GIT_TAG_SHORT=$(shorten_tag $GIT_TAG_SHORT)

GIT_TAG_LONG=`git describe --abbrev=4 --dirty=-d`
GIT_REF_COMMIT=`git rev-parse HEAD`
GIT_COMMIT_COUNT=`git rev-list HEAD --count`
GIT_VERSION_DATE=`git log -1 --format=%at`

GIT_COMPATIBLE_VERSIONID=`git rev-list 1.6.0..HEAD --count`
GIT_COMPATIBLE_VERSIONID=`expr 2165 + $GIT_COMPATIBLE_VERSIONID`

echo "#define GIT_TAG_SHORT \"$GIT_TAG_SHORT\"" > '$file.tmp'
echo "#define GIT_TAG_LONG \"$GIT_TAG_LONG\"" >> '$file.tmp'
echo "#define GIT_REF_COMMIT \"$GIT_REF_COMMIT\"" >> '$file.tmp'
echo "#define GIT_COMMIT_COUNT $GIT_COMMIT_COUNT" >> '$file.tmp'
echo "#define VERSION_DATE $GIT_VERSION_DATE" >> '$file.tmp'
echo "#define COMPATIBLE_VERSIONID $GIT_COMPATIBLE_VERSIONID" >> '$file.tmp'

touch $file
diff --brief '$file.tmp' $file > /dev/null
has_changes=$?

if [ $has_changes -eq 1 ]
then
    echo "Version changes detected updating the gitdefs file..."
    rm -f $file
    mv '$file.tmp' $file
else
    echo "No Version changes detected, using the old gitdefs file..."
    rm -f '$file.tmp'
fi
