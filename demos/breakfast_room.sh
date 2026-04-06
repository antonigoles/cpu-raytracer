./build/RayTracer \
    --in "./assets/breakfast_room/breakfast_room.obj" \
    --vp -3.0 2.0 3.0 \
    --vd -0.34 0.0 -0.94 \
    --up 0.0 1.0 0.0 \
    --fovy 70 \
    --res 1920 1080 \
    --r 8 \
    --engine "HavranKDTree" \
    --ray_normal_bias 0.01 \
    --sphere_lights \
        -2.0 4.0 -2.0 0.91 0.65 0.65  5.0 0.1 \
        1.0 4.0 -2.0 0.91 0.65 0.65  5.0 0.1 \
    --o "./demos/breakfast_render-havran.jpg" \
    --metrics_path "./demos/breakfast_room-havran.log"


./build/RayTracer \
    --in "./assets/breakfast_room/breakfast_room.obj" \
    --vp -3.0 2.0 3.0 \
    --vd -0.34 0.0 -0.94 \
    --up 0.0 1.0 0.0 \
    --fovy 70 \
    --res 1920 1080 \
    --r 8 \
    --engine "Embree" \
    --ray_normal_bias 0.01 \
    --sphere_lights \
        -2.0 4.0 -2.0 0.91 0.65 0.65 5.0 0.1 \
        1.0 4.0 -2.0 0.91 0.65 0.65 5.0 0.1 \
    --o "./demos/breakfast_render-embree.jpg" \
    --metrics_path "./demos/breakfast_room-embree.log"