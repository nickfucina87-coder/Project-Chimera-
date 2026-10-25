"""Animation-only FBX for UE: the rig plus a tiny stand-in mesh (the real 2.3M-tri mesh crashes the editor on import).
Run: blender -b Ironclad_Hexapod_Anim.blend --python export_anims_light.py
Drops CTRL_* helper actions. Never saves the .blend."""
import bpy, os

W = os.path.dirname(bpy.data.filepath)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')

for o in [o for o in bpy.data.objects if o.type != 'ARMATURE']:
    bpy.data.objects.remove(o, do_unlink=True)
for a in [a for a in bpy.data.actions if a.name.startswith("CTRL_")]:
    bpy.data.actions.remove(a)

def fcurves(action):
    if hasattr(action, "layers") and action.layers:
        for layer in action.layers:
            for strip in layer.strips:
                for bag in strip.channelbags:
                    yield from bag.fcurves
    else:
        yield from action.fcurves

# UE rejects animations whose length isn't a whole frame; stretch e.g. 28.8 -> 29 frames.
for a in bpy.data.actions:
    start, end = a.frame_range
    target = round(end)
    if abs(end - target) > 1e-3 or abs(start) > 1e-3:
        k = (target - start) / (end - start) if end != start else 1.0
        for fc in fcurves(a):
            for kp in fc.keyframe_points:
                for p in (kp.co, kp.handle_left, kp.handle_right):
                    p.x = (p.x - start) * k
        a.use_frame_range = True
        a.frame_start, a.frame_end = 0, target
        print("RETIMED", a.name, (start, end), "->", (0, target))

me = bpy.data.meshes.new("HexapodAnimProxy")
s = 0.02
verts = [(x * s, y * s, z * s) for x in (-1, 1) for y in (-1, 1) for z in (-1, 1)]
faces = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
me.from_pydata(verts, [], faces)
proxy = bpy.data.objects.new("HexapodAnimProxy", me)
bpy.context.scene.collection.objects.link(proxy)
proxy.parent = arm
vg = proxy.vertex_groups.new(name="Root")
vg.add(range(8), 1.0, 'REPLACE')
proxy.modifiers.new("Armature", 'ARMATURE').object = arm

bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True)
proxy.select_set(True)
bpy.context.view_layer.objects.active = arm
out = os.path.join(W, os.path.splitext(os.path.basename(bpy.data.filepath))[0].replace("Anim", "Anims") + "_light.fbx")
bpy.ops.export_scene.fbx(filepath=out, use_selection=True,
    object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=True,
    bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True, apply_scale_options='FBX_SCALE_ALL',
    axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', use_armature_deform_only=False)
print("EXPORTED", out, "actions:", [a.name for a in bpy.data.actions])
