#!/bin/bash

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'

function confirm {
	# call with a prompt string or use a default
	read -r -p "${1:-Do you wish to continue? [y/N]} " response
	case $response in
		[yY][eE][sS]|[yY]) 
			true
			;;
		*)
			false
			;;
	esac
}

printf "This script will adjust the local configuration of this git repository and proceed to hard reset the working copy afterwards. This script should be ran immediately after initial git clone\n\n"
if ! confirm; then 
	exit 0
fi

if [ $# -eq 2 ]; then
	regexp='^[A-Za-z0-9_ ]+$'
	if [[ $1 =~ $regexp ]]; then
		printf "${GREEN}Settings user.name to \"$1\"\n${NC}"
		git config --local user.name "$1"
	else
		printf "${RED}Invalid username \"$2\" provided.\n${NC}"
	fi

	regexp='^[a-z0-9_@\.]+$'
	if [[ $2 =~ $regexp ]]; then
		printf "${GREEN}Settings user.email to \"$2\"\n${NC}"
		git config --local user.email "$2"
	else
		printf "${RED}Invalid email \"$2\" provided.\n${NC}"
	fi
fi

git config --local core.autocrlf false
git config --local core.ignorecase false
printf "${GREEN}Settings core.autocrlf and core.ignorecase set to false...\n${NC}"

git config --local filter.utf16win.clean "iconv -sc -f utf-16le -t utf-8"
git config --local filter.utf16win.smudge "iconv -sc -f utf-8 -t utf-16le"
git config --local filter.utf16win.required true
printf "${GREEN}Filter 'utf16win' defined...\n${NC}"

printf "${GREEN}Beginning working copy reset...\n${NC}"

# get rid of utf-8 encoded files checked out before the config was changed
rm -f ./StrongDC.rc
rm -f ./windows/Resource.h

# this will delete all working copy files, excluding this script and preform a hard reset on the repository
shopt -s extglob dotglob
git clean -dfx -e compiled
git rm -rf !(.|..|.git|compiled|gitconfig.sh) > /dev/null
git reset --hard

printf "${GREEN}Working copy reset complete!\n${NC}"
