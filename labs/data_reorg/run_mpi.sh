#! /bin/sh

rm -f matmul$2_mpi
make matmul$2_mpi

scp matmul$2_mpi $1@phe.cs.duke.edu:~/parprog/labs/data_reorg
scp matmul$2_mpi $1@phi.cs.duke.edu:~/parprog/labs/data_reorg
scp matmul$2_mpi $1@phum.cs.duke.edu:~/parprog/labs/data_reorg

mpirun -np 4 -ppn 1 ./matmul$2_mpi
