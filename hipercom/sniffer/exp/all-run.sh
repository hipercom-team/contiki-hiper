#! /bin/sh

python processPacketLog.py
python runClockAdjust.py
gnuplot misc-time.plot

for i in *.ps ; do pstopdf $i ; done
