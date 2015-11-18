# $1=esesc directory
path=$1/conf

#Check if the directories exist
if [ ! -d "./regression" ]; then
  echo "Created directory ./regression"
  mkdir ./regression
fi

if [ ! -d "regression/run" ]; then
  echo "Created directory ./regression/run"
  mkdir ./regression/run
fi

if [ ! -d "regression/output" ]; then
  echo "Created directory ./regression/output"
  mkdir ./regression/output
fi

if [ ! -d "regression/report" ]; then
  echo "Created directory ./regression/report"
  mkdir ./regression/report
fi

if [ ! -d "regression/tables" ]; then
  echo "Created directory ./regression/tables"
  mkdir ./regression/tables
fi

#Copy over the files from conf to where the benchmark scripts are
cp $1/conf/* ./regression/run > /dev/null 2>&1

#Run mt-scripts to generate benchmark testing scripts
cd ./regression/run
./mt-scripts.rb -b single_core -e default_user -m TASS \
              -c esesc.conf \
              -x ../../main/esesc -l -j $path

