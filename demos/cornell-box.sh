# ./build/RayTracer \
#     --in "./assets/CornellBox/CornellBox-Sphere.obj" \
#     --vp 0.0 0.8 2.0 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --fovy 60 \
#     --res 1920 1080 \
#     --r 8 \
#     --engine "HavranKDTree" \
#     --ray_normal_bias 0.0001 \
#     --point_lights \
#         0.0 1.5 0.0  1.0 1.0 1.0  10.0 \
#     --o "./demos/cornell_render-havran.jpg" \
#     --metrics_path "./demos/cornell-havran.log"

./build/RayTracer \
    --in "./assets/CornellBox/CornellBox-Sphere.obj" \
    --vp 0.0 0.8 2.4 \
    --vd 0.0 0.0 -1.0 \
    --up 0.0 1.0 0.0 \
    --fovy 60 \
    --res 1600 1200 \
    --r 4 \
    --engine "Embree" \
    --ray_normal_bias 0.01 \
    --point_lights \
        0.0 1.5 0.0  1.0 1.0 1.0  10.0 \
    --o "./demos/cornell_render-embree.jpg" \
    --metrics_path "./demos/cornell-embree.log"