np=16
nl=4
ppm_pc=1500000
ppm_ppc=8
ppm_a=0.93
ppm_sr=0.05
ppm_pgl=1500
method="sppm"
suffix=$ppm_pc-$ppm_ppc-$ppm_a-$ppm_sr-$ppm_pgl-$method

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

./RayTracer \
    --in "../assets/TestRoom2/Monkey-rgb.glb" \
    --vp 0.0 0.8 2.4 \
    --vd 0.0 0.0 -1.0 \
    --up 0.0 1.0 0.0 \
    --focal_length 1.0 \
    --fovy 54.4 \
    --res 1280 960 \
    --np $np \
    --nl $nl \
    --exr true \
    --jitter_scale 0 \
    --engine "Embree" \
    --o "../demos/monkey_rgb_render-$suffix.exr" \
    --metrics_path "../demos/monkey_rgb-$suffix.log" \
    --illumination_technique $method \
    --ppm_pc $ppm_pc \
    --ppm_ppc $ppm_ppc \
    --ppm_a $ppm_a \
    --ppm_sr $ppm_sr \
    --ppm_pgl $ppm_pgl
iv ../demos/monkey_rgb_render-$suffix.exr &> /dev/null
