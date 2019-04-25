#ifndef ODE_WRAPPER_HPP
#define ODE_WRAPPER_HPP
//======================================================================================
// Athena++ astrophysical MHD code
// Copyright (C) 2014 James M. Stone  <jmstone@princeton.edu>
// See LICENSE file for full public license information.
//======================================================================================
//! \file ode_wrapper.hpp
//  \brief definitions for ode solver classes.
//======================================================================================

// Athena++ classes headers
#include "../athena.hpp"
#include "../athena_arrays.hpp"
#include "network/network.hpp" 
//CVODE headers
#include <sundials/sundials_types.h> /* realtype type*/
#include <sundials/sundials_dense.h>
#include <nvector/nvector_serial.h> /* N_Vector type*/
#include <cvode/cvode.h>            /* CVODE solver fcts., consts. */
#include <cvode/cvode_direct.h>       /* prototype for CVDense */
#include <sunmatrix/sunmatrix_dense.h> /* access to dense SUNMatrix            */
#include <sunlinsol/sunlinsol_dense.h> /* access to dense SUNLinearSolver      */

class ParameterInput;
class Species;

//! \class ODEWrapper
//  \brief Wrapper for ODE solver, CVODE
class ODEWrapper {
public:
  //Constructor: Initialize CVODE, allocate memory for the ODE solver.
  ODEWrapper(Species *pspec, ParameterInput *pin);
  ~ODEWrapper();
  //Update abundance in species over time dt.
  // For each cell:
  // Step 1: Set the radiation field strength in ChemNetwork.
  // Depends on the data structure of radiation field, this can be copying
  // the value from Radiation class to ChemNetwork class, or just pass a pointer.
  //
  // Step 2: re-initialize CVODE with starting time t, and starting abundance
  // y. If x(k, j, i, ispec), we can just pass a pointer to CVODE, otherwise,
  // we need to copy the abundance of species to an array.
  //
  // Step 3: Integration. Update the array of species abundance in that
  // cell over time dt.
  // 
  // Note that this will be not vectorizable(?).
  void Integrate();

  //solve the chemical abundance to equilibrium. Useful for post-processing.
  void SolveEq();

  void SetInitStep(const Real h_init);
  //Get the last step size
  Real GetLastStep() const;
  //Get the next step size
  Real GetNextStep() const;
  //Get the number of steps between two reinits.
  long int GetNsteps() const;

private:
  Species *pmy_spec_;
  Real reltol_;//relative tolerance
  Real abstol_[NSPECIES];//absolute tolerance
  SUNMatrix dense_matrix_;
  SUNLinearSolver dense_ls_;
  void *cvode_mem_;
  N_Vector y_;
  Real *ydata_;
  Real h_init_;

  //CVODE checkflag
  void CheckFlag(const void *flagvalue, const char *funcname, 
                 const int opt) const;
};


#endif // ODE_WRAPPER_HPP