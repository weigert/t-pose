# gnuplot.sh
# gnupload load "plot.gp"


unset logscale; set logscale y; set logscale y;
plot  "flip_delaunay.txt" using 1:2 title 'delaunay' with lines,\
      "flip_max.txt" using 1:2 title 'max' with lines, \
      "flip_set_maxangle.txt" using 1:2 title 'set angle' with lines, \
      "flip_set_maxenergy.txt" using 1:2 title 'set energy' with lines, \
      "flip_set_maxenergy_dp_2.txt" using 1:2 title 'set energy, decrease dp linear 2' with lines, \
      "flip_set_maxenergy_dp_3.txt" using 1:2 title 'set energy, decrease dp linear 3' with lines, \
      "flip_set_maxenergy_dp_10.txt" using 1:2 title 'set energy, decrease dp linear 10' with lines, \
      "flip_set_maxenergy_dp_20.txt" using 1:2 title 'set energy, decrease dp linear 20' with lines, \
      "flip_set_maxenergy_dp_50.txt" using 1:2 title 'set energy, decrease dp linear 50' with lines, \
      "flip_delaunay_dp_2.txt" using 1:2 title 'delaunay, decrease dp linear 2' with lines
