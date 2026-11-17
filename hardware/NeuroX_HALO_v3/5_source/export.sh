#!/bin/bash
# exports every part (and every colour of multi-colour parts) in print and model orientation
cd "$(dirname "$0")"
mkdir -p stl stl_model
JOBS="front:MG,WH,BL side_L:MG,WH,BL side_R:MG,WH,BL strap:MG,WH,BL fitcheck:MG,WH,BL crest:WH,BL,MG,RD temple_cap:WH,BL dial:WH,BL back_cover:WH,BL hub_cap:WH,BL cup:WH,BL slider_temple_L:MG,BL slider_temple_R:MG,BL slider_ref_L:MG,BL slider_ref_R:MG,BL slider_nose:MG,BL stem_test:MG,BL rib_spine:ALL rib_side:ALL hub:ALL hub_ring:ALL back_hub:ALL band_clip:ALL arm_clip:ALL cup:ALL slider_temple_L:ALL slider_temple_R:ALL slider_ref_L:ALL slider_ref_R:ALL slider_nose:ALL stem_test:ALL knob:ALL front:ALL side_L:ALL side_R:ALL strap:ALL fitcheck:ALL crest:ALL temple_cap:ALL dial:ALL back_cover:ALL hub_cap:ALL"
for j in ${@:-$JOBS}; do p=${j%%:*}; for c in $(echo ${j#*:} | tr , ' '); do for o in print model; do
  d=stl; [ $o = model ] && d=stl_model; n=$p; [ $c != ALL ] && n=${p}__$c
  ( openscad -o $d/$n.stl -D "PART=\"$p\"" -D "COL=\"$c\"" -D "ORIENT=\"$o\"" neurox_halo_print.scad > $d/$n.log 2>&1 || echo "FAIL $n $o" ) &
  while [ $(jobs -r | wc -l) -ge 3 ]; do sleep 0.5; done
done; done; done
wait
grep -l -i "warning\|error" stl/*.log stl_model/*.log 2>/dev/null | sed 's/^/WARN /'
ls stl/*.stl | wc -l
