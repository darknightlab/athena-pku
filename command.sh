#!/bin/bash

# This script is example of build, plot, ...

mkdir outputs

# Configure
python configure.py -g -b --prob gr_my_torus --coord=kerr-schild --flux hlle --nghost 4 -hdf5 --hdf5_path=/usr/lib/hdf5 -omp

# Build with openmp
make clean && make -j 16

# Plot Mesh
./bin/athena -i inputs/mhd_gr/athinput.my_torus -m 16
python vis/python/plot_mesh.py --output outputs/mesh.jpg

# Plot Initial Condition
./bin/athena -i inputs/mhd_gr/athinput.my_torus time/tlim=0 -d outputs
vis/python/plot_spherical.py outputs/my_torus.prim.00000.athdf rho outputs/initial.png --colormap RdBu_r --logc