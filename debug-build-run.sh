mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
./RayTracer \
    --in "./assets/CornellBoxRGK/CornellBox-Sphere.obj" \
    --vp 0.0 0.8 2.4 \
    --vd 0.0 0.0 -1.0 \
    --up 0.0 1.0 0.0 \
    --focal_length 1.0 \
    --fovy 60 \
    --res 640 480 \
    --r 16 \
    --np 4132 \
    --nl 1 \
    --exr \
    --jitter_scale 0.01 \
    --engine "Embree" \
    --ray_normal_bias 0.01 \
    --o "../demos/cornell_render-embree.exr" \
    --metrics_path "../demos/cornell-embree.exr"