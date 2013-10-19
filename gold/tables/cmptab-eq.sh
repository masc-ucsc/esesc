#crafty
echo ${13}
${13}/gold/exe/cmptab -t $1:$2 -f0 $1 -f1 $2 -o $1:$2 -m 6 -e 0.05 0.11 0.21 0.05 0.05 0.0 -c eq eq eq eq eq eq

${13}/gold/exe/cmptab -t $3:$4 -f0 $3 -f1 $4 -o $3:$4 -m 6 -e 0.05 0.11 0.21 0.05 0.05 0.0 -c eq eq eq eq eq eq

#mcf
${13}/gold/exe/cmptab -t $5:$6 -f0 $5 -f1 $6 -o $5:$6 -m 6 -e 0.05 0.11 3.80 0.05 0.05 0.0 -c eq eq eq eq eq eq

${13}/gold/exe/cmptab -t $7:$8 -f0 $7 -f1 $8 -o $7:$8 -m 6 -e 0.05 0.11 3.80 0.05 0.05 0.0 -c eq eq eq eq eq eq

#vpr
${13}/gold/exe/cmptab -t $9:${10} -f0 $9 -f1 ${10} -o $9:${10} -m 6 -e 0.05 0.11 0.50 0.05 0.05 0.0 -c eq eq eq eq eq eq

${13}/gold/exe/cmptab -t ${11}:${12} -f0 ${11} -f1 ${12} -o ${11}:${12} -m 6 -e 0.05 0.11 0.50 0.05 0.05 0.0 -c eq eq eq eq eq eq

