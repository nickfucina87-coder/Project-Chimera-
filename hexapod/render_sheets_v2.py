"""Workbench contact sheets for every v2 action (never saves the .blend).
Run: blender -b Ironclad_Hexapod_Anim_v2.blend --python render_sheets_v2.py -- [ActionName ...]"""
import bpy, os, sys, math
import numpy as np
from mathutils import Vector, Matrix

W = os.path.dirname(bpy.data.filepath); OUTD = os.path.join(W, "previews_v2"); os.makedirs(OUTD, exist_ok=True)
only = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
sc = bpy.context.scene; arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
for o in bpy.data.objects:
    if o.type == 'MESH' and o.name != "IroncladHexapod_Mesh": o.hide_render = True
sc.render.engine = 'BLENDER_WORKBENCH'; sc.render.resolution_x, sc.render.resolution_y = 400, 225
sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'OBJECT'
sc.display.shading.show_shadows = True
sc.render.film_transparent = False
GROUND = -0.302
import json
QUADS = json.load(open(os.path.join(W, "anim_manifest_v2.json"))).get("preview_quads", {})
cam = bpy.data.objects.new("SheetCam", bpy.data.cameras.new("SheetCam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.lens = 35

def quad(name, center, u, v, color, parent=None, mat=Matrix()):
    me = bpy.data.meshes.new(name); c = Vector(center); u = Vector(u); v = Vector(v)
    me.from_pydata([mat @ (c - u - v), mat @ (c + u - v), mat @ (c + u + v), mat @ (c - u + v)], [], [(0, 1, 2, 3)])
    o = bpy.data.objects.new(name, me); sc.collection.objects.link(o); o.color = color
    if parent: o.parent = parent
    return o

CUR_M = [Matrix()]
GREY = (0.45, 0.47, 0.5, 1); BLUE = (0.35, 0.45, 0.6, 1)
def root_world(f):
    sc.frame_set(f); return CUR_M[0] @ arm.pose.bones["Root"].matrix @ arm.data.bones["Root"].matrix_local.inverted()

def scene_for(name, n):
    """Returns (temp objects, armature world matrix) for the action's surfaces."""
    objs = []; M = Matrix()
    if name in QUADS:
        for i, q in enumerate(QUADS[name]):
            me = bpy.data.meshes.new("surf"); me.from_pydata([Vector(p) for p in q], [], [(0, 1, 2, 3)])
            o = bpy.data.objects.new("surf", me); sc.collection.objects.link(o); o.color = (GREY, BLUE)[i % 2]; objs.append(o)
    elif name in ("Trans_Floor_To_Wall", "Trans_Wall_To_Ceiling"):
        Dw = 0.85
        objs += [quad("floor", (0, 0.4, GROUND), (1.5, 0, 0), (0, 1.25, 0), GREY), quad("wall", (0, -Dw, GROUND + 1.2), (1.5, 0, 0), (0, 0, 1.2), BLUE)]
    elif name == "Trans_Over_Ledge":
        E = 0.62
        objs += [quad("floor", (0, 0.6, GROUND), (1.5, 0, 0), (0, E + 0.6, 0) * 1, GREY), quad("face", (0, -E, GROUND - 1.2), (1.5, 0, 0), (0, 0, 1.2), BLUE)]
        objs[0].data.vertices[0].co.y = -E; objs[0].data.vertices[1].co.y = -E
    elif name in ("Jump_Wall_To_Floor", "Drop_Wall_To_Floor", "Drop_Ceiling_To_Floor"):
        M = {"Jump_Wall_To_Floor": Matrix.Rotation(math.radians(90), 4, 'X'),
             "Drop_Wall_To_Floor": Matrix.Rotation(math.radians(-90), 4, 'X'),
             "Drop_Ceiling_To_Floor": Matrix.Rotation(math.radians(180), 4, 'Y')}[name]
        arm.matrix_world = M; CUR_M[0] = M; bpy.context.view_layer.update()
        R0 = root_world(0); Rn = root_world(n)
        objs += [quad("start", (0, 0, GROUND), (1.4, 0, 0), (0, 1.4, 0), BLUE, mat=R0),
                 quad("end", (0, 0, GROUND), (1.6, 0, 0), (0, 1.6, 0), GREY, mat=Rn)]
    else:
        objs.append(quad("floor", (0, 0, GROUND), (2, 0, 0), (0, 2, 0), GREY))
    return objs, M

def place_camera(name, n, M):
    rm = name.startswith(("Trans_", "Jump_Wall", "Drop_"))
    if rm:
        pts = [root_world(f).translation for f in range(0, n + 1, 4)]
        c = sum(pts, Vector()) / len(pts); ext = max((p - c).length for p in pts)
        cam.location = c + Vector((3.6 + ext * 1.6, 0.0, 0.35))
        cam.rotation_euler = (c - cam.location).to_track_quat('-Z', 'Y').to_euler()
    else:
        cam.location = Vector((2.3, -2.6, 1.25)); tgt = Vector((0, -0.1, -0.12))
        cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z', 'Y').to_euler()

def render_strip(name, frames, tag):
    tiles = []
    for f in frames:
        sc.frame_set(f); p = os.path.join(OUTD, f"_tmp_{tag}_{f}.png"); sc.render.filepath = p
        bpy.ops.render.render(write_still=True)
        img = bpy.data.images.load(p); a = np.empty(len(img.pixels), np.float32); img.pixels.foreach_get(a)
        tiles.append(a.reshape(img.size[1], img.size[0], 4)); bpy.data.images.remove(img); os.remove(p)
    return np.concatenate(tiles, axis=1)

def save(arr, path):
    h, w = arr.shape[:2]; img = bpy.data.images.new("sheet", w, h, alpha=True)
    img.pixels.foreach_set(arr.ravel()); img.filepath_raw = path; img.file_format = 'PNG'; img.save(); bpy.data.images.remove(img)

acts = [a for a in bpy.data.actions if not only or a.name in only]
for act in acts:
    n = int(act.frame_range[1]); arm.animation_data.action = act
    objs, M = scene_for(act.name, n); place_camera(act.name, n, M)
    k = 6 if not act.use_cyclic else 4
    frames = sorted({round(i * n / (k - 1)) for i in range(k)}) if not act.use_cyclic else [round(i * n / k) for i in range(k)]
    save(render_strip(act.name, frames, act.name), os.path.join(OUTD, f"{act.name}.png"))
    print("[SHEET]", act.name, frames)
    for o in objs: bpy.data.objects.remove(o, do_unlink=True)
    arm.matrix_world = Matrix(); CUR_M[0] = Matrix()
