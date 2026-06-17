np=1024
nl=1

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
# ./RayTracer
# ./RayTracer \
#     --in "../assets/CornellBoxHR/CornellBox.obj" \
#     --vp 0.0 0.8 2.4 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 54.4 \
#     --res 1280 960 \
#     --r 32 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --ray_normal_bias 0.01 \
#     --o "../demos/cornell_render-np$np-nl$nl-ppm.exr" \
#     --metrics_path "../demos/cornell-np$np-nl$nl-ppm.log"
# iv ../demos/cornell_render-np$np-nl$nl-ppm.exr &> /dev/null

# ./RayTracer \
#     --in "../assets/CornellBox/CornellBox-Water.obj" \
#     --vp 0.0 0.8 2.4 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 54.4 \
#     --res 1280 960 \
#     --r 32 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --ray_normal_bias 0.01 \
#     --o "../demos/cornell_water_render-np$np-nl$nl-ppm.exr" \
#     --metrics_path "../demos/cornell_water-np$np-nl$nl-ppm.log"
# iv ../demos/cornell_water_render-np$np-nl$nl-ppm.exr &> /dev/null

ppm_pc=350000
ppm_ppc=4096
ppm_a=0.7
ppm_sr=0.1
ppm_pgl=1000
method="sppm"
energy_clamping=10.0
suffix=$ppm_pc-$ppm_ppc-$ppm_a-$ppm_sr-$ppm_pgl-$method-$energy_clamping-mis

# ./RayTracer \
#     --in "../assets/TestRoom2/Monkey.glb" \
#     --vp 0.0 0.8 2.4 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 54.4 \
#     --res 1280 960 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --o "../demos/monkey_render-$suffix.exr" \
#     --metrics_path "../demos/monkey_rgb-$suffix.log" \
#     --illumination_technique $method \
#     --ppm_pc $ppm_pc \
#     --ppm_ppc $ppm_ppc \
#     --ppm_a $ppm_a \
#     --ppm_sr $ppm_sr \
#     --ppm_pgl $ppm_pgl \
#     --energy_clamping $energy_clamping
# iv ../demos/monkey_render-$suffix.exr &> /dev/null

# ./RayTracer \
#     --in "../assets/TestRoom/testroom-3.glb" \
#     --vp 0.0 0.8 2.4 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 54.4 \
#     --res 1280 960 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --o "../demos/testroom_3_render-$suffix.exr" \
#     --metrics_path "../demos/testroom_3-$suffix.log" \
#     --illumination_technique $method \
#     --ppm_pc $ppm_pc \
#     --ppm_ppc $ppm_ppc \
#     --ppm_a $ppm_a \
#     --ppm_sr $ppm_sr \
#     --ppm_pgl $ppm_pgl \
#     --energy_clamping $energy_clamping
# iv ../demos/testroom_3_render-$suffix.exr &> /dev/null

# ./RayTracer \
#     --in "../assets/TestRoom2/Monkey-rgb.glb" \
#     --vp 0.0 0.8 2.4 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 54.4 \
#     --res 1280 960 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --o "../demos/monkey_rgb_render-$suffix.exr" \
#     --metrics_path "../demos/monkey_rgb-$suffix.log" \
#     --illumination_technique $method \
#     --ppm_pc $ppm_pc \
#     --ppm_ppc $ppm_ppc \
#     --ppm_a $ppm_a \
#     --ppm_sr $ppm_sr \
#     --ppm_pgl $ppm_pgl \
#     --energy_clamping $energy_clamping 
# iv ../demos/monkey_rgb_render-$suffix.exr &> /dev/null

./RayTracer \
    --in "../assets/CornellBoxHR/CornellBox-glass.glb" \
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
    --o "../demos/cornell_glass_render-$suffix.exr" \
    --metrics_path "../demos/cornell_glass-$suffix.log" \
    --illumination_technique $method \
    --ppm_pc $ppm_pc \
    --ppm_ppc $ppm_ppc \
    --ppm_a $ppm_a \
    --ppm_sr $ppm_sr \
    --ppm_pgl $ppm_pgl \
    --energy_clamping $energy_clamping
iv ../demos/cornell_glass_render-$suffix.exr &> /dev/null


# ./RayTracer \
#     --in "../assets/CornellBoxHR/CornellBox.glb" \
#     --vp 0.0 0.8 2.4 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 60 \
#     --res 1280 960 \
#     --r 32 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --ray_normal_bias 0.01 \
#     --o "../demos/cornellbox_render-$suffix.exr" \
#     --metrics_path "../demos/cornellbox-$suffix.log" \
#     --ppm_pc $ppm_pc \
#     --ppm_ppc $ppm_ppc \
#     --ppm_a $ppm_a \
#     --ppm_sr $ppm_sr \
#     --ppm_pgl $ppm_pgl
# iv ../demos/cornellbox_render-$suffix.exr &> /dev/null

# ./RayTracer \
#     --in "../assets/CornellBoxHR/CornellBox-glass.glb" \
#     --vp 0.0 0.8 2.4 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 54.4 \
#     --res 1280 960 \
#     --r 32 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --ray_normal_bias 0.01 \
#     --o "../demos/cornellbox_glass_render-$suffix.exr" \
#     --metrics_path "../demos/cornellbox_glass-$suffix.log" \
#     --ppm_pc $ppm_pc \
#     --ppm_ppc $ppm_ppc \
#     --ppm_a $ppm_a \
#     --ppm_sr $ppm_sr \
#     --ppm_pgl $ppm_pgl
# iv ../demos/cornellbox_glass_render-$suffix.exr &> /dev/null

# ./RayTracer \
#     --in "../assets/TestRoom/testroom.glb" \
#     --vp 0.0 0.8 2.4 \
#     --vd 0.0 0.0 -1.0 \
#     --up 0.0 1.0 0.0 \
#     --focal_length 1.0 \
#     --fovy 60 \
#     --res 1280 960 \
#     --r 32 \
#     --np $np \
#     --nl $nl \
#     --exr true \
#     --jitter_scale 0 \
#     --engine "Embree" \
#     --ray_normal_bias 0.01 \
#     --o "../demos/testroom_render-$suffix.exr" \
#     --metrics_path "../demos/testroom-$suffix.log" \
#     --ppm_pc $ppm_pc \
#     --ppm_ppc $ppm_ppc \
#     --ppm_a $ppm_a \
#     --ppm_sr $ppm_sr \
#     --ppm_pgl $ppm_pgl
# iv ../demos/testroom_render-$suffix.exr &> /dev/null

# ./RayTracer \
#     --in "./assets/conference/conference.obj" \
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
#     --o "../demos/conference_render-np$np-nl$nl.exr" \
#     --metrics_path "../demos/conference-np$np-nl$nl.log"
# iv ../demos/conference_render-np$np-nl$nl.exr &> /dev/null

# ./RayTracer \
# --in "./assets/breakfast_room/breakfast_room.obj" \
#     -vp -3.0 2.0 3.0 \
#     -vd -0.34 0.0 -0.94 \
#     -up 0.0 1.0 0.0 \
#     -focal_length 1.0 \
#     -fovy 70 \
#     -res 1600 1200 \
#     -r 32 \
#     -np $np \
#     -nl $nl \
#     -exr true \
#     -jitter_scale 0 \
#     -engine "Embree" \
#     -ray_normal_bias 0.01 \
#     -sphere_lights \
#         -2.0 4.0 -2.0 0.91 0.65 0.65 5.0 0.1 \
#         1.0 4.0 -2.0 0.91 0.65 0.65 5.0 0.1 \
#     -o "../demos/breakfast_room_render-np$np-nl$nl.exr" \
#     -metrics_path "../demos/breakfast_room-np$np-nl$nl.log"
# iv ../demos/breakfast_room_render-np$np-nl$nl.exr &> /dev/null
