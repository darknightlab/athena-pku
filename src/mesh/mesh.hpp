#ifndef MESH_HPP
#define MESH_HPP
//========================================================================================
// Athena++ astrophysical MHD code
// Copyright(C) 2014 James M. Stone <jmstone@princeton.edu> and other code contributors
// Licensed under the 3-clause BSD License, see LICENSE file for details
//========================================================================================
//! \file mesh.hpp
//  \brief defines Mesh and MeshBlock classes, and various structs used in them
//  The Mesh is the overall grid structure, and MeshBlocks are local patches of data
//  (potentially on different levels) that tile the entire domain.

// C/C++ headers
#include <stdint.h>  // int64_t
#include <string>

// Athena++ classes headers
#include "../athena.hpp"
#include "../athena_arrays.hpp"
#include "../parameter_input.hpp"
#include "../outputs/io_wrapper.hpp"
#include "../task_list/task_list.hpp"
#include "../bvals/bvals.hpp"
#include "meshblock_tree.hpp"
#include "mesh_refinement.hpp"

// Forward declarations
class ParameterInput;
class Mesh;
class MeshRefinement;
class MeshBlockTree;
class BoundaryValues;
class GravityBoundaryValues;
class TaskList;
class TaskState;
class Coordinates;
class Reconstruction;
class Hydro;
class Field;
class Gravity;
class MGGravityDriver;
class EquationOfState;
class FFTDriver;
class FFTGravityDriver;
class TurbulenceDriver;
class Radiation;


//----------------------------------------------------------------------------------------
//! \class MeshBlock
//  \brief data/functions associated with a single block

class MeshBlock {
  friend class RestartOutput;
  friend class BoundaryValues;
  friend class GravityBoundaryValues;
  friend class Mesh;
  friend class Hydro;
  friend class TaskList;
  friend class Radiation;
#ifdef HDF5OUTPUT
  friend class ATHDF5Output;
#endif

public:
  MeshBlock(int igid, int ilid, LogicalLocation iloc, RegionSize input_size,
            enum BoundaryFlag *input_bcs, enum BoundaryFlag *input_rad_bcs, 
            Mesh *pm, ParameterInput *pin, int igflag, bool ref_flag = false);
  MeshBlock(int igid, int ilid, Mesh *pm, ParameterInput *pin, LogicalLocation iloc,
            RegionSize input_block, enum BoundaryFlag *input_bcs, 
            enum BoundaryFlag *input_rad_bcs, Real icost, char *mbdata,
            int igflag);
  ~MeshBlock();

  //data
  Mesh *pmy_mesh;  // ptr to Mesh containing this MeshBlock
  LogicalLocation loc;
  RegionSize block_size;
  int is,ie,js,je,ks,ke;
  int gid, lid;
  int cis,cie,cjs,cje,cks,cke,cnghost;
  int gflag;
  // Track the partial dt abscissae for substepping each memory register, relative to t^n
  Real step_dt[3];

  // user output variables for analysis
  int nuser_out_var;
  AthenaArray<Real> user_out_var;
  std::string *user_out_var_names_;

  // user MeshBlock data that can be stored in restart files
  AthenaArray<Real> *ruser_meshblock_data;
  AthenaArray<int> *iuser_meshblock_data;

  // mesh-related objects
  Coordinates *pcoord;
  BoundaryValues *pbval;
  GravityBoundaryValues *pgbval;
  Reconstruction *precon;
  MeshRefinement *pmr;

  // physics-related objects
  Hydro *phydro;
  Field *pfield;
  Radiation *prad;
  int nrad_var; // total number of radiation variables needed for boundary
  Gravity *pgrav;
  EquationOfState *peos;

  MeshBlock *prev, *next;

  // functions
  size_t GetBlockSizeInBytes(void);
  void SearchAndSetNeighbors(MeshBlockTree &tree, int *ranklist, int *nslist);
  void UserWorkInLoop(void); // in ../pgen
  void InitUserMeshBlockData(ParameterInput *pin); // in ../pgen
  void UserWorkBeforeOutput(ParameterInput *pin); // in ../pgen

private:
  // data
  Real cost;
  Real new_block_dt;
  TaskState tasks;
  int nreal_user_meshblock_data_, nint_user_meshblock_data_;

  // functions
  void AllocateRealUserMeshBlockDataField(int n);
  void AllocateIntUserMeshBlockDataField(int n);
  void AllocateUserOutputVariables(int n);
  void SetUserOutputVariableName(int n, const char *name);

  void ProblemGenerator(ParameterInput *pin); // in ../pgen
};

//----------------------------------------------------------------------------------------
//! \class Mesh
//  \brief data/functions associated with the overall mesh

class Mesh {
  friend class RestartOutput;
  friend class HistoryOutput;
  friend class MeshBlock;
  friend class BoundaryBase;
  friend class BoundaryValues;
  friend class MGBoundaryValues;
  friend class GravityBoundaryValues;
  friend class Coordinates;
  friend class MeshRefinement;
  friend class HydroSourceTerms;
  friend class Hydro;
  friend class FFTDriver;
  friend class FFTGravityDriver;
  friend class TurbulenceDriver;
  friend class MultigridDriver;
  friend class MGGravityDriver;
  friend class Gravity;
#ifdef HDF5OUTPUT
  friend class ATHDF5Output;
#endif

public:
  Mesh(ParameterInput *pin, int test_flag=0);
  Mesh(ParameterInput *pin, IOWrapper &resfile, int test_flag=0);
  ~Mesh();

  // accessors
  int GetNumMeshBlocksThisRank(int my_rank) {return nblist[my_rank];}
  int GetNumMeshThreads() const {return num_mesh_threads_;}
  int64_t GetTotalCells() {return (int64_t)nbtotal*
     pblock->block_size.nx1*pblock->block_size.nx2*pblock->block_size.nx3;}

  // data
  RegionSize mesh_size;
  enum BoundaryFlag mesh_bcs[6];
  enum BoundaryFlag mesh_rad_bcs[6];
  Real start_time, tlim, cfl_number, time, dt;
  int nlim, ncycle, ncycle_out;
  int nbtotal, nbnew, nbdel;
  bool adaptive, multilevel;
  int gflag;
  int turb_flag; // turbulence flag

  MeshBlock *pblock;

  TurbulenceDriver *ptrbd;
  FFTGravityDriver *pfgrd;
  MGGravityDriver *pmgrd;

  AthenaArray<Real> *ruser_mesh_data;
  AthenaArray<int> *iuser_mesh_data;

  // functions
  void Initialize(int res_flag, ParameterInput *pin);
  void SetBlockSizeAndBoundaries(LogicalLocation loc, RegionSize &block_size,
              enum BoundaryFlag *block_bcs, enum BoundaryFlag *block_rad_bcs);
  void NewTimeStep(void);
  void AdaptiveMeshRefinement(ParameterInput *pin);
  unsigned int CreateAMRMPITag(int lid, int ox1, int ox2, int ox3);
  MeshBlock* FindMeshBlock(int tgid);
  void ApplyUserWorkBeforeOutput(ParameterInput *pin);
  void UserWorkAfterLoop(ParameterInput *pin); // method in ../pgen

private:
  // data
  int root_level, max_level, current_level;
  int num_mesh_threads_;
  int *nslist, *ranklist, *nblist;
  Real *costlist;
  int *nref, *nderef, *bnref, *bnderef, *rdisp, *brdisp, *ddisp, *bddisp;
  LogicalLocation *loclist;
  MeshBlockTree tree;
  long int nrbx1, nrbx2, nrbx3;
  bool use_meshgen_fn_[3]; // flag to use non-uniform or user meshgen function
  int nreal_user_mesh_data_, nint_user_mesh_data_;

  int nuser_history_output_;
  std::string *user_history_output_names_;

  // global constants
  Real four_pi_G_, grav_eps_;

  // functions
  MeshGenFunc_t MeshGenerator_[3];
  SrcTermFunc_t UserSourceTerm_;
  BValFunc_t BoundaryFunction_[6];
  RadBValFunc_t RadBoundaryFunction_[6]; // Function Pointer for radiation
  AMRFlagFunc_t AMRFlag_;
  TimeStepFunc_t UserTimeStep_;
  HistoryOutputFunc_t *user_history_func_;
  MetricFunc_t UserMetric_;
  MGBoundaryFunc_t MGBoundaryFunction_[6];
  GravityBoundaryFunc_t GravityBoundaryFunction_[6];

  void AllocateRealUserMeshDataField(int n);
  void AllocateIntUserMeshDataField(int n);
  void OutputMeshStructure(int dim);
  void LoadBalance(Real *clist, int *rlist, int *slist, int *nlist, int nb);

  // methods in ../pgen
  void InitUserMeshData(ParameterInput *pin);
  void EnrollUserBoundaryFunction (enum BoundaryFace face, BValFunc_t my_func);
  void EnrollUserRadBoundaryFunction (enum BoundaryFace face, RadBValFunc_t my_func);
  void EnrollUserRefinementCondition(AMRFlagFunc_t amrflag);
  void EnrollUserMeshGenerator(enum CoordinateDirection dir, MeshGenFunc_t my_mg);
  void EnrollUserExplicitSourceFunction(SrcTermFunc_t my_func);
  void EnrollUserTimeStepFunction(TimeStepFunc_t my_func);
  void AllocateUserHistoryOutput(int n);
  void EnrollUserHistoryOutput(int i, HistoryOutputFunc_t my_func, const char *name);
  void EnrollUserMetric(MetricFunc_t my_func);
  void EnrollUserMGBoundaryFunction(enum BoundaryFace dir, MGBoundaryFunc_t my_bc);
  void EnrollUserGravityBoundaryFunction(enum BoundaryFace dir, GravityBoundaryFunc_t my_bc);
  void SetGravitationalConstant(Real g) { four_pi_G_=4.0*PI*g; };
  void SetFourPiG(Real fpg) { four_pi_G_=fpg; };
  void SetGravityThreshold(Real eps) { grav_eps_=eps; };
};

//----------------------------------------------------------------------------------------
// \!fn Real DefaultMeshGeneratorX1(Real x, RegionSize rs)
// \brief x1 mesh generator function, x is the logical location; x=i/nx1

inline Real DefaultMeshGeneratorX1(Real x, RegionSize rs)
{
  Real lw, rw;
  if(rs.x1rat==1.0) {
    rw=x, lw=1.0-x;
  } else {
    Real ratn=pow(rs.x1rat,rs.nx1);
    Real rnx=pow(rs.x1rat,x*rs.nx1);
    lw=(rnx-ratn)/(1.0-ratn);
    rw=1.0-lw;
  }
  return rs.x1min*lw+rs.x1max*rw;
}

//----------------------------------------------------------------------------------------
// \!fn Real DefaultMeshGeneratorX2(Real x, RegionSize rs)
// \brief x2 mesh generator function, x is the logical location; x=j/nx2

inline Real DefaultMeshGeneratorX2(Real x, RegionSize rs)
{
  Real lw, rw;
  if(rs.x2rat==1.0) {
    rw=x, lw=1.0-x;
  } else {
    Real ratn=pow(rs.x2rat,rs.nx2);
    Real rnx=pow(rs.x2rat,x*rs.nx2);
    lw=(rnx-ratn)/(1.0-ratn);
    rw=1.0-lw;
  }
  return rs.x2min*lw+rs.x2max*rw;
}

//----------------------------------------------------------------------------------------
// \!fn Real DefaultMeshGeneratorX3(Real x, RegionSize rs)
// \brief x3 mesh generator function, x is the logical location; x=k/nx3

inline Real DefaultMeshGeneratorX3(Real x, RegionSize rs)
{
  Real lw, rw;
  if(rs.x3rat==1.0) {
    rw=x, lw=1.0-x;
  } else {
    Real ratn=pow(rs.x3rat,rs.nx3);
    Real rnx=pow(rs.x3rat,x*rs.nx3);
    lw=(rnx-ratn)/(1.0-ratn);
    rw=1.0-lw;
  }
  return rs.x3min*lw+rs.x3max*rw;
}

#endif  // MESH_HPP
