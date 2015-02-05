/**
 * \file   srelo.cpp
 * \brief  Scattering Rate Electron-LO phonon
 * \author Paul Harrison  <p.harrison@shu.ac.uk>
 * \author Alex Valavanis <a.valavanis@leeds.ac.uk>
 */

#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <gsl/gsl_math.h>
#include "qclsim-constants.h"
#include "qclsim-fileio.h"
#include "qclsim-subband.h"
#include "qwwad-options.h"

using namespace Leeds;
using namespace constants;

static void ff_table(const double   dKz,
                     const Subband &isb,
                     const Subband &fsb,
                     unsigned int   nKz,
                     std::valarray<double> &Kz,
                     std::valarray<double> &Gifsqr);

void ff_output(const std::valarray<double> &Kz,
               const std::valarray<double> &Gifsqr,
               unsigned int        i,
               unsigned int        f);

static double Gsqr(const double   Kz,
                   const Subband &isb,
                   const Subband &fsb);

static Options configure_options(int argc, char* argv[])
{
    Options opt;

    std::string doc("Find the polar LO-phonon scattering rate.");

    opt.add_switch        ("outputff,a",            "Output form-factors to file.");
    opt.add_switch        ("noblocking,S",          "Disable final-state blocking.");
    opt.add_numeric_option("latticeconst,A",  5.65, "Lattice constant in growth direction [angstrom]");
    opt.add_numeric_option("ELO,E",          36.0,  "Energy of LO phonon [meV]");
    opt.add_numeric_option("epss,e",         13.18, "Static dielectric constant");
    opt.add_numeric_option("epsinf,f",       10.89, "High-frequency dielectric constant");
    opt.add_numeric_option("mass,m",         0.067, "Band-edge effective mass (relative to free electron)");
    opt.add_char_option   ("particle,p",       'e', "ID of particle to be used: 'e', 'h' or 'l', for "
                                                    "electrons, heavy holes or light holes respectively.");
    opt.add_numeric_option("Te",               300, "Carrier temperature [K].");
    opt.add_numeric_option("Tl",               300, "Lattice temperature [K].");
    opt.add_numeric_option("Ecutoff",               "Cut-off energy for carrier distribution [meV]. If not specified, then 5kT above band-edge.");
    opt.add_size_option   ("nki",             1001, "Number of initial wave-vector samples.");
    opt.add_size_option   ("nKz",             1001, "Number of phonon wave-vector samples.");

    opt.add_prog_specific_options_and_parse(argc, argv, doc);

    return opt;
}

int main(int argc,char *argv[])
{
const Options opt = configure_options(argc, argv);

const bool   ff_flag     = opt.get_switch("outputff");                     // True if formfactors are wanted
const double A0          = opt.get_numeric_option("latticeconst") * 1e-10; // Lattice constant [m]
const double Ephonon     = opt.get_numeric_option("ELO") * e/1000;         // Phonon energy [J]
const double epsilon_s   = opt.get_numeric_option("epss")   * eps0;        // Static permittivity [F/m]
const double epsilon_inf = opt.get_numeric_option("epsinf") * eps0;        // High-frequency permittivity [F/m]
const double m           = opt.get_numeric_option("mass")*me;              // Band-edge effective mass [kg]
const char   p           = opt.get_char_option("particle");  	           // Particle ID
const double Te          = opt.get_numeric_option("Te");                   // Carrier temperature [K]
const double Tl          = opt.get_numeric_option("Tl");                   // Lattice temperature [K]
const bool   b_flag      = !opt.get_switch("noblocking");                  // Include final-state blocking by default
const size_t nki         = opt.get_size_option("nki");                     // number of ki calculations
const size_t nKz         = opt.get_size_option("nKz");                     // number of Kz calculations

// calculate step length in phonon wave-vector
const double dKz=2/(A0*nKz); // Taken range of phonon integration as 2/A0

// calculate often used constants
const double omega_0 = Ephonon/hBar; // phonon angular frequency
const double N0      = 1.0/(exp(Ephonon/(kB*Tl))-1.0); // Bose-Einstein factor

// Find pre-factors for scattering rates
const double Upsilon_star_a=pi*e*e*omega_0/epsilon_s*(epsilon_s/epsilon_inf-1)*(N0)
    *2*m/gsl_pow_2(hBar)*2/(8*pi*pi*pi);

const double Upsilon_star_e=pi*e*e*omega_0/epsilon_s*(epsilon_s/epsilon_inf-1)*(N0+1)
    *2*m/gsl_pow_2(hBar)*2/(8*pi*pi*pi);

    std::ostringstream E_filename; // Energy filename string
    E_filename << "E" << p << ".r";
    std::ostringstream wf_prefix;  // Wavefunction filename prefix
    wf_prefix << "wf_" << p;

    // Read data for all subbands from file
    std::vector<Subband> subbands = Subband::read_from_file(E_filename.str(),
            wf_prefix.str(),
            ".r",
            m);

// Read and set carrier distributions within each subband
std::valarray<double>       Ef;      // Fermi energies [J]
std::valarray<double>       N;       // Subband populations [m^{-2}]
std::valarray<unsigned int> indices; // Subband indices (garbage)
read_table("Ef.r", indices, Ef);
Ef *= e/1000.0; // Rescale to J
read_table("N.r", N);	// read populations

for(unsigned int isb = 0; isb < subbands.size(); ++isb)
    subbands[isb].set_distribution(Ef[isb], N[isb]);

// Read list of wanted transitions
std::valarray<unsigned int> i_indices;
std::valarray<unsigned int> f_indices;

read_table("rrp.r", i_indices, f_indices);
const size_t ntx = i_indices.size();

std::valarray<double> Wabar(ntx);
std::valarray<double> Webar(ntx);

// Loop over all desired transitions
for(unsigned int itx = 0; itx < i_indices.size(); ++itx)
{
    // State indices for this transition (NB., these are indexed from 1)
    unsigned int i = i_indices[itx];
    unsigned int f = f_indices[itx];

    // Convenience labels for each subband (NB., these are indexed from 0)
    const Subband isb = subbands[i-1];
    const Subband fsb = subbands[f-1];

    // Subband minima
    const double Ei = isb.get_E();
    const double Ef = fsb.get_E();

    double kimax = 0;
    double Ecutoff = 0.0; // Maximum kinetic energy in initial subband

    // Use user-specified value if given
    if(opt.vm.count("Ecutoff"))
    {
        Ecutoff = opt.get_numeric_option("Ecutoff")*e/1000;

        if(Ecutoff+Ei - Ephonon < Ef)
        {
            std::cerr << "No scattering permitted from state " << i << "->" << f << " within the specified cut-off energy." << std::endl;
            std::cerr << "Extending range automatically" << std::endl;
            Ecutoff += Ef;
        }
    }
    // Otherwise, use a fixed, 5kT range
    else
    {
        kimax   = isb.get_k_max(Te);
        Ecutoff = hBar*hBar*kimax*kimax/(2*m);

        if(Ecutoff+Ei < Ef)
            Ecutoff += Ef;
    }

    kimax = isb.k(Ecutoff);

    std::valarray<double> Kz(nKz);
    std::valarray<double> Gifsqr(nKz);

    ff_table(dKz,isb,fsb,nKz,Kz,Gifsqr); // generates formfactor table

    // Output form-factors if desired
    if(ff_flag)
        ff_output(Kz, Gifsqr, i,f);

    /* Generate filename for particular mechanism and open file	*/
    char	filename[9];	/* character string for output filename		*/
    sprintf(filename,"LOa%i%i.r",i,f);	/* absorption	*/
    FILE *FLOa=fopen(filename,"w");			
    sprintf(filename,"LOe%i%i.r",i,f);	/* emission	*/
    FILE *FLOe=fopen(filename,"w");			

 /* calculate Delta variables, constant for each mechanism	*/
 const double Delta_a = Ef - Ei - Ephonon;
 const double Delta_e = Ef - Ei + Ephonon;

 /* calculate maximum value of ki and hence ki step length */
 const double dki=kimax/((float)nki); // Step length for integration over initial wave-vector [1/m]

 std::valarray<double> Waif(nki); // Absorption scattering rate at this wave-vector [1/s]
 std::valarray<double> Weif(nki); // Emission scattering rate at this wave-vector [1/s]

 std::valarray<double> Wabar_integrand_ki(nki); // Average scattering rate [1/s]
 std::valarray<double> Webar_integrand_ki(nki); // Average scattering rate [1/s]

 // calculate e-LO rate for all ki
 for(unsigned int iki=0;iki<nki;iki++)
 {
  const double ki=dki*iki;
  std::valarray<double> Waif_integrand_dKz(nKz); // Integrand for scattering rate
  std::valarray<double> Weif_integrand_dKz(nKz); // Integrand for scattering rate

  // Integral over phonon wavevector Kz
  for(unsigned int iKz=0;iKz<nKz;iKz++)
  {
      const double Kz_2 = Kz[iKz] * Kz[iKz];
      const double Kz_4 = Kz_2 * Kz_2;

      Waif_integrand_dKz[iKz] = Gifsqr[iKz] /
          sqrt(Kz_4 + 2*Kz_2 * (2*ki*ki - 2*m*Delta_a/(hBar*hBar))+
                  gsl_pow_2(2*m*Delta_a/(hBar*hBar)));

      Weif_integrand_dKz[iKz] = Gifsqr[iKz] /
          sqrt(Kz_4 + 2*Kz_2 * (2*ki*ki - 2*m*Delta_e/(hBar*hBar))+
                  gsl_pow_2(2*m*Delta_e/(hBar*hBar))
              );
  } // end integral over Kz

  // Note integral from 0->inf, hence *	2
  Waif[iki] = Upsilon_star_a*pi*integral(Waif_integrand_dKz,dKz);
  Weif[iki] = Upsilon_star_e*pi*integral(Weif_integrand_dKz,dKz);

  /* Now check for energy conservation!, would be faster with a nasty `if'
     statement just after the beginning of the ki loop!			*/
  const double Eki = hBar*hBar*ki*ki/(2*m); // Initial kinetic energy
  Weif[iki] *= Theta(Eki-Delta_e);
  Waif[iki] *= Theta(Eki-Delta_a);

  // Include final-state blocking factor
  if (b_flag)
  {
      // Total final energy
      const double Etf_em = Ei + Eki - Ephonon;
      const double Etf_ab = Ei + Eki + Ephonon;

      // Final kinetic energy
      const double Ef_em = Etf_em - Ef;
      const double Ef_ab = Etf_ab - Ef;

      // Final wave-vector
      const double kf_em = sqrt(Ef_em*2*m)/hBar;
      const double kf_ab = sqrt(Ef_ab*2*m)/hBar;

      Waif[iki] *= (1.0 - fsb.f_FD_k(kf_ab, Te));
      Weif[iki] *= (1.0 - fsb.f_FD_k(kf_em, Te));
  }

  Wabar_integrand_ki[iki] = Waif[iki]*ki*isb.f_FD_k(ki, Te);
  Webar_integrand_ki[iki] = Weif[iki]*ki*isb.f_FD_k(ki, Te);

  /* output scattering rate versus carrier energy=subband minima+in-plane
     kinetic energy						*/
  fprintf(FLOa,"%20.17le %20.17le\n",(Ei + Eki)/(1e-3*e),Waif[iki]);
  fprintf(FLOe,"%20.17le %20.17le\n",(Ei + Eki)/(1e-3*e),Weif[iki]);
 } // End loop over ki

 Wabar[itx] = integral(Wabar_integrand_ki, dki)/(pi*isb.get_pop());
 Webar[itx] = integral(Webar_integrand_ki, dki)/(pi*isb.get_pop());

 fclose(FLOa);	/* close output file for this mechanism	*/
 fclose(FLOe);	/* close output file for this mechanism	*/
} /* end while over states */

write_table("LOa-if.r", i_indices, f_indices, Wabar);
write_table("LOe-if.r", i_indices, f_indices, Webar);

return EXIT_SUCCESS;
}

/**
 * \brief Computes the formfactor at a range of phonon wave-vectors
 */
static void ff_table(const double   dKz,
                     const Subband &isb,
                     const Subband &fsb,
                     unsigned int   nKz,
                     std::valarray<double> &Kz,
                     std::valarray<double> &Gifsqr)
{
    for(unsigned int iKz=0;iKz<nKz;iKz++)
    {
        Kz[iKz]     = iKz*dKz;                 // Magnitude of phonon wave vector
        Gifsqr[iKz] = Gsqr(Kz[iKz], isb, fsb); // Squared form-factor
    }
}

/**
 * \brief calculates the overlap integral squared between the two states
 */
static double Gsqr(const double   Kz,
                   const Subband &isb,
                   const Subband &fsb)
{
 const std::valarray<double> z = isb.z_array();
 const double dz = z[1] - z[0];
 const double nz = z.size();
 const std::valarray<double> psi_i = isb.psi_array();
 const std::valarray<double> psi_f = fsb.psi_array();

 std::complex<double> I(0,1); // Imaginary unit

 // Find form-factor integral
 std::valarray< std::complex<double> > G_integrand_dz(nz);

 for(unsigned int iz=0; iz<nz; ++iz)
     G_integrand_dz[iz] = exp(Kz*z[iz]*I) * psi_i[iz] * psi_f[iz];

 std::complex<double> G = integral(G_integrand_dz, dz);

 return norm(G);
}

/* This function outputs the formfactors into files	*/
void ff_output(const std::valarray<double> &Kz,
               const std::valarray<double> &Gifsqr,
               unsigned int        i,
               unsigned int        f)
{
 char	filename[9];	/* output filename				*/
 sprintf(filename,"G%i%i.r",i,f);	
 write_table(filename, Kz, Gifsqr);
}
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
