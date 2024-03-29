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
		pushd $PWD/$localRepoDir/labs/map
		make
		echo -e "$Group Run Serial Code"
		(ulimit -t 300; $PWD/pass_serial `cat keys/99999999`)
		ulimit -t
		echo -e "$Group Run openmp Code"
		(ulimit -t 10000; $PWD/pass_openmp `cat keys/99999999` 1 3)
		echo -e "$Group Run cilk Code"
		(ulimit -t 10000; $PWD/pass_cilk `cat keys/99999999` 1 2)
		echo -e "$Group Run tbb Code"
		(ulimit -t 10000; $PWD/pass_tbb `cat keys/99999999` 1 2)
		popd
    fi
  done
done
