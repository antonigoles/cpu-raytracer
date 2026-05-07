np=16
nl=4

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
./RayTracer \
    --in "./assets/conference/conference.obj" \
    --vp 2.5 1.8 6.5 \
    --vd -0.3 -0.2 -1.0 \
    --up 0.0 1.0 0.0 \
    --focal_length 1.0 \
    --fovy 60 \
    --res 1600 1200 \
    --r 32 \
    --np $np \
    --nl $nl \
    --exr true \
    --jitter_scale 0 \
    --engine "Embree" \
    --ray_normal_bias 0.01 \
    --o "../demos/conference_render-np$np-nl$nl.exr" \
    --metrics_path "../demos/conference-np$np-nl$nl.log"
iv ../demos/conference_render-np$np-nl$nl.exr &> /dev/null