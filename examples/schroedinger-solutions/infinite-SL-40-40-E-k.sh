#!/bin/sh
set -e

# Calculates the lowest energy solution in Kronig-Penney superlattice
# with a range of electron momenta
#
# This script is part of the QWWAD software suite. Any use of this code
# or its derivatives in published work must be accompanied by a citation
# of:
#   P. Harrison and A. Valavanis, Quantum Wells, Wires and Dots, 4th ed.
#    Chichester, U.K.: J. Wiley, 2015, ch.2
#
# (c) Copyright 1996-2015
#     Paul Harrison <p.harrison@shu.ac.uk>
#     Alex Valavanis <a.valavanis@leeds.ac.uk>
#
# QWWAD is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# QWWAD is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QWWAD.  If not, see <http://www.gnu.org/licenses/>.

# Initialise files
outfile=infinite-SL-40-40-E-k.dat
rm -f $outfile

# Specify a fixed well and barrier width
export QWWAD_WELLWIDTH=40
export QWWAD_BARRIERWIDTH=40

# Calculate conduction band barrier height for GaAs/Ga(1-x)Al(x)As
# Use V=0.67*1247*x, keep x=0.4
export QWWAD_BARRIERPOTENTIAL=334.1965

# Calculate bulk effective mass of electron in Ga(1-x)Al(x)As
# Use MB=0.067+0.083*x
export QWWAD_BARRIERMASS=0.1002

# Only compute one state
export QWWAD_NST=1

# Loop over carrier wave-vector
for K in `seq -2.0 0.01 2.0`; do
    printf "%e\t" $K >> $outfile	# write wave vector to file

    # Calculate energies for different wave vectors
    qwwad_ef_superlattice --wavevector $K
    awk '{printf("%9.3f\n",$2)}' Ee.r >> $outfile		# send data to file
done

cat << EOF
Results have been written to $outfile in the format:

  COLUMN 1 - Wave-vector along growth axis [rel. to pi/L]
  COLUMN 2 - Energy of lowest state [meV]

This script is part of the QWWAD software suite.

(c) Copyright 1996-2015
    Alex Valavanis <a.valavanis@leeds.ac.uk>
    Paul Harrison  <p.harrison@leeds.ac.uk>

Report bugs to https://bugs.launchpad.net/qwwad
EOF

rm Ee.r wf_e1.r
