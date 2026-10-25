# Warehouse Bible - rules taken from the Factory sample (/Game/Maps/Demonstration)

Measured on 2026-10-25 from all 11,872 placed objects in the sample (`sample_dump.json`, reference shots in `shots/`).
Floor in the sample is z=20; heights below are measured from the floor. Our Lvl_Warehouse floor is z=-76.

## Density (a busy 10 x 10 m area)
- ~107 meshes, ~19 decals, ~5 lights. Nothing is a bare floor, wall or ceiling.
- Decals (grime, stains, lane paint, hazard stripes, zone numbers) are a big part of the look: 1,687 of them.

## Ceiling is built in layers (top -> down)
1. Roof trusses / purlins.
2. Beam grid (SM_MetalBeam*, ~11-14 m).
3. Ducts (SM_pipe_conditioning*, SM_AirConditioningPipe_01): median bottom 13.9 m, run along beams, branch with tees, end at a vent/unit.
4. Pipes (SM_Pipe_round_long / short / corner2 / tee_transition / grate): median 11.4 m.
5. Cable bundles (SM_Cables*): median 11 m, laid along the beams.
6. Lights: SM_LampSet_01 fluorescent strips in continuous rows, ~1.35 m apart along the row, hung at ~10 m.
   Hanging single lamps (SM_Lamp_with_Rope01) every ~9 m at ~5.5 m in lower areas.

## Cables (416 in the sample)
- Only 1.4% touch nothing but other cables. Cables are chains of segments that always start and end on something.
- 64% are fixed to a beam, 60% touch a machine/prop, 44% run past/along a lamp row, 33% reach the floor, 18% end at an electrical box.
- A cable leaves a machine, rises straight up to the beam grid, runs along the beams, and drops at a junction box,
  switchboard or wall panel. Floor cables run along walls or machine bases, never across walkways.
- Orientation: laid flat with no roll/pitch on floor and ceiling runs (roll 0). Only short drops use 90/270 roll.
  No upside-down (roll 180) cables except 3 out of 416.

## Pipes (1,396 pieces)
- 96% are connected at both ends. Runs are built from long + short straights, corners and tees,
  and always end in a wall, a machine, a grate/vent or another pipe. A pipe never ends in the air.

## Walls and floors
- Walls carry lockers, electrical groups (SM_ElectricalSupply_Group04), junction boxes, fire signals,
  extinguishers, hydrant cabinets and pipes running along them.
- Floors: pedestrian paths (SM_PedestrianPath01), rubber floor, floor mats, drains, lane lines and hazard stripes at
  every machine edge. Zone signs ("ZONE B3") hang from the beams above each area.

## Storage
- Racks are pre-filled merged meshes: SM_StorageShelves_group02/03/04/06 plus SM_Boxes_group_01..18,
  SM_PaperBox_*, SM_PalletCart_PalletBox_*, plastic boxes and pallets on the floor in front.

## Process we follow in Lvl_Warehouse
1. Per room: delete every cable/pipe piece not attached at both ends; nothing upside down.
2. Build power routes: power room -> wall panels -> ceiling cable trays on the beam grid -> drops to each machine/server row.
3. Ceiling layers, then walls, then floor paint/decals, then clutter, in that order.
4. Start at the entrance and move room by room.
