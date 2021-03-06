/**
 * \file   qwwad_density_of_states.cpp
 * \brief  Density of states calculator
 * \author Paul Harrison  <p.harrison@shu.ac.uk>
 * \author Alex Valavanis <a.valavanis@leeds.ac.uk>
 *
 * \details This program calculates the density of states for bulk (3D),
 *          quantum wells (2D) and quantum wires (1D), for a series of subband
 *          minima which are read in from the external file `Ee.r', or `Eh.r'
 */

#include <iostream>
#include "qwwad/dos-functions.h"
#include "qwwad/constants.h"
#include "qwwad/file-io.h"
#include "qwwad/file-io-deprecated.h"
#include "qwwad/options.h"
#include <cstdlib>
#include <valarray>
#include <fstream>

using namespace QWWAD;
using namespace constants;

/**
 * Configure command-line options for the program
 */
Options configure_options(int argc, char* argv[])
{
    Options opt;

    std::string summary("Find density of states for bulk (3D), quantum wells (2D) and quantum wires (1D).");

    opt.add_option<double>      ("mass,m",     0.067, "Effective mass (relative to free electron).");
    opt.add_option<double>      ("vcb",        0.00,  "Band-edge potential [eV]");
    opt.add_option<double>      ("alpha",      0.00,  "Non-parabolicity parameter [eV^{-1}]");
    opt.add_option<char>        ("particle,p", 'e',   "ID of particle to be used: 'e', 'h' or 'l', for electrons, heavy holes or light holes respectively.");
    opt.add_option<unsigned int>("ndim",         2,   "Dimensionality of the system (1, 2 or 3)");

    opt.add_prog_specific_options_and_parse(argc, argv, summary);

    return opt;
};

int main(int argc,char *argv[])
{
    const auto opt = configure_options(argc, argv);

    const auto p     = opt.get_option<char>        ("particle");  // particle ID (e, h or l)
    const auto m     = opt.get_option<double>      ("mass") * me; // effective mass [kg]
    const auto V     = opt.get_option<double>      ("vcb") * e;   // band_edge potential [J]
    const auto alpha = opt.get_option<double>      ("alpha") / e; // Non-parabolicity [1/J]
    const auto ndim  = opt.get_option<unsigned int>("ndim");      // Get number of dimensions

    const size_t n=1000; // Number of output energies

    // Read in the subband energies if we're solving for a quantised system
    std::valarray<double> E;

    if(ndim == 1 || ndim == 2)
        E = read_E(p); // read in subband minima [J]

    std::valarray<double> energy(n+1); // Energies at which dos is calculated [J]
    std::valarray<double> dos(n+1);    // Density of states [J^{-1}m^{-n}]

    for(unsigned int ie=0;ie<=n;ie++)
    {
        energy[ie] = ie*1e-3*e; // convert meV-> J

        switch(ndim)
        {
            case 1:
                dos[ie] = calculate_dos_1D(m, energy[ie], E);
                break;
            case 2:
                dos[ie] = calculate_dos_2D(m, energy[ie], E, V, alpha);
                break;
            case 3:
                dos[ie] = calculate_dos_3D(m, energy[ie], V, alpha);
                break;
            default:
                std::cerr << "Only 1, 2 or 3 dimensions are permitted" << std::endl;
                exit(EXIT_FAILURE);
        }
    }

    const std::valarray<double> E_out = 1000.0*energy/e;
    write_table("rho.r", E_out, dos);

    return EXIT_SUCCESS;
} /* end main */
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
