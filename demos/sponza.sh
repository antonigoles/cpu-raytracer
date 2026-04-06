./build/RayTracer \
    --in "./assets/Sponza/sponza.obj" \
    --vp 10.0 8.8 0.0 \
    --vd 1.0 0.0 0.0 \
    --up 0.0 1.0 0.0 \
    --fovy 70 \
    --res 1920 1080 \
    --r 8 \
    --engine "HavranKDTree" \
    --ray_normal_bias 0.0001 \
    --sphere_lights \
        -5.0 10.0 0.0  0.13 0.22 1.00  500.0 \
         0.0 10.0 0.0  0.08 0.74 0.16  500.0 \
         5.0 10.0 0.0  0.61 0.09 0.09  500.0 \
    --o "./demos/sponza_render-havran.jpg" \
    --metrics_path "./demos/sponza-havran.log"


./build/RayTracer \
    --in "./assets/Sponza/sponza.obj" \
    --vp 10.0 8.8 0.0 \
    --vd 1.0 0.0 0.0 \
    --up 0.0 1.0 0.0 \
    --fovy 70 \
    --res 1920 1080 \
    --r 8 \
    --engine "Embree" \
    --ray_normal_bias 0.0001 \
    --sphere_lights \
        -5.0 10.0 0.0  0.13 0.22 1.00  500.0 \
         0.0 10.0 0.0  0.08 0.74 0.16  500.0 \
         5.0 10.0 0.0  0.61 0.09 0.09  500.0 \
    --o "./demos/sponza_render-embree.jpg" \
    --metrics_path "./demos/sponza-embree.log"