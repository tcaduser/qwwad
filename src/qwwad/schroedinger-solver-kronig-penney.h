/**
 * \file   schroedinger-solver-kronig-penney.h
 * \author Alex Valavanis <a.valavanis@leeds.ac.uk>
 * \author Paul Harrison  <p.harrison@shu.ac.uk>
 * \brief  Schroedinger solver for a Kronig-Penney superlattice
 */

#ifndef QWWAD_SCHROEDINGER_SOLVER_KRONIG_PENNEY_H
#define QWWAD_SCHROEDINGER_SOLVER_KRONIG_PENNEY_H

#include "schroedinger-solver.h"

#include <armadillo>

namespace QWWAD
{
/**
 * Schroedinger solver for a Kronig-Penney superlattice
 */
class SchroedingerSolverKronigPenney : public SchroedingerSolver
{
public:
    SchroedingerSolverKronigPenney(const double l_w,
                                   const double l_b,
                                   const double V,
                                   const double m_w,
                                   const double m_b,
                                   const double k,
                                   const size_t nz,
                                   const size_t nper = 10,
                                   const unsigned int nst_max = 1);

    std::string get_name() {return "kronig-penney";}

    static double test_matching(double v,
                                void   *params);

    double get_lhs(const double v) const;
    double get_rhs() const;

private:
    double _l_w; ///< Width of well [m]
    double _l_b; ///< Width of well [m]
    double _V0;  ///< Well depth [J]
    double _m_w; ///< Effective mass in well [kg]
    double _m_b; ///< Effective mass in barriers [kg]
    double _k;   ///< Wave vector [1/m]

    void calculate();

    arma::cx_mat get_matching_matrix(const double E) const;
    arma::vec get_wavefunction(const double E) const;
};
} // namespace
#endif
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
