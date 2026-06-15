np=16
nl=4

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
./RayTracer \
    --in "../assets/TestRoom/TestRoom.obj" \
    --vp 0.0 0.8 2.4 \
    --vd 0.0 0.0 -1.0 \
    --up 0.0 1.0 0.0 \
    --focal_length 1.0 \
    --fovy 60 \
    --res 1280 960 \
    --r 32 \
    --np $np \
    --nl $nl \
    --exr true \
    --jitter_scale 0 \
    --engine "Embree" \
    --ray_normal_bias 0.01 \
    --o "../demos/testroom_render-np$np-nl$nl.exr" \
    --metrics_path "../demos/testroom-np$np-nl$nl.log"
iv ../demos/testroom_render-np$np-nl$nl.exr &> /dev/null