algo=$1
trace=$2

./bin/$1Sim traces/$2.trace | tee logs/$1_$2.txt
