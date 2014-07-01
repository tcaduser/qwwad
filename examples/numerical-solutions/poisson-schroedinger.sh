#!/bin/sh
set -e

# Define output file
outfile=poisson-schroedinger-E-I.dat

# Initialise files

rm -f $outfile wf_e1*.r

# Define well width and doping density
LW=100
D=-1		# Note -ve implies `n-type' doping

# First generate structure definition `s.r' file
echo 200 0.2 0.0  > s.r
echo $LW 0.0 2e18  >> s.r
echo 200 0.2 0.0 >> s.r
 
find_heterostructure	# generate alloy concentration as a function of z
efxv			# generate potential data

cp v.r vcb.r # Save conduction-band energy
  
for I in `seq 0 7`; do
 # Calculate ground state energy
 efss --nst-max 1

 densityinput # Generate an estimate of the population density
 chargedensity # Compute charge density profile
 
 # save wave function is separate file
 cp wf_e1.r wf_e1-I=$I.r

 # Write energy to output file
 E1=`awk '{printf("\t%20.17e\n",$2)}' Ee.r`

 echo $I $E1 >> $outfile

 # Implement self consistent Poisson calculation
 find_poisson_potential
 paste vcb.r v_p.r | awk '{print $1, $2+$4}' > v.r
# scps
done # X