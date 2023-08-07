#!/bin/bash
N=2500
TIMESTEP=4
dir=outputs

# Step 2: Loop from 0 to  and generate images
for ((CYCLE=0; CYCLE<=N; CYCLE++)); do
    FCYCLE=$(printf "%05d" $CYCLE) # Format CYCLE with leading zeros
    echo "Generating image for cycle $FCYCLE"
    vis/python/plot_spherical.py $dir/my_torus.prim.$FCYCLE.athdf rho $dir/my_torus.$FCYCLE.png --colormap RdBu_r --logc --vmin 1e-6 --vmax 10 -t $TIMESTEP
done

# Step 3: Use FFmpeg to create a video from the generated images
ffmpeg -framerate 50 -i $dir/my_torus.%05d.png -vf "crop=trunc(iw/2)*2:trunc(ih/2)*2" -c:v libx265 -pix_fmt yuv420p $dir/my_torus_video.mp4
