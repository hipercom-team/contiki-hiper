
C=32768.0

set grid
set term post eps color
set output "time-diff-sync5.ps"
set title "resync every 5 seconds (part)"
set xlabel "time difference (sec)"
set ylabel "time (sec)"
plot [0:50] [] 'time-sync5.dat' u ($1/C):(($3)/C) w lp, \
               'time-sync5.dat' u ($1/C):(($4)/C) w lp, \
	       'time-sync5.dat' u ($1/C):(($5)/C) w lp


set term post eps color
set output "time-diff-sync25.ps"
set title "resync every 25 seconds (part)"
set xlabel "time difference (sec)"
set ylabel "time (sec)"
plot [0:250] [] 'time-sync25.dat' u ($1/C):(($3)/C) w lp, \
                'time-sync25.dat' u ($1/C):(($4)/C) w lp, \
                'time-sync25.dat' u ($1/C):(($5)/C) w lp


set term post eps color
set output "time-diff-sync100.ps"
set title "resync every 100 seconds (part)"
set xlabel "time difference (sec)"
set ylabel "time (sec)"
plot [0:1000] [] 'time-sync100.dat' u ($1/C):(($3)/C) w lp, \
                 'time-sync100.dat' u ($1/C):(($4)/C) w lp, \
                 'time-sync100.dat' u ($1/C):(($5)/C) w lp
