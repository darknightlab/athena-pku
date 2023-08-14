#!/bin/bash

# This script is example of build, plot, ...

mkdir outputs



# Configure
python configure.py -g -b --prob gr_my_torus --coord=kerr-schild --flux hlle --nghost 4 -hdf5 --hdf5_path=/usr/lib/hdf5 -omp

# Build with openmp
make clean && make -j 16

export NAME=my_torus
cp inputs/mhd_gr/athinput.$NAME outputs/athinput.$NAME
# Plot Initial Condition
./bin/athena -i inputs/mhd_gr/athinput.$NAME time/tlim=0 -d outputs
vis/python/plot_spherical.py outputs/my_torus.prim.00000.athdf rho outputs/initial.png --colormap RdBu_r --logc -t 10

# Plot Mesh
./bin/athena -i inputs/mhd_gr/athinput.$NAME -m 1
python vis/python/plot_mesh.py --output outputs/mesh.jpg

# Run
./bin/athena -i inputs/mhd_gr/athinput.$NAME -d outputs

vis/python/plot_spherical.py outputs/my_torus.prim.00000.athdf rho outputs/initial.png --colormap RdBu_r --logc --stream Bcc -r 40 -t 10

export CYCLE=00000 && vis/python/plot_spherical.py outputs/my_torus.prim.$CYCLE.athdf rho outputs/my_torus.$CYCLE.png --colormap RdBu_r --logc --vmin 1e-6 --vmax 10 -t 10