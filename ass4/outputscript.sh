echo "Params                                Throughput                         Forwarding                      Delay" > output.txt
traces="$(ls traces)"
for i in ${traces[@]};
do
  python script.py "traces/$i" >> output.txt
done
