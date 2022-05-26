# gnuplot.sh
# gnupload load "plot.gp"


unset logscale; set logscale y;
plot  "flip_delaunay.txt" using 1:2 title 'delaunay' with lines,\
      "flip_max.txt" using 1:2 title 'max' with lines, \
      "flip_set_maxangle.txt" using 1:2 title 'set angle' with lines, \
      "flip_set_maxenergy.txt" using 1:2 title 'set energy' with lines
