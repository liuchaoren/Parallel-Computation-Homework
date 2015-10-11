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
		pushd $PWD/$localRepoDir/labs/forkjoin
		make
		echo -e "Run Serial Code"
		(ulimit -t 300; $PWD/starhole_serial 10000 100 200 100 0 -100 0 0 100 0 -100)
		echo -e "Run openmp Code"
		(ulimit -t 300; $PWD/starhole_openmp 10000 100 200 100 0 -100 0 0 100 0 -100)
		echo -e "Run Cilk Code"
		(ulimit -t 300; $PWD/starhole_cilk 10000 100 200 100 0 -100 0 0 100 0 -100)
		echo -e "Run TBB Code"
		(ulimit -t 300; $PWD/starhole_tbb 10000 100 200 100 0 -100 0 0 100 0 -100)
		popd
    fi
  done
done
