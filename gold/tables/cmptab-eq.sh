# shell file to run table comparisons between tables whose metrics are expected to be equal
# results written to: build/regression/output

# cmp_tab.py usage: 

#        python cmp_tab.py infile1 infile2 outfile -e eps1 eps2 ... -c exp1 exp2 ...
#        assumes 4 metrics, to specify use -m x where x = # of metrics 

echo ${13}

#crafty
python ${13}/gold/exe/cmp_tab.py $1 $2 $1:$2 -e 0.05 0.11 0.21 0.05 -c eq eq eq eq

python ${13}/gold/exe/cmp_tab.py $3 $4 $3:$4 -e 0.05 0.11 0.21 0.05 -c eq eq eq eq

#mcf
python ${13}/gold/exe/cmp_tab.py $5 $6 $5:$6 -e 0.05 0.11 0.21 0.05 -c eq eq eq eq

python ${13}/gold/exe/cmp_tab.py $7 $8 $7:$8 -e 0.05 0.11 0.21 0.05 -c eq eq eq eq

#vpr
python ${13}/gold/exe/cmp_tab.py $9 ${10} $9:${10} -e 0.05 0.11 0.21 0.05 -c eq eq eq eq

python ${13}/gold/exe/cmp_tab.py ${11} ${12} ${11}:${12} -e 0.05 0.11 0.21 0.05 -c eq eq eq eq

