np=9160
nl=0

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
# ./RayTracer
# ./RayTracer \
#     --in "./assets/CornellBoxRGK/CornellBox-Sphere.obj" \
#     --vp 0.0 0.8 2.4 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 60 \
#     --res 1600 1200 \
#     --r 32 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --ray_normal_bias 0.01 \
#     --o "../demos/cornell_render-np$np-nl$nl.exr" \
#     --metrics_path "../demos/cornell-np$np-nl$nl.log"
# iv ../demos/cornell_render-np$np-nl$nl.exr &> /dev/null

# ./RayTracer \
#     --in "./assets/bedroom/iscv2.obj" \
#     --vp 2.5 1.8 6.5 \
#     --vd -0.3 -0.2 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 60 \
#     --res 1600 1200 \
#     --r 32 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --ray_normal_bias 0.01 \
#     --o "../demos/bedroom_render-np$np-nl$nl.exr" \
#     --metrics_path "../demos/bedroom-np$np-nl$nl.log"
# iv ../demos/bedroom_render-np$np-nl$nl.exr &> /dev/null

./RayTracer \
--in "./assets/breakfast_room/breakfast_room.obj" \
    --vp -3.0 2.0 3.0 \
    --vd -0.34 0.0 -0.94 \
    --up 0.0 1.0 0.0 \
    --focal_length 1.0 \
    --fovy 70 \
    --res 1600 1200 \
    --r 32 \
    --np $np \
    --nl $nl \
    --exr true \
    --jitter_scale 0 \
    --engine "Embree" \
    --ray_normal_bias 0.01 \
    --sphere_lights \
        -2.0 4.0 -2.0 0.91 0.65 0.65 5.0 0.1 \
        1.0 4.0 -2.0 0.91 0.65 0.65 5.0 0.1 \
   --o "../demos/breakfast_room_render-np$np-nl$nl.exr" \
    --metrics_path "../demos/breakfast_room-np$np-nl$nl.log"
iv ../demos/breakfast_room_render-np$np-nl$nl.exr &> /dev/null
