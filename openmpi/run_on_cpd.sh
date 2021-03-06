#!/bin/sh
# Run this first for reasons

user="$1"
testfile="$2"
profile="$3"
passAll="$4"

home="/afs/.ist.utl.pt/users/4/8/$user/"
if [[ $passAll == *[!\ ]* ]]; then
  echo "Passing main.c"
  sshpass -f clusterpass scp ./mpi/main.c \
    "$user"@cluster.rnl.tecnico.ulisboa.pt:"$home"main.c

  sshpass -f clusterpass ssh "$user"@cluster.rnl.tecnico.ulisboa.pt -t \
    "./sshpass -f cpdpass scp "$home"main.c cpd06@cpd-6:~/main.c"

  echo "Passing profile"
  sshpass -f clusterpass scp "$profile" \
    "$user"@cluster.rnl.tecnico.ulisboa.pt:"$home"my-nodes.txt

  sshpass -f clusterpass ssh "$user"@cluster.rnl.tecnico.ulisboa.pt -t \
    "./sshpass -f ./cpdpass scp "$home"my-nodes.txt cpd06@cpd-6:~/my-nodes.txt"

  echo "Compiling Kuduro"
  sshpass -f clusterpass ssh "$user"@cluster.rnl.tecnico.ulisboa.pt -t \
    "./sshpass -f cpdpass ssh cpd06@cpd-6 -t " \
    "mpicc -std=gnu11 -O2 -Os -ffast-math -o kuduro-mpi main.c"
fi

echo "Executing Kuduro"
sshpass -f clusterpass ssh "$user"@cluster.rnl.tecnico.ulisboa.pt -t \
  "./sshpass -f cpdpass ssh cpd06@cpd-6 -t " \
  "time mpirun --hostfile ./my-nodes.txt kuduro-mpi $testfile"
