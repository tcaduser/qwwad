#! /bin/sh
set -e

# Calculates the carrier-carrier scattering as function of initial electron
# energy for a QW with & without screening.
#
# This script is part of the QWWAD software suite. Any use of this code
# or its derivatives in published work must be accompanied by a citation
# of:
#   P. Harrison and A. Valavanis, Quantum Wells, Wires and Dots, 4th ed.
#    Chichester, U.K.: J. Wiley, 2015, ch.2
#
# (c) Copyright 1996-2014
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

# Calculation of the e-e scattering rate over two subband populations
# with and without screening
# Define temperature
T=77

# Initialise files
outfile1=cc-screening-N1.dat
outfile10=cc-screening-N10.dat
rm -f $outfile

# Scattering rate codes (at the moment) are angled towards numerical
# solutions and require a potential barrier height as an upper limit
# for integration, so define an artificial structure
# making sure it contains the same number of points as below
cat > s.r << EOF
100 1.0 0.0
400 0.0 0.0
100 1.0 0.0
EOF

# Now convert structure into potential data
# 1 point-per-angstrom
find_heterostructure --nz-1per 301
efxv

# Define width of infinite well 
LW=400

# Generate infinitely deep well solutions
efiw --width $LW --nz 301 --nst 2

# Define subband populations in file `N.r'
N=1
cat > N.r << EOF
1 $N
2 $N
EOF

# Calculate the distribution functions
sbp --Te $T
 
# Define required e-e rate
echo 2 2 1 1 > rr.r

# Calculate e-e rate WITH screening
srcc --temperature $T

# Save data
cat cc2211.r > $outfile1
 
# Now calculate WITHOUT screening
srcc --temperature $T --noscreening

printf "\n"  >> $outfile1
cat cc2211.r >> $outfile1

# Repeat for N=10e10 cm^{-2}
N=10

# Define subband populations in file `N.r'
cat > N.r << EOF
1 $N
2 $N
EOF

# Calculate the distribution functions
sbp --Te $T
 
# Define required e-e rate
echo 2 2 1 1 > rr.r

# Calculate e-e rate WITH screening
srcc --temperature $T

# Save data
cat cc2211.r > $outfile10
 
# Now calculate WITHOUT screening
srcc --temperature $T --noscreening

printf "\n"  >> $outfile10
cat cc2211.r >> $outfile10

cat << EOF
Results have been written to $outfile1 and ${outfile10}.
$outfile1 contains results for a sheet doping density of 1 x 10^{10} cm^{-2}
in each subband.
$outfile10 contains results for a sheet doping density of 10 x 10^{10} cm^{-2}
in each subband.

Each file is in the format:

  COLUMN 1 - Total initial carrier energy [meV]
  COLUMN 2 - |2,2> -> |1,1> scattering rate [s^{-1}]

Each file contains 2 data sets:

  SET 1 - Screening included
  SET 2 - Screening not included

This script is part of the QWWAD software suite.

(c) Copyright 1996-2015
    Alex Valavanis <a.valavanis@leeds.ac.uk>
    Paul Harrison  <p.harrison@leeds.ac.uk>

Report bugs to https://bugs.launchpad.net/qwwad
EOF

# Clean up workspace
rm -f *.r
