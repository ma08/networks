echo "Params  tFirstTPacket tLastTPacket tFirstRPacket tLastRPacket delaySum tBytes rBytes lostPackets timesForwarded pcktsDropped bytesDropped tThroughput rThroughput\n" > output.txt
traces="$(ls traces/*.tr)"
for i in ${traces[@]};
do
  python script.py $i >> output.txt
done

