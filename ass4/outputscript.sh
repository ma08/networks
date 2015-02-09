echo "Params                                Throughput                         Forwarding                      Delay" > output.txt
rm *.data
traces="$(ls $1)"
for i in ${traces[@]};
do
  python script.py "$1/$i" >> output.txt
done
