mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
./RayTracer \
    --in "./assets/breakfast_room/breakfast_room.obj" \
    --vp -3.0 2.0 3.0 \
    --vd -0.34 0.0 -0.94 \
    --up 0.0 1.0 0.0 \
    --fovy 70 \
    --res 1280 720 \
    --r 2 \
    --point_lights \
        -2.0 4.0 -2.0 0.91 0.65 0.65  5.0 \
        1.0 4.0 -2.0 0.91 0.65 0.65  5.0 \
    --o "./output/breakfast_render.jpg"