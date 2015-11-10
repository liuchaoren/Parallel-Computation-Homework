#! /bin/sh

rm -f matmul_mpi$1
make matmul_mpi$1

scp matmul_mpi$1 ml311@phe.cs.duke.edu:~/parprog/labs/data_reorg
scp matmul_mpi$1 ml311@phi.cs.duke.edu:~/parprog/labs/data_reorg
scp matmul_mpi$1 ml311@phum.cs.duke.edu:~/parprog/labs/data_reorg

mpirun -np 4 -ppn 1 ./matmul_mpi$1
