for infofile in `ls *.info`
do
  isthere=`grep "SM_20" ${infofile} | wc -l`
  if [ ${isthere} -gt 0 ]; then
    echo "Kernel register details are already available for ${infofile}"
  else
    cp $infofile tmp.out

    for kernels in `cat ${infofile} | grep "Name=" | cut -d"=" -f2`
    do
      outstring=""
      for arch in sm_20 sm_10
      do
        archupper=`echo $arch | tr a-z A-Z`
        cat kernelstats | grep -A1 -i ${kernels} | grep -A1 ${arch} > tmp

        for data in `cat tmp | grep -i "Used" | cut -d ":" -f2 | sed 's/Used//g;s/bytes//g' |sed 's/,/\n/g'`
        do
          case $data in
            registers)
              outstring=$outstring`echo "\n${archupper}_GMEM=$(($olddata))"`
              ;;
            smem)
              outstring=$outstring`echo "\n${archupper}_SMEM=$(($olddata))"`
              ;;
            cmem*)
              upper=`echo $data | tr a-z A-Z`
              outstring=$outstring`echo "\n${archupper}_${upper}=$(($olddata))"`
              ;;
            *)
              olddata=$data
              ;;
          esac
          #echo $data | sed 's/" "/\n/g'
        done

      done
      #echo -e $outstring

      cat tmp.out | sed "s/Name=${kernels}/Name=${kernels}${outstring}/" > tmp.in
      mv tmp.in tmp.out

    done
    mv tmp.out $infofile

  fi 
done

