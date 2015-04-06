# shell file to compare tables whose metrics are expected to be unequal 

# cmp_tab.py usage:
#           python cmp_tab.py infile1 infile2 outfile -e eps1 eps2 ... -c exp1 exp2 ...
#           Assumes 4 metrics. Use argument -m x, where x is number of metrics in the table          

#crafty
python $7/gold/exe/cmp_tab.py $1 $2 $1:$2 -e 0.05 0.11 0.21 0.05 -c gt gt eq lt

#mcf
python $7/gold/exe/cmp_tab.py $3 $4 $3:$4 -e 0.05 0.11 0.21 0.05 -c gt gt eq lt

#vpr
python $7/gold/exe/cmp_tab.py $5 $6 $5:$6 -e 0.05 0.11 0.21 0.05 -c gt gt eq lt
