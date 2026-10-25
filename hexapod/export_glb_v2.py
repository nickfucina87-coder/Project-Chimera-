"""Export rig + skinned mesh + every animation action as one .glb for Unreal (same axes/units as the original mesh import).
Run: blender -b Ironclad_Hexapod_Anim_v2.blend --python export_glb_v2.py   (never saves the .blend)"""
import bpy, os
W = os.path.dirname(bpy.data.filepath)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
keep = {arm.name} | {o.name for o in bpy.data.objects if o.type == 'MESH' and o.parent == arm}
for o in [o for o in bpy.data.objects if o.name not in keep]:
    bpy.data.objects.remove(o, do_unlink=True)
# glTF exports actions that are stashed/active on the armature; push every action into its own NLA track
arm.animation_data_create()
arm.animation_data.action = None
for tr in list(arm.animation_data.nla_tracks): arm.animation_data.nla_tracks.remove(tr)
for a in bpy.data.actions:
    t = arm.animation_data.nla_tracks.new(); t.name = a.name
    s = t.strips.new(a.name, int(a.frame_range[0]), a); t.mute = True
for pb in arm.pose.bones: pb.matrix_basis.identity()
out = os.path.join(W, "Ironclad_Hexapod_v2_Animated.glb")
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=False, export_animations=True,
                          export_animation_mode='NLA_TRACKS', export_frame_range=False, export_force_sampling=True,
                          export_def_bones=False, export_anim_single_armature=True, export_reset_pose_bones=True,
                          export_apply=False, export_yup=True, export_skins=True)
print("EXPORTED", out, os.path.getsize(out) // 1024, "KB", "actions:", len(bpy.data.actions))
