import sys

sys.path.insert(0, "vis/python")
import athena_read

dir = "outputs"
prim_file_path = dir + "/my_torus.prim.00000.athdf"
user_file_path = dir + "/my_torus.user.00000.athdf"


prim_data = athena_read.athdf(prim_file_path)
user_data = athena_read.athdf(user_file_path)

print(dir)
print("pmag", user_data["pmag"].max())
print("rho", prim_data["rho"].max())
print("press", prim_data["press"].max())
print("pmag/rho", user_data["pmag"].max() / prim_data["rho"].max())
print("press/rho", prim_data["press"].max() / prim_data["rho"].max())
print("pmag/press", user_data["pmag"].max() / prim_data["press"].max())
print("press/pmag", prim_data["press"].max() / user_data["pmag"].max())
