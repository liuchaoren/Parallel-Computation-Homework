#!/bin/bash

remoteHost=gitlab.oit.duke.edu
remoteUser=git
# replace the following line with your group number
Groups=4
remoteRepos=parprog.git
localCodeDir=tmprepo

for gitRepo in $remoteRepos
do

  for Group in $Groups
  do
	fullGroupName=parprog_f15_group$Group
    echo -e "Group = $Group gitRepo = $gitRepo localCodDir = $localCodeDir"
    localRepoDir=$(echo ${localCodeDir}/$fullGroupName/${gitRepo}|cut -d'.' -f1)
	echo -e "LocalRepoDir = $localRepoDir"
    if [ -d $localRepoDir ]; then 	
		echo -e "Directory $localRepoDir already exits, please remove it..."
	else
		cloneCmd="git clone $remoteUser@$remoteHost:$fullGroupName/"
		cloneCmd=$cloneCmd"$gitRepo $localRepoDir"
		
		echo -e "$cloneCmd"
	#	cloneCmdRun=$($cloneCmd 2>&1)

		cloneCmdRun=$($cloneCmd)
		echo -e "Running: \n$ $cloneCmd"
		echo -e "${cloneCmdRun}"
		pushd $PWD/$localRepoDir/labs/collective
		make
		echo -e "Run Serial Code"
		(ulimit -t 15; $PWD/crypto_serial US.dic keys/*)
		echo -e "Run openmp Code"
		(ulimit -t 300; $PWD/crypto_openmp US.dic keys/*)
		echo -e "Run cilk Code"
		(ulimit -t 300; $PWD/crypto_cilk US.dic keys/*)
		echo -e "Run tbb Code"
		(ulimit -t 300; $PWD/crypto_tbb US.dic keys/*)
		popd
    fi
  done
done
