#! /bin/sh

rm -f matmul$1_mpi
make matmul$1_mpi

scp matmul$1_mpi $(logname)@phe.cs.duke.edu:$PWD
scp matmul$1_mpi $(logname)@phi.cs.duke.edu:$PWD
scp matmul$1_mpi $(logname)@phum.cs.duke.edu:$PWD

mpirun -np 4 -ppn 1 ./matmul$1_mpi
