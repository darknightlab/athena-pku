//======================================================================================
// Athena++ astrophysical MHD code
// Copyright (C) 2014 James M. Stone  <jmstone@princeton.edu>
//
// This program is free software: you can redistribute and/or modify it under the terms
// of the GNU General Public License (GPL) as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
// PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of GNU GPL in the file LICENSE included in the code
// distribution.  If not see <http://www.gnu.org/licenses/>.
//======================================================================================
//! \file gow16.cpp
//  \brief implementation of functions in class ChemNetwork, using the GOW16
//  network, see paper by Gong, Ostriker, Wolfire 2016 
//======================================================================================

// this class header
#include "gow16_ng.hpp"

//athena++ header
#include "network.hpp"
#include "../species.hpp"
#include "../../parameter_input.hpp"       //ParameterInput
#include "../../mesh.hpp"
#include "../../hydro/hydro.hpp"
#include "../../radiation/radiation.hpp"
#include "../../utils/cgk_utils.hpp"

//c++ header
#include <vector>
#include <stdexcept>  // std::runtime_error()
#include <sstream>    // stringstream
#include <iostream>   // endl
#include <math.h> //a^x = pow(a,x)
#include <stdio.h>    // c style file


//constants
const Real ChemNetwork::temp_coll_ = 7.0e2;

//species names
const std::string ChemNetwork::species_names[NSPECIES] = 
{"He+", "OHx", "CHx", "CO", "C+", "HCO+", "H2", "H+", "H3+", "H2+", "S+", "Si+",
 "*Si", "*S", "*C", "*O", "*He", "*H", "E"};

//below are ghost species. The aboundances of ghost species are
// recalculated in RHS everytime by other species.
const std::string ChemNetwork::ghost_species_names_[ngs_] = 
{"*e"};

//index of species
const int ChemNetwork::iHeplus_ =
	CGKUtility::FindStrIndex(species_names, NSPECIES, "He+");
const int ChemNetwork::iOHx_ = 
	CGKUtility::FindStrIndex(species_names, NSPECIES, "OHx");
const int ChemNetwork::iCHx_ = 
	CGKUtility::FindStrIndex(species_names, NSPECIES, "CHx");
const int ChemNetwork::iCO_ =
	CGKUtility::FindStrIndex(species_names, NSPECIES, "CO");
const int ChemNetwork::iCplus_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "C+");
const int ChemNetwork::iHCOplus_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "HCO+");
const int ChemNetwork::iH2_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "H2");
const int ChemNetwork::iHplus_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "H+");
const int ChemNetwork::iH3plus_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "H3+");
const int ChemNetwork::iH2plus_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "H2+");
const int ChemNetwork::iSplus_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "S+");
const int ChemNetwork::iSiplus_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "Si+");
const int ChemNetwork::iE_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "E");
const int ChemNetwork::igSi_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "*Si");
const int ChemNetwork::igS_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "*S");
const int ChemNetwork::igC_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "*C");
const int ChemNetwork::igO_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "*O");
const int ChemNetwork::igHe_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "*He");
const int ChemNetwork::igH_ =
  CGKUtility::FindStrIndex(species_names, NSPECIES, "*H");
//index of ghost species
const int ChemNetwork::ige_ =
  CGKUtility::FindStrIndex(ghost_species_names_, ngs_, "*e") + NSPECIES;

//-------------------chemical network---------------------
//cosmic ray chemistry network
// (0) cr + H2 -> H2+ + *e
// (1) cr + *He -> He+ + *e 
// (2) cr + *H  -> H+ + *e 
// -----added as Clark + Glover 2015---- 
// (3) cr + *C -> C+ + *e     --including direct and cr induce photo reactions 
// -----S, CR induced photo ionization, experimenting----
// (4) cr + S -> S+ + e, simply use 2 times rate of C, as in UMIST12
// -----Si, CR induced photo ionization, experimenting----
// (5) cr + Si -> Si+ + e, UMIST12
const int ChemNetwork::icr_H2_ = 0;
const int ChemNetwork::icr_He_ = 1;
const int ChemNetwork::icr_H_ = 2;
const int ChemNetwork::incr_[n_cr_] = 
                        {iH2_, igHe_, igH_,
 												 igC_, igS_, igSi_};
const int ChemNetwork::outcr_[n_cr_] = 
                          {iH2plus_, iHeplus_, iHplus_,
                           iCplus_, iSplus_, iSiplus_};
const Real ChemNetwork::kcr_base_[n_cr_] = 
{2.0, 1.1, 1.0, 
1020., 2040., 4200.}; 

/*2 body reactions*/
/*NOTE: photons from recombination are ignored*/
/* Reactions are, in order.
 * -- are equations of special rate treatment in Glover, Federrath+ 2010:
 (0) H3+ + C -> CH + H2
 (1) H3+ + O -> OH + H2        
 (2) H3+ + CO -> HCO+ + H2
 (3) He+ + H2 -> H+ + He + H   --(89) exp(-35/T)
 (4) He+ + CO -> C+ + *O + *He   
 (5) C+ + H2 -> CH + H         -- schematic reaction for C+ + H2 -> CH2+
 (6) C+ + OH -> HCO+ + *e            -- Schematic equation for C+ + OH -> CO+ + H.
 Use rates in KIDA website.
 (7) CH + O -> CO + H
 (8) OH + C -> CO + H          --exp(0.108/T)
 (9) He+ + *e -> He + *e             --(17) Case B
 (10) H3+ + *e -> H2 + H
 (11) C+ + *e -> C + *e              -- Include RR and DR, Badnell2003, 2006.
 (12) HCO+ + *e -> CO + H
 ----added in GO2012--------
 (13) H2+ + H2 -> H3+ + H       --(54) exp(-T/46600)
 (14) H+ + *e -> H + *e              --(12) Case B
 ---collisional dissociation, only important at high temperature T>1e3---
 (15) H2 + H -> 3 H            --(9) Density dependent. See Glover+MacLow2007
 (16) H2 + H2 -> H2 + 2 H       --(10) Density dependent. See Glover+MacLow2007
 (17) H + *e -> H+ + *e       --(11) Relates to Te
 ----added for H3+ destruction in addition to (10)----
 (18) H3+ + *e -> *3H            --(111)
 ----added He+ destruction in addtion to (3), from UMIST12----
 (19) He+ + H2 -> H2+ + He
 ----added CH reaction to match for abundances of CH---
 (20) CH + H -> H2 + C         
 ----added to match the Meudon code ---
 (21) OH + O -> O + O + H
 ---branching of C+ + H2 ------
 (22) C+ + H2 (+*e) -> C + H + H
 ---S , rate from UMIST12---
 (23) S+ + *e -> S + *e
 (24) C+ + S -> S+ + C
 ---Si , rate from UMIST12---
 (25) Si+ + *e -> Si + *e
 (26) C+ + Si -> Si+ + *C
 --- H2O+ + e reaction ---
 (27) H3+ + *O (+*e) -> H2 + O + H
 --- OH destruction with He+
 (28) He+ + OH (+*e) -> OH + He 
 --- H2+ charge exchange with H ---
 (29) H2+ + H -> H+ + H2 
 */
const int ChemNetwork::i2body_H2_H = 15;
const int ChemNetwork::i2body_H2_H2 = 16;
const int ChemNetwork::i2body_H_e = 17;
const int ChemNetwork::in2body1_[n_2body_] = 
   {iH3plus_, iH3plus_, iH3plus_, iHeplus_, iHeplus_,    
    iCplus_, iCplus_, iCHx_, iOHx_, iHeplus_,
    iH3plus_, iCplus_, iHCOplus_, iH2plus_, iHplus_,
 	  iH2_, iH2_, igH_, iH3plus_, iHeplus_, 
    iCHx_, iOHx_, iCplus_, iSplus_, iCplus_,
    iSiplus_, iCplus_, iH3plus_, iHeplus_, iH2plus_};
const int ChemNetwork::in2body2_[n_2body_] = 
          {igC_, igO_, iCO_, iH2_, iCO_,   
           iH2_, iOHx_, igO_, igC_, ige_,   
           ige_, ige_, ige_, iH2_, ige_,
					 igH_, iH2_, ige_, ige_, iH2_, 
           igH_, igO_, iH2_, ige_, igS_,
           ige_, igSi_, igO_, iOHx_, igH_};
/*Note: output to ghost species doesn't matter. The abundances of ghost species
 * are updated using the other species at every timestep*/
const int ChemNetwork::out2body1_[n_2body_] = 
   {iCHx_, iOHx_, iHCOplus_, iHplus_, iCplus_,   
    iCHx_, iHCOplus_, iCO_, iCO_, igHe_,   
    iH2_, igC_, iCO_, iH3plus_, igH_,
 	  igH_, iH2_, iHplus_, igH_, iH2plus_, 
    iH2_, igO_, igC_, igS_, iSplus_,
    igSi_, iSiplus_, iH2_, iOHx_, iHplus_};
const int ChemNetwork::out2body2_[n_2body_] = 
   {iH2_, iH2_, iH2_, igHe_, igO_,   
    igH_, ige_, igH_, igH_, ige_,   
    igH_, ige_, igH_, igH_, ige_,
  	igH_, igH_, ige_, igH_, igHe_, 
    igC_, igO_, igH_, ige_, igC_,
    ige_, igC_, igO_, igHe_, iH2_};
const int ChemNetwork::out2body3_[n_2body_] = 
   {ige_, ige_, ige_, igH_, igHe_,   
    ige_, ige_, ige_, ige_, ige_,   
    ige_, ige_, ige_, ige_, ige_,
 	  igH_, igH_, ige_, igH_, ige_, 
    ige_, igH_, igH_, ige_, ige_,
    ige_, ige_, igH_, ige_, ige_};
const Real ChemNetwork::k2Texp_[n_2body_] = 
 {0.0, -0.190, 0.0, 0.0, 0.0, 
  -1.3, 0.0, 0.0, -0.339, -0.5, 
  -0.52, 0.0, -0.64,
  0.042, 0.0, 0.0, 0.0, 0.0,
  -0.52, 0.0,
  0.26, 0.0, -1.3, -0.59, 0.0,
  -0.62, 0.0, -0.190, 0.0, 0.0};
const Real ChemNetwork::k2body_base_[n_2body_] = 
                {2.0e-9, 1.99e-9, 1.7e-9, 3.7e-14, 1.6e-9, 
                 3.3e-13 * 0.7, 1.00, 7.0e-11, 7.95e-10, 1.0e-11, 
                 4.54e-7, 1.00, 1.15e-5,
								 2.84e-9, 2.753e-14, 1.00, 1.00, 1.00,
								 8.46e-7, 7.20e-15, 
                 2.81e-11, 3.5e-11, 3.3e-13 * 0.3, 1.6e-10, 5e-11,
                 1.46e-10, 2.1e-9, 1.99e-9, 1.00, 6.4e-10};

/* photo reactions.
 * Reaction rates in Drain 1978 field units.
 * Reactions are, in order:
 (0) h nu + C -> C+ + *e
 (1) h nu + CH -> C + H
 (2) h nu + CO -> C + O            --self-shielding and shielding by H2
 (3) h nu + OH -> O + H
 ----added in GO2012--------
 (4) h nu + H2 -> H + H            --self- and dust shielding
 ----S, from UMIST12
 (5) h nu + *S -> S+ + *e
 ----Si, from UMIST12
 (6) h nu + *Si -> Si+ + *e
 */
const int ChemNetwork::iph_C_ = 0;
const int ChemNetwork::iph_CO_ = 2;
const int ChemNetwork::iph_H2_ = 4;
const int ChemNetwork::inph_[n_ph_] =
      {igC_, iCHx_, iCO_, iOHx_, iH2_,
 			 igS_, igSi_};
const int ChemNetwork::outph1_[n_ph_] = 
      {iCplus_, igC_, igC_, igO_, igH_,
 			 iSplus_, iSiplus_};
const int ChemNetwork::outph2_[n_ph_] = 
      {ige_, igH_, igO_, igH_, igH_,
 			 ige_, ige_};
const Real ChemNetwork::kph_base_[n_ph_] = {3.1e-10, 9.2e-10, 2.6e-10/*Visser2009*/,   
 															  3.9e-10, 5.6e-11, 
                                 6e-10, 3.1e-9}; 
const Real ChemNetwork::kph_avfac_[n_ph_] = {3.33, 1.72, 3.53/*Visser2009*/,  
	                                       2.24, 3.74/*Draine+Bertoldi1996*/,
                                         3.10, 2.3};

/* Grain assisted recombination of H, H2, C+ and H+
 (0) H + H + gr -> H2 + gr
 (1) H+ + *e + gr -> H + gr
 (2) C+ + *e + gr -> C + gr
 (3) He+ + *e + gr -> He + gr
 ------S, from WD2001-----
 (4) S+ + *e + gr -> S + gr
 ------Si, from WD2001-----
 (5) Si+ + *e + gr -> Si + gr
 */
const int ChemNetwork::igr_H_ = 0;
const int ChemNetwork::ingr_[n_gr_] = {igH_, iHplus_, iCplus_, iHeplus_, 
                                       iSplus_, iSiplus_};
const int ChemNetwork::outgr_[n_gr_] = {iH2_, igH_, igC_, igHe_, 
                                        igS_, igSi_};
const Real ChemNetwork::cHp_[7] = {12.25, 8.074e-6, 1.378, 5.087e2, 1.586e-2,
 										            	 0.4723, 1.102e-5}; 
const Real ChemNetwork::cCp_[7] = {45.58, 6.089e-3, 1.128, 4.331e2, 4.845e-2,
                                   0.8120, 1.333e-4};
const Real ChemNetwork::cHep_[7] = {5.572, 3.185e-7, 1.512, 5.115e3, 3.903e-7,
                                    0.4956, 5.494e-7};
const Real ChemNetwork::cSp_[7] = {3.064, 7.769e-5, 1.319, 1.087e2, 3.475e-1,
                                   0.4790, 4.689e-2};
const Real ChemNetwork::cSip_[7] = {2.166, 5.678e-8, 1.874, 4.375e4, 1.635e-6,
                                    0.8964, 7.538e-5};
//-----------------end of chemical network---------------------


ChemNetwork::ChemNetwork(ChemSpecies *pspec, ParameterInput *pin) {
	//number of species and a list of name of species
  pmy_spec_ = pspec;
	pmy_mb_ = pspec->pmy_block;
  //check whether number of frequencies equal to the input file specification
  const int nfreq = pin->GetOrAddInteger("radiation", "n_frequency",1);
  std::stringstream msg; //error message
  if (nfreq != n_freq_) {
    msg << "### FATAL ERROR in ChemNetwork constructor" << std::endl
      << "number of frequencies in radiation:" << nfreq 
      << " not equal to that in chemistry" << n_freq_  << std::endl;
    throw std::runtime_error(msg.str().c_str());
  }

	//set the parameters from input file
	zdg_ = pin->GetOrAddReal("chemistry", "Zdg", 1.);//dust and gas metallicity
	//cosmic ray ionization rate per H
	cr_rate_ = pin->GetOrAddReal("chemistry", "CR", 2e-16);
	//Veolcity dispersion of CO in km/s for calculating NCOeff in CO cooling
	bCO_ = pin->GetOrAddReal("chemistry", "bCO", 1.);
  //units of density and radiation
	unit_density_in_nH_ = pin->GetReal("chemistry", "unit_density_in_nH");
	unit_radiation_in_draine1987_ = pin->GetReal(
                                "chemistry", "unit_radiation_in_draine1987");
  //constant temperature
  is_const_temp_ = pin->GetOrAddInteger("chemistry", "const_T_flag", 0);
  if (is_const_temp_) {
    temperature_ = pin->GetReal("chemistry", "temperature");
  } else {
    temperature_ = 0.;
  }

  //initialize rates to zero
  for (int i=0; i<n_cr_; i++) {
    kcr_[i] = 0;
  }
  for (int i=0; i<n_2body_; i++) {
    k2body_[i] = 0;
  }
  for (int i=0; i<n_ph_; i++) {
    kph_[i] = 0;
  }
  for (int i=0; i<n_gr_; i++) {
    kgr_[i] = 0;
  }
  //copy species to a full list of species names
  for (int i=0; i<NSPECIES; i++) {
    species_names_all_[i] = species_names[i];
  }
  for (int i=NSPECIES; i<NSPECIES+ngs_; i++) {
    species_names_all_[i] = ghost_species_names_[i-NSPECIES];
  }

}

ChemNetwork::~ChemNetwork() {
}

void ChemNetwork::RHS(const Real t, const Real y[NSPECIES], Real ydot[NSPECIES]) {
	Real rate;
	//store previous y includeing ghost species
	Real yprev[NSPECIES+ngs_];
	Real ydotg[NSPECIES+ngs_];

	for(int i=0; i<NSPECIES+ngs_; i++) {
		ydotg[i] = 0.0;
	}

	// copy y to yprev and set ghost species
	GetGhostSpecies(y, yprev);
  UpdateRates(yprev);

	//cosmic ray reactions
	for (int i=0; i<n_cr_; i++) {
		rate = kcr_[i] * yprev[incr_[i]];
		ydotg[incr_[i]] -= rate;
		ydotg[outcr_[i]] += rate;
	}

	for (int i=0; i<n_2body_; i++) {
		rate =  k2body_[i] * yprev[in2body1_[i]] * yprev[in2body2_[i]];
		ydotg[in2body1_[i]] -= rate;
		ydotg[in2body2_[i]] -= rate;
		ydotg[out2body1_[i]] += rate;
		ydotg[out2body2_[i]] += rate;
		ydotg[out2body3_[i]] += rate;
	}

	//photo reactions
	for (int i=0; i<n_ph_; i++) {
		rate = kph_[i] * yprev[inph_[i]];
		ydotg[inph_[i]] -= rate;
		ydotg[outph1_[i]] += rate;
		ydotg[outph2_[i]] += rate;
	}

	//grain assisted reactions
  //(0) H + H + gr -> H2 + gr
	rate = kgr_[0] * yprev[ingr_[0]];
	ydotg[ingr_[0]] -= 2.*rate;
	ydotg[outgr_[0]] += rate;
	for (int i=1; i<n_gr_; i++) {
		rate = kgr_[i] * yprev[ingr_[i]];
		ydotg[ingr_[i]] -= rate;
		ydotg[outgr_[i]] += rate;
	}

	//special reactions
  // crphoto + CO -> O + C       
	rate = 10. * cr_rate_ * 2*yprev[iH2_] * yprev[iCO_];
	ydotg[iCO_] -= rate;
	ydotg[igO_] += rate;
	ydotg[igC_] += rate;
  // cr + CO + H -> HCO+ + *e  --schematic for cr + CO -> CO+ + e
	rate = 6.52 * cr_rate_ * yprev[iCO_];
	ydotg[iCO_] -= rate;
	ydotg[igH_] -= rate;
	ydotg[iHCOplus_] += rate;

  //energy equation
  //TODO: need to include energy equation later.
  if (!is_const_temp_) {
    //ydotg[iE_] = dEdt_(yprev);
  }
	//set ydot to return
	for (int i=0; i<NSPECIES; i++) {
		ydot[i] = ydotg[i];
	}
  return;
}

void ChemNetwork::Jacobian(const Real t,
               const Real y[NSPECIES], const Real fy[NSPECIES], 
               Real jac[NSPECIES][NSPECIES],
               Real tmp1[NSPECIES], Real tmp2[NSPECIES], Real tmp3[NSPECIES]) {
	Real rate_pa = 0; // rate for partial derivative respect to species a
	Real rate_pb = 0;
	int ia, ib, ic, id;//index for species a, b, c, d
	//store previous y with ghost species
	double yprev[NSPECIES+ngs_];
	//Jacobian include ghost indexes
	Real jac_[NSPECIES+ngs_][NSPECIES+ngs_];

	// copy y to yprev and set ghost species
	GetGhostSpecies(y, yprev);
  // TODO: We can might skip this, which was caluclated in RHS
  //UpdateRates(yprev);

	//initialize jac_ to be zero
	for (int i=0; i<NSPECIES+ngs_; i++) {
		for (int j=0; j<NSPECIES+ngs_; j++) {
			jac_[i][j] = 0;
		}
	}

	// 2 body reactions: a+b -> c+d
	for (int i=0; i<n_2body_; i++) {
		ia = in2body1_[i];
		ib = in2body2_[i];
		ic = out2body1_[i];
		id = out2body2_[i];
		rate_pa = k2body_[i] * yprev[ib];
		rate_pb = k2body_[i] * yprev[ia];
		jac_[ia][ia] -= rate_pa;
		jac_[ib][ia] -= rate_pa;
		jac_[ic][ia] += rate_pa;
		jac_[id][ia] += rate_pa;
		jac_[ia][ib] -= rate_pb;
		jac_[ib][ib] -= rate_pb;
		jac_[ic][ib] += rate_pb;
		jac_[id][ib] += rate_pb;
	}
	// photo reactions a + photon -> c+d
	for (int i=0; i<n_ph_; i++) {
		ia = inph_[i];
		ic = outph1_[i];
		rate_pa = kph_[i];
		jac_[ia][ia] -= rate_pa;
		jac_[ic][ia] += rate_pa;
	}
	//Cosmic ray reactions a + cr -> c
	for (int i=0; i<n_cr_; i++) {
		ia = incr_[i];
		ic = outcr_[i];
		rate_pa = kcr_[i];
		jac_[ia][ia] -= rate_pa;
		jac_[ic][ia] += rate_pa;
	}

	//grain reactions a + gr -> c
	for (int i=0; i<n_gr_; i++) {
		ia = ingr_[i];
		ic = outgr_[i];
		rate_pa = kgr_[i];
		jac_[ia][ia] -= rate_pa;
		jac_[ic][ia] += rate_pa;
	}

	//copy J to return
	for (int i=0; i<NSPECIES; i++) {
		for (int j=0; j<NSPECIES; j++) {
			jac[i][j] = jac_[i][j];
		}
	}
  return;
}

void ChemNetwork::InitializeNextStep(const int k, const int j, const int i) {
  Real rad_sum;
  int nang = pmy_mb_->prad->nang;
  //density
  nH_ = pmy_mb_->phydro->u(IDN, k, j, i) / unit_density_in_nH_;
  //average radiation field of all angles
  for (int ifreq=0; ifreq < n_freq_; ++ifreq) {
    rad_sum = 0;
    for (int iang=0; iang < nang; ++iang) {
      rad_sum += pmy_mb_->prad->ir(k, j, i, ifreq * nang + iang);
    }
    rad_[ifreq] = rad_sum / nang / unit_radiation_in_draine1987_;
  }
	return;
}

void ChemNetwork::OutputProperties(FILE *pf) const {
  //output the reactions and base rates
	for (int i=0; i<n_cr_; i++) {
		fprintf(pf, "cr  + %4s -> %4s,     kcr = %.2e\n", 
		 species_names_all_[incr_[i]].c_str(), species_names_all_[outcr_[i]].c_str(), kcr_[i]);
	}
	for (int i=0; i<n_2body_; i++) {
		fprintf(pf, "%4s  + %4s -> %4s  + %4s  + %4s,     k2body = %.2e\n", 
		 species_names_all_[in2body1_[i]].c_str(),
		 species_names_all_[in2body2_[i]].c_str(),
		 species_names_all_[out2body1_[i]].c_str(),
		 species_names_all_[out2body2_[i]].c_str(),
		 species_names_all_[out2body3_[i]].c_str(),
		 k2body_[i]);
	}
	for (int i=0; i<n_ph_; i++) {
		fprintf(pf, "h nu  + %4s -> %4s  + %4s,     kph = %.2e\n", 
		 species_names_all_[inph_[i]].c_str(),
		 species_names_all_[outph1_[i]].c_str(),
		 species_names_all_[outph2_[i]].c_str(),
		 kph_[i] );
	}
	for (int i=0; i<n_gr_; i++) {
		fprintf(pf, "gr  + %4s -> %4s,     kgr = %.2e\n", 
		 species_names_all_[ingr_[i]].c_str(), species_names_all_[outgr_[i]].c_str(),
		 kgr_[i]);
	}
  return;
}

void ChemNetwork::GetGhostSpecies(const Real *y, Real yghost[NSPECIES+ngs_]) {
	//copy the aboundances in y to yghost
	for (int i=0; i<NSPECIES; i++) {
		yghost[i] = y[i];
	}
	//set the ghost species
  yghost[ige_] = yghost[iHeplus_] + yghost[iCplus_] + yghost[iHCOplus_]
                     + yghost[iH3plus_] + yghost[iH2plus_] + yghost[iHplus_]
                     + yghost[iSplus_] + yghost[iSiplus_]; 
	return;
}

Real ChemNetwork::CII_rec_rate_(const Real temp) {
  Real A, B, T0, T1, C, T2, BN, term1, term2, alpharr, alphadr;
  A = 2.995e-9;
  B = 0.7849;
  T0 =  6.670e-3;
  T1 = 1.943e6;
  C = 0.1597;
  T2 = 4.955e4;
  BN = B + C * exp(-T2/temp);
  term1 = sqrt(temp/T0);
  term2 = sqrt(temp/T1);
  alpharr = A / ( term1*pow(1.0+term1, 1.0-BN) * pow(1.0+term2, 1.0+BN) );
  alphadr = pow( temp, -3.0/2.0 ) * ( 6.346e-9 * exp(-1.217e1/temp) +
        9.793e-09 * exp(-7.38e1/temp) + 1.634e-06 * exp(-1.523e+04/temp) );
  return (alpharr+alphadr);
}

void ChemNetwork::UpdateRates(const Real y[NSPECIES+ngs_]) {
  Real T;
  //temperature TODO: constant for now, need evolution later.
  if (is_const_temp_) {
    T = temperature_;
  } else {
    //need to calculate temperature. In slab models:
    //T = y[iE_] / Thermo::CvCold(y[iH2_], xHe_, y[ie_]);
  }
	const Real logT = log10(T);
	const Real logT4 = log10(T/1.0e4);
	const Real lnTe = log(T * 8.6173e-5);
  Real ncr, n2ncr;
	Real psi; /*H+ grain recombination parameter*/
  Real kcr_H_fac;//ratio of total rate to primary rate
  Real psi_gr_fac_;
  const Real kida_fac = ( 0.62 + 45.41 / sqrt(T) ) * nH_;
	/*cosmic ray reactions*/
	for (int i=0; i<n_cr_; i++) {
		kcr_[i] = kcr_base_[i] * cr_rate_;
	}
  //cosmic ray induced photo-reactions, proportional to x(H2)
  //(0) cr + H2 -> H2+ + *e
  //(1) cr + *He -> He+ + *e 
  //(2) cr + *H  -> H+ + *e 
  //(3) cr + *C -> C+ + *e     --including direct and cr induce photo reactions 
  //(4) cr + S -> S+ + e, simply use 2 times rate of C, as in UMIST12
  //(5) cr + Si -> Si+ + e, UMIST12 
  kcr_H_fac = 1.15 * 2*y[iH2_] + 1.5 * y[igH_];
  kcr_[0] *= kcr_H_fac;
  kcr_[2] *= kcr_H_fac;
  kcr_[3] *= (2*y[iH2_] + 3.85/kcr_base_[3]);
  kcr_[4] *= 2*y[iH2_];
  kcr_[5] *= 2*y[iH2_];
	//2 body reactions
	for (int i=0; i<n_2body_; i++){
		k2body_[i] = k2body_base_[i] * pow(T, k2Texp_[i]) * nH_;
	}
	/*Special treatment of rates for some equations*/
	/*(3) He+ + H2 -> H+ + *He + *H   --(89) exp(-35/T) */
	k2body_[3] *= exp(-35./T);
  /*(5) C+ + H2 -> CH + *H         -- schematic reaction for C+ + H2 -> CH2+*/
  k2body_[5] *= exp(-23./T);
  /* ---branching of C+ + H2 ------
 (22) C+ + H2 + *e -> *C + *H + *H*/
  k2body_[22] *= exp(-23./T);
  /* (6) C+ + OH -> HCO+             -- Schematic equation for C+ + OH -> CO+ + H.
     Use rates in KIDA website.*/
  k2body_[6] = 9.15e-10 * kida_fac;
  /*(8) OH + *C -> CO + *H          --exp(0.108/T)*/
  k2body_[8] *= exp(0.108/T);
	/*(9) He+ + *e -> *He             --(17) Case B */
	k2body_[9] *= 11.19 + (-1.676 + (-0.2852 + 0.04433*logT) * logT )* logT;
  /* (11) C+ + *e -> *C              -- Include RR and DR, Badnell2003, 2006. */
  k2body_[11] = CII_rec_rate_(T) * nH_;
  /* (13) H2+ + H2 -> H3+ + *H       --(54) exp(-T/46600) */
  k2body_[13] *= exp(- T/46600.);
	/* (14) H+ + *e -> *H              --(12) Case B */
	k2body_[14] *= pow( 315614.0 / T, 1.5) 
									 * pow(  1.0 + pow( 115188.0 / T, 0.407) , -2.242 );
  //k2body_[14] = 3.5e-12 * pow( 300./T, 0.75 ) * nH_;
  /* (28) He+ + OH -> *H + *He + *O(O+)*/
  k2body_[28] = 1.35e-9 * kida_fac;
  /*--- H2O+ + e branching--
  (1) H3+ + *O -> OH + H2        
  (27) H3+ + *O + *e -> H2 + *O + *H     */
  const double h2oplus_ratio = 
                k2body_[1] * y[iH2_] / ( 3.5e-7 * sqrt(300./T) * y[ige_] * nH_ );
  k2body_[1] *= h2oplus_ratio / (h2oplus_ratio + 1.);
  k2body_[27] *= 1. / (h2oplus_ratio + 1.);

  //Collisional dissociation, k>~1.0e-30 at T>~5e2.
  Real k9l, k9h, k10l, k10h, ncrH, ncrH2;
  if (T > temp_coll_) {
    /*(15) H2 + *H -> 3 *H   
     *(16) H2 + H2 -> H2 + 2 *H
     * --(9) Density dependent. See Glover+MacLow2007*/
  	k9l = 6.67e-12 * sqrt(T) * exp(-(1. + 63590./T)); 
    k9h = 3.52e-9 * exp(-43900.0 / T);
    k10l = 5.996e-30 * pow(T, 4.1881) / pow((1.0 + 6.761e-6 * T), 5.6881)  
            * exp(-54657.4 / T);
    k10h = 1.3e-9 * exp(-53300.0 / T); 
    ncrH = pow(10, (3.0 - 0.416 * logT4 - 0.327 * logT4*logT4));
    ncrH2 = pow(10, (4.845 - 1.3 * logT4 + 1.62 * logT4*logT4));
    ncr = 1. / ( y[igH_]/ncrH + y[iH2_]/ncrH2 );
    n2ncr = nH_ / ncr;
    k2body_[15] = pow(10, log10(k9h) *  n2ncr/(1. + n2ncr) 
                         + log10(k9l) / (1. + n2ncr)) * nH_;
    k2body_[16] = pow(10, log10(k10h) *  n2ncr/(1. + n2ncr) 
                         + log10(k10l) / (1. + n2ncr)) * nH_;
    /* (17) *H + *e -> H+ + 2 *e       --(11) Relates to Te */
    k2body_[17] *= exp( -3.271396786e1 + 
                      (1.35365560e1 + (- 5.73932875 + (1.56315498 
                    + (- 2.877056e-1 + (3.48255977e-2 + (- 2.63197617e-3
                    + (1.11954395e-4 + (-2.03914985e-6)
        *lnTe)*lnTe)*lnTe)*lnTe)*lnTe)*lnTe)*lnTe)*lnTe
                     );
  } else {
    k2body_[15] = 0.;
    k2body_[16] = 0.;
    k2body_[17] = 0.;
  }
  
	//photo reactions
	for (int i=0; i<n_ph_; i++) {
    kph_[i] = kph_base_[i] * rad_[i];
	}

	// Grain assisted recombination of H and H2
	//	 (0) *H + *H + gr -> H2 + gr , from Draine book chapter 31.2 page 346,
	//	 Jura 1975
	kgr_[0] = 3.0e-18 * sqrt(T) * nH_ * zdg_;
	//	 (1) H+ + *e + gr -> *H + gr
  //	 (2) C+ + *e + gr -> *C + gr
  //   (3) He+ + *e + gr -> *He + gr
  //   (4) S+ + *e + gr -> *S + gr
  //   (5) Si+ + *e + gr -> *Si + gr
  //   , rate dependent on e aboundance. 
	psi_gr_fac_ = 1.7 * rad_[index_gpe_] * sqrt(T) / nH_; 
	psi = psi_gr_fac_ / y[ige_];
	kgr_[1] = 1.0e-14 * cHp_[0] / 
		           (
			           1.0 + cHp_[1]*pow(psi, cHp_[2]) * 
								   (1.0 + cHp_[3] * pow(T, cHp_[4])
										             *pow( psi, -cHp_[5]-cHp_[6]*log(T) ) 
									 ) 
								) * nH_ * zdg_;
	kgr_[2] = 1.0e-14 * cCp_[0] / 
		           (
			           1.0 + cCp_[1]*pow(psi, cCp_[2]) * 
								   (1.0 + cCp_[3] * pow(T, cCp_[4])
										             *pow( psi, -cCp_[5]-cCp_[6]*log(T) ) 
									 ) 
								) * nH_ * zdg_;
	kgr_[3] = 1.0e-14 * cHep_[0] / 
		           (
			           1.0 + cHep_[1]*pow(psi, cHep_[2]) * 
								   (1.0 + cHep_[3] * pow(T, cHep_[4])
										             *pow( psi, -cHep_[5]-cHep_[6]*log(T) ) 
									 ) 
								) * nH_ * zdg_;
	kgr_[4] = 1.0e-14 * cSp_[0] / 
		           (
			           1.0 + cSp_[1]*pow(psi, cSp_[2]) * 
								   (1.0 + cSp_[3] * pow(T, cSp_[4])
										             *pow( psi, -cSp_[5]-cSp_[6]*log(T) ) 
									 ) 
								) * nH_ * zdg_;
	kgr_[5] = 1.0e-14 * cSip_[0] / 
		           (
			           1.0 + cSip_[1]*pow(psi, cSip_[2]) * 
								   (1.0 + cSip_[3] * pow(T, cSip_[4])
										             *pow( psi, -cSip_[5]-cSip_[6]*log(T) ) 
									 ) 
								) * nH_ * zdg_;

  return;
}
