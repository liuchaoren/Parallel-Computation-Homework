#! /bin/sh

make matmul_mpi

scp matmul_mpi ml311@phe.cs.duke.edu:~/parprog/labs/data_reorg
scp matmul_mpi ml311@phi.cs.duke.edu:~/parprog/labs/data_reorg
scp matmul_mpi ml311@phum.cs.duke.edu:~/parprog/labs/data_reorg

mpirun -np 4 -ppn 1 ./matmul_mpi
