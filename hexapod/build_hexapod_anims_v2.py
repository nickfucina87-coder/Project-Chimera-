"""Ironclad Hexapod - animation set v2.
Run: blender -b Ironclad_Hexapod_Anim.blend --python build_hexapod_anims_v2.py
Reads the v1 .blend (never overwrites it) and writes Ironclad_Hexapod_Anim_v2.blend.

All poses are solved with an analytic per-leg IK on the real joint pivots
(hip yaw+pitch, knee pitch, ankle pitch), then baked to FK quaternion keys.
Coordinates: forward = -Y, left = +X, up = +Z, 24 fps, metres."""
import bpy, os, math, json
import numpy as np
from mathutils import Vector, Matrix

W = r"C:\Users\nickf\Documents\Chimera_Hexapod_Anim"
OUT = os.path.join(W, "Ironclad_Hexapod_Anim_v2.blend")
FPS = 24
LOG = []
def log(*a):
    s = " ".join(str(x) for x in a); print("[HEX2]", s); LOG.append(s)

sc = bpy.context.scene; sc.render.fps = FPS
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
body_mesh = bpy.data.objects["IroncladHexapod_Mesh"]

def fcurves(act):
    if hasattr(act, "layers") and act.layers:
        for layer in act.layers:
            for strip in layer.strips:
                for bag in strip.channelbags:
                    yield from bag.fcurves
    else:
        yield from act.fcurves

# ---------------------------------------------------------------- clean slate
# v1 leftovers: generated gaits (rebuilt here), IK helpers (Blender IK solved the wrong segments).
for a in list(bpy.data.actions):
    if a.name != "Walk_TripodGait":
        bpy.data.actions.remove(a)
for pb in arm.pose.bones:
    for c in list(pb.constraints):
        pb.constraints.remove(c)
for o in list(bpy.data.objects):
    if o.name.startswith("CTRL_") or o.name == "PreviewCam":
        bpy.data.objects.remove(o, do_unlink=True)
for c in list(bpy.data.collections):
    if c.name.startswith("CTRL_"):
        bpy.data.collections.remove(c)

arm.animation_data_create(); arm.animation_data.action = None
for pb in arm.pose.bones: pb.matrix_basis = Matrix()
bpy.context.view_layer.update()

REST = {b.name: b.matrix_local.copy() for b in arm.data.bones}
HEAD = {b.name: b.head_local.copy() for b in arm.data.bones}
Z = Vector((0, 0, 1)); FWD = Vector((0, -1, 0)); LEFT = Vector((1, 0, 0))
LEGS = ["L_front", "L_mid", "L_rear", "R_front", "R_mid", "R_rear"]
FRONT, MID, REAR = ["L_front", "R_front"], ["L_mid", "R_mid"], ["L_rear", "R_rear"]

# ---------------------------------------------------------------- mesh measurements
me = body_mesh.data
co = np.empty(len(me.vertices) * 3, np.float32); me.vertices.foreach_get("co", co)
co = co.reshape(-1, 3).astype(np.float64)
mw = np.array(body_mesh.matrix_world); co = co @ mw[:3, :3].T + mw[:3, 3]
TIP = {}
for L in LEGS:
    A = HEAD[L + "_ankle"]
    m = (np.hypot(co[:, 0] - A.x, co[:, 1] - A.y) < 0.15) & (co[:, 2] < A.z)
    i = np.flatnonzero(m)[np.argmin(co[m, 2])]
    TIP[L] = Vector(co[i])
GROUND = sum(t.z for t in TIP.values()) / 6
for L in LEGS: TIP[L].z = GROUND
HB = -GROUND
core = co[np.abs(co[:, 0]) < 0.17]
NOSE, TAIL = core[:, 1].min(), core[:, 1].max()
BELLY, TOP = core[:, 2].min(), core[:, 2].max()
BOX = [Vector((x, y, z)) for x in (-0.2, 0.2) for y in (NOSE, TAIL) for z in (BELLY, TOP)]
del co, core
log("tips", {L: tuple(round(v, 3) for v in TIP[L]) for L in LEGS})
log("ground", round(GROUND, 3), "nose", round(NOSE, 3), "tail", round(TAIL, 3), "belly", round(BELLY, 3), "top", round(TOP, 3))

def wrap(a): return (a + math.pi) % (2 * math.pi) - math.pi
def smooth(a): a = min(1.0, max(0.0, a)); return a * a * (3 - 2 * a)
def clamp(a, lo=0.0, hi=1.0): return min(hi, max(lo, a))
def mir(L, v): return Vector((v[0] * (1 if L[0] == "L" else -1), v[1], v[2]))
T = Matrix.Translation

# ---------------------------------------------------------------- leg IK
STATS = {}
class Leg:
    def __init__(s, name):
        s.name = name
        s.H, s.K, s.A, s.T = HEAD[name + "_hip"], HEAD[name + "_knee"], HEAD[name + "_ankle"], TIP[name]
        d = s.T - s.H
        s.u = Vector((d.x, d.y, 0)).normalized()
        s.n = s.u.cross(Z)             # hinge axis: positive rotation lifts u toward +Z
        s.yaw0 = math.atan2(s.u.y, s.u.x)
        pl = lambda v: ((v - s.H).dot(s.u), (v - s.H).z)
        k, a, t = pl(s.K), pl(s.A), pl(s.T)
        s.l1, s.l2, s.l3 = math.hypot(*k), math.dist(k, a), math.dist(a, t)
        s.t1 = math.atan2(k[1], k[0])
        s.t2 = math.atan2(a[1] - k[1], a[0] - k[0])
        s.t3 = math.atan2(t[1] - a[1], t[0] - a[0])

    def solve(s, P, nrm=None, k=0.0, fadj=0.0):
        """P: foot tip target in body space. nrm: surface normal (body space) to tilt the foot toward."""
        d = P - s.H
        yaw = wrap(math.atan2(d.y, d.x) - s.yaw0) if d.xy.length > 1e-6 else 0.0
        Ry = Matrix.Rotation(yaw, 3, 'Z'); dl = Ry.transposed() @ d
        pu, pz = dl.dot(s.u), dl.z
        th3 = s.t3 + math.radians(fadj)
        if nrm is not None and k > 0:
            nd = Ry.transposed() @ (-nrm)
            th3 += k * clamp(wrap(math.atan2(nd.z, nd.dot(s.u)) + math.pi / 2), -1.0, 1.0)
        au, az = pu - s.l3 * math.cos(th3), pz - s.l3 * math.sin(th3)
        dd = math.hypot(au, az); reach = s.l1 + s.l2
        stretch = dd / reach
        dd = min(max(dd, abs(s.l1 - s.l2) + 1e-4), reach * 0.9995)
        ph = math.atan2(az, au)
        al = math.acos(clamp((s.l1 ** 2 + dd ** 2 - s.l2 ** 2) / (2 * s.l1 * dd), -1, 1))
        th1 = ph + al                  # knee-up solution
        kx, kz = s.l1 * math.cos(th1), s.l1 * math.sin(th1)
        th2 = math.atan2(dd * math.sin(ph) - kz, dd * math.cos(ph) - kx)
        p1 = wrap(th1 - s.t1); p2 = wrap(th2 - s.t2 - (th1 - s.t1)); p3 = wrap(th3 - s.t3 - (th2 - s.t2))
        st = STATS.setdefault(s.name, [0.0] * 5)
        for i, v in enumerate((yaw, p1, p2, p3)): st[i] = max(st[i], abs(math.degrees(v)))
        st[4] = max(st[4], stretch)
        Mf = T(s.H) @ Ry.to_4x4() @ Matrix.Rotation(p1, 4, s.n) @ T(-s.H)
        Mt = Mf @ T(s.K) @ Matrix.Rotation(p2, 4, s.n) @ T(-s.K)
        Mo = Mt @ T(s.A) @ Matrix.Rotation(p3, 4, s.n) @ T(-s.A)
        return Mf, Mt, Mo
LEG = {L: Leg(L) for L in LEGS}
for L in LEGS:
    g = LEG[L]; log("leg", L, "l1/l2/l3", round(g.l1, 3), round(g.l2, 3), round(g.l3, 3))

def rot_about(pivot, pry):
    """pry = (pitch nose-up, roll, yaw left) in degrees, about pivot."""
    p, r, y = (math.radians(v) for v in pry)
    R = Matrix.Rotation(y, 4, 'Z') @ Matrix.Rotation(r, 4, 'Y') @ Matrix.Rotation(-p, 4, 'X')
    return T(pivot) @ R @ T(-pivot)

PLATE_PIVOT = Vector((0, 0.07, 0))     # hinge plates near their rear edge
def evaluate(sp):
    Mr = sp["root"]; Mb = Mr @ sp["body"]; Mbi = Mb.inverted(); Rbi = Mbi.to_3x3()
    P = {"Root": Mr @ REST["Root"], "body": Mb @ REST["body"]}
    hp, hy = sp.get("head", (0, 0)); P["head"] = Mb @ rot_about(HEAD["head"], (hp, 0, hy)) @ REST["head"]
    ap, ay = sp.get("abd", (0, 0)); P["abdomen"] = Mb @ rot_about(HEAD["abdomen"], (ap, 0, ay)) @ REST["abdomen"]
    for nm, v in zip(("plate_front", "plate_mid", "plate_rear"), sp.get("plates", (0, 0, 0))):
        P[nm] = Mb @ rot_about(HEAD[nm] + PLATE_PIVOT, (v, 0, 0)) @ REST[nm]
    for nm in ("pod_L", "pod_R"):
        P[nm] = Mb @ rot_about(HEAD[nm], (sp.get("pods", 0), 0, 0)) @ REST[nm]
    for L, (Pw, nw, k, fa) in sp["feet"].items():
        nb = (Rbi @ nw).normalized() if nw is not None else None
        Mf, Mt, Mo = LEG[L].solve(Mbi @ Pw, nb, k, fa)
        P[L + "_hip"] = Mb @ Mf @ REST[L + "_hip"]
        P[L + "_knee"] = Mb @ Mt @ REST[L + "_knee"]
        P[L + "_ankle"] = Mb @ Mo @ REST[L + "_ankle"]
    return P

# ---------------------------------------------------------------- baking
MANIFEST = []
def bake(name, specs, loop, note, root_motion=False, speed=None):
    STATS.clear()
    act = bpy.data.actions.new(name); act.use_fake_user = True
    arm.animation_data.action = act
    prev = {}
    bones = list(arm.pose.bones)
    for pb in bones: pb.rotation_mode = 'QUATERNION'
    for f, sp in enumerate(specs):
        P = evaluate(sp)
        for pb in bones:
            b = pb.bone; m = P[b.name]
            if b.parent:
                basis = (REST[b.parent.name].inverted() @ REST[b.name]).inverted() @ P[b.parent.name].inverted() @ m
            else:
                basis = REST[b.name].inverted() @ m
            loc, q, _ = basis.decompose()
            if b.name in prev and q.dot(prev[b.name]) < 0: q.negate()
            prev[b.name] = q
            pb.location = loc; pb.rotation_quaternion = q; pb.scale = (1, 1, 1)
            pb.keyframe_insert("location", frame=f, group=b.name)
            pb.keyframe_insert("rotation_quaternion", frame=f, group=b.name)
    for fc in fcurves(act):
        for kp in fc.keyframe_points: kp.interpolation = 'LINEAR'
        if loop: fc.modifiers.new('CYCLES')     # repeats in Blender whatever the timeline length
    act.use_frame_range = True; act.frame_start = 0; act.frame_end = len(specs) - 1
    act.use_cyclic = loop
    arm.animation_data.action = None
    worst = max(STATS.items(), key=lambda kv: kv[1][4])
    joint = max(max(v[:4]) for v in STATS.values())
    log(f"baked {name:26s} 0-{len(specs)-1:3d} loop={loop} rm={root_motion} maxJoint={joint:5.1f}deg "
        f"maxStretch={worst[1][4]:.2f}({worst[0]})" + (f" speed={speed}" if speed else ""))
    MANIFEST.append(dict(name=name, frames=len(specs) - 1, loop=loop, root_motion=root_motion, speed=speed, note=note,
                         max_joint_deg=round(joint, 1), max_stretch=round(worst[1][4], 2)))

# ---------------------------------------------------------------- pose parameters
SCALARS = ["rx", "ry", "rz", "ra", "rr", "bx", "by", "bz", "bp", "br", "byaw", "pvy", "pvz",
           "hp", "hy", "ap", "ay", "pf", "pm", "pr", "pod"]
def default(nm):
    return Vector((0, 0, 0)) if nm.startswith("o_") else 0.0
DEF = {nm: 0.0 for nm in SCALARS}
for L in LEGS: DEF["o_" + L] = Vector((0, 0, 0)); DEF["w_" + L] = 0.0; DEF["fa_" + L] = 0.0
NEUTRAL = dict(DEF)

def spec_from(p):
    Mr = T((p["rx"], p["ry"], p["rz"])) @ Matrix.Rotation(math.radians(p["ra"]), 4, 'X') @ Matrix.Rotation(math.radians(p["rr"]), 4, 'Y')
    Mb = T((p["bx"], p["by"], p["bz"])) @ rot_about(Vector((0, p["pvy"], p["pvz"])), (p["bp"], p["br"], p["byaw"]))
    up = Mr.to_3x3() @ Z; feet = {}
    for L in LEGS:
        base = TIP[L] + Vector(p["o_" + L]); w = clamp(p["w_" + L])
        feet[L] = ((Mr @ base).lerp(Mr @ Mb @ base, w), up, 0.6 * (1 - w), p["fa_" + L])
    return dict(root=Mr, body=Mb, feet=feet, head=(p["hp"], p["hy"]), abd=(p["ap"], p["ay"]),
                plates=(p["pf"], p["pm"], p["pr"]), pods=p["pod"])

def H_track(pts, n):
    """Cubic Hermite through keys; flat tangents at ends and at local extrema (no overshoot on holds)."""
    pts = sorted(pts, key=lambda p: p[0]); fs = [p[0] for p in pts]; vs = [p[1] for p in pts]
    m = [0.0] * len(vs)
    for i in range(1, len(vs) - 1):
        if (vs[i] - vs[i - 1]) * (vs[i + 1] - vs[i]) > 0:
            m[i] = (vs[i + 1] - vs[i - 1]) / (fs[i + 1] - fs[i - 1])
    out = []; j = 0
    for f in range(n + 1):
        if f <= fs[0]: out.append(vs[0]); continue
        if f >= fs[-1]: out.append(vs[-1]); continue
        while fs[j + 1] < f: j += 1
        h = fs[j + 1] - fs[j]; t = (f - fs[j]) / h
        out.append((2*t**3 - 3*t**2 + 1) * vs[j] + (t**3 - 2*t**2 + t) * h * m[j]
                   + (-2*t**3 + 3*t**2) * vs[j + 1] + (t**3 - t**2) * h * m[j + 1])
    return out

def tracks(n, keys):
    names = set()
    for _, d in keys: names |= set(d)
    out = {}
    for nm in names:
        pts = [(f, d[nm]) for f, d in keys if nm in d]
        if pts[0][0] > 0: pts.insert(0, (0, default(nm)))
        if nm.startswith("o_"):
            comps = [H_track([(f, Vector(v)[i]) for f, v in pts], n) for i in range(3)]
            out[nm] = [Vector((comps[0][f], comps[1][f], comps[2][f])) for f in range(n + 1)]
        else:
            out[nm] = H_track(pts, n)
    return out

def keyed(n, keys, over=None):
    tr = tracks(n, keys); specs = []
    for f in range(n + 1):
        p = dict(DEF); p.update({nm: tr[nm][f] for nm in tr})
        if over: over(f, p)
        specs.append(spec_from(p))
    return specs

def proc(n, fn):
    specs = []
    for f in range(n + 1):
        p = dict(DEF); p.update(fn(f / n, f)); specs.append(spec_from(p))
    return specs

def at(names, pos):
    """Foot targets in root space, pos given for the left side and mirrored for the right."""
    return {"o_" + L: TIP[L].copy() * 0 + (mir(L, pos) - TIP[L]) for L in names}
def off(names, v): return {"o_" + L: mir(L, v) for L in names}
def wt(names, w): return {"w_" + L: w for L in names}

# ---------------------------------------------------------------- cyclic gaits (in place)
def gait(n, dirv, stride, lift, duty, phases, bz=0.0, bp=0.0, bob=0.0, bobn=2, bob_ph=0.0, pitch=0.0, pitch_ph=0.0,
         roll=0.0, turn=0.0, head=(0.0, 0.0), head_yaw=0.0, plates=(0, 0, 0), pods=0.0):
    specs = []
    for f in range(n + 1):
        u = (f % n) / n; feet = {}
        for L in LEGS:
            ph = (u + phases[L]) % 1.0
            if ph < duty:
                a = ph / duty; s = 0.5 - a; z = 0.0
            else:
                a = (ph - duty) / (1 - duty); s = -0.5 + smooth(a); z = lift * math.sin(math.pi * a)
            base = TIP[L]
            if turn: base = Matrix.Rotation(math.radians(turn) * s, 3, 'Z') @ base
            feet[L] = (base + dirv * stride * s + Z * z, Z, 0.6, 0.0)
        body = T((0, 0, bz + bob * math.cos(2 * math.pi * (bobn * u + bob_ph)))) @ rot_about(
            Vector(), (bp + pitch * math.sin(2 * math.pi * (u + pitch_ph)), roll * math.sin(2 * math.pi * u), 0))
        specs.append(dict(root=Matrix(), body=body, feet=feet,
                          head=(head[0], head[1] + head_yaw * math.sin(2 * math.pi * u)), plates=plates, pods=pods))
    return specs

def spd(stride, duty, n): return f"{stride / (duty * n / FPS):.2f} m/s"

TRI = {L: (0.0 if L in ("L_front", "R_mid", "L_rear") else 0.5) for L in LEGS}
WAVE = {L: -i / 6 for i, L in enumerate(["R_rear", "R_mid", "R_front", "L_rear", "L_mid", "L_front"])}
GAL = {L: {"front": 0.0, "mid": 0.14, "rear": 0.28}[L[2:]] + (0.03 if L[0] == "R" else 0.0) for L in LEGS}

G = [
    ("Walk_Fwd",      dict(n=32, dirv=FWD,   stride=0.28, lift=0.08, duty=0.55, phases=TRI, bob=0.008, roll=1.0, head_yaw=3)),
    ("Walk_Backup",   dict(n=32, dirv=-FWD,  stride=0.22, lift=0.07, duty=0.55, phases=TRI, bob=0.008, roll=1.0)),
    ("Strafe_L",      dict(n=28, dirv=LEFT,  stride=0.22, lift=0.07, duty=0.55, phases=TRI, bob=0.006, roll=2.0)),
    ("Strafe_R",      dict(n=28, dirv=-LEFT, stride=0.22, lift=0.07, duty=0.55, phases=TRI, bob=0.006, roll=2.0)),
    ("Run_Fwd",       dict(n=18, dirv=FWD,   stride=0.42, lift=0.11, duty=0.42, phases=TRI, bz=-0.02, bp=-3, bob=0.015, roll=1.5)),
    ("Sprint_Fwd",    dict(n=14, dirv=FWD,   stride=0.46, lift=0.12, duty=0.38, phases=TRI, bz=-0.045, bp=-6, bob=0.02,
                           roll=1.5, plates=(-3, -3, 0), head=(-4, 0))),
    ("Gallop_Fwd",    dict(n=14, dirv=FWD,   stride=0.48, lift=0.15, duty=0.32, phases=GAL, bob=0.045, bobn=1, bob_ph=0.48,
                           pitch=6, pitch_ph=0.35)),
    ("Stealth_Crawl", dict(n=72, dirv=FWD,   stride=0.22, lift=0.09, duty=5 / 6, phases=WAVE, bz=-0.08, bp=-3, bob=0.004,
                           head=(-8, 0), head_yaw=10, plates=(-2, -2, -2))),
]
for name, kw in G:
    n = kw.pop("n")
    bake(name, gait(n, **kw), True, "in-place gait", speed=spd(kw["stride"], kw["duty"], n))
for name, sign in (("Turn_L", 1), ("Turn_R", -1)):
    bake(name, gait(28, Vector(), 0.0, 0.07, 0.55, TRI, turn=28 * sign, bob=0.006), True, "in-place turn",
         speed=f"{28 / (0.55 * 28 / FPS):.0f} deg/s")

# ---------------------------------------------------------------- idles
bake("Idle", proc(72, lambda u, f: dict(
    bz=0.008 * math.sin(2 * math.pi * u), bp=0.6 * math.sin(2 * math.pi * u + 1), br=0.4 * math.sin(2 * math.pi * u + 2),
    hy=5 * math.sin(2 * math.pi * u), hp=2 * math.sin(4 * math.pi * u), pm=1.5 + 1.5 * math.sin(2 * math.pi * u),
    ap=1.5 * math.sin(2 * math.pi * u + 0.5))), True, "breathing idle")
bake("Idle_LookAround", keyed(120, [
    (0, {}), (18, dict(hy=32, byaw=5, br=-2, hp=4)), (42, dict(hy=34, byaw=6, br=-2, hp=-4)),
    (62, dict(hy=-30, byaw=-6, br=2, hp=2)), (90, dict(hy=-32, byaw=-5, br=2, hp=-3)), (120, dict(NEUTRAL))]),
    True, "head scan idle")
bake("Idle_Ceiling_Hang", proc(72, lambda u, f: dict(
    bz=0.05 + 0.01 * math.sin(2 * math.pi * u), bp=0.8 * math.sin(2 * math.pi * u),
    hy=6 * math.sin(2 * math.pi * u), hp=-6 + 2 * math.sin(4 * math.pi * u))),
    True, "use while attached to a ceiling (actor upside down): body sags away from the surface")

# ---------------------------------------------------------------- jump (in place, game drives the capsule)
AIR = {"o_" + L: mir(L, (0.05, 0.0, 0.07)) for L in LEGS}
AIRW = wt(LEGS, 1.0)
bake("Jump_Start", keyed(12, [
    (0, {}), (6, dict(bz=-0.09, bp=-3, pf=-4)), (9, dict(bz=0.06, bp=2, **wt(LEGS, 0.0))),
    (12, dict(bz=0.0, bp=3, pf=4, pm=4, **AIR, **AIRW))]), False, "crouch + push; capsule launches at frame 9")
bake("Jump_Fall", proc(24, lambda u, f: dict(
    bp=2 + 2 * math.sin(2 * math.pi * u), hp=4, pf=4, pm=4,
    **{"o_" + L: mir(L, (0.05, 0.02 * math.sin(2 * math.pi * (u + i / 6)), 0.07 + 0.025 * math.sin(2 * math.pi * (u + i / 6))))
       for i, L in enumerate(LEGS)}, **AIRW)), True, "airborne loop (also used after wall/ceiling drops)")
bake("Jump_Land", keyed(18, [
    (0, dict(bp=2, **{"o_" + L: mir(L, (0.04, 0, -0.04)) for L in LEGS}, **wt(LEGS, 0.4))),
    (2, dict(bz=-0.02, **off(LEGS, (0, 0, 0)), **wt(LEGS, 0.0))), (5, dict(bz=-0.11, bp=-3, pf=-3, hp=-6)),
    (10, dict(bz=-0.02, bp=1, hp=2)), (18, dict(NEUTRAL))]), False, "impact absorb; frame 0 = ground contact")
def jump_over(f, p):
    t0, t1, peak = 9, 31, 0.5
    if t0 <= f <= t1:
        x = (f - t0) / (t1 - t0); p["bz"] += 4 * peak * x * (1 - x)
bake("Jump_InPlace", keyed(42, [
    (0, {}), (6, dict(bz=-0.09, bp=-3)), (9, dict(bz=0.04, **wt(LEGS, 0.0))), (13, dict(bz=0.0, **AIR, **AIRW)),
    (27, dict(**AIR, **AIRW)), (31, dict(**off(LEGS, (0, 0, 0)), **wt(LEGS, 0.0))), (34, dict(bz=-0.11, bp=-3)),
    (42, dict(NEUTRAL))], over=jump_over), False, "full hop with body arc baked in (no root motion)")

# ---------------------------------------------------------------- grab / reach / stand
GZ = GROUND
HOLD = dict(bp=4, by=0.02, bz=0.02, hp=-10, **at(FRONT, (0.08, -0.66, GZ + 0.30)))
bake("Grab_Reach", keyed(30, [
    (0, {}), (6, dict(bp=-6, by=-0.04, bz=-0.03, **at(FRONT, (0.42, -0.70, GZ + 0.22)))),
    (14, dict(bp=-10, by=-0.07, bz=-0.05, hp=-8, **at(FRONT, (0.26, -0.88, GZ + 0.10)))),
    (20, dict(bp=-11, **at(FRONT, (0.08, -0.84, GZ + 0.07)))), (30, HOLD)]),
    False, "front legs grab an object ~0.8 m ahead at ground level and lift it; ends in Grab_Hold pose")
def hold_fn(u, f):
    p = dict(HOLD); p["by"] += 0.006 * math.sin(2 * math.pi * u); p["hy"] = 6 * math.sin(2 * math.pi * u)
    for i, L in enumerate(FRONT):
        p["o_" + L] = p["o_" + L] + Vector((0, 0, 0.015 * math.sin(2 * math.pi * u + i * math.pi)))
    return p
bake("Grab_Hold", proc(48, hold_fn), True, "holding loop")
bake("Grab_Release", keyed(20, [
    (0, HOLD), (6, dict(**at(FRONT, (0.24, -0.74, GZ + 0.25)), bp=0)), (13, dict(**at(FRONT, (0.50, -0.60, GZ + 0.10)))),
    (20, dict(NEUTRAL))]), False, "open claws and step back down")

bake("Reach_Up", keyed(48, [
    (0, {}), (12, dict(bp=18, pvy=0.18, bz=0.05, hp=8, **at(FRONT, (0.35, -0.62, GZ + 0.35)))),
    (20, dict(bp=28, bz=0.08, hp=15, pf=6, **at(FRONT, (0.26, -0.78, 0.45)))),
    (34, dict(bp=27, bz=0.08, hp=18, **at(FRONT, (0.24, -0.80, 0.50)))), (48, dict(NEUTRAL))]),
    False, "rear up on 4 legs and reach overhead with the front pair")

UP = dict(bp=55, pvy=0.18, bz=0.07, hp=-30, pf=18, pm=12, pod=-12,
          **off(MID, (0, 0.10, 0)), **off(FRONT, (-0.12, -0.20, 0.12)), **wt(FRONT, 1.0))
UP_MID = dict(bp=30, bz=0.04, **off(MID, (0, 0.05, 0.08)), **off(FRONT, (-0.06, -0.10, 0.10)), **wt(FRONT, 1.0))
bake("Stand_Upright_Start", keyed(30, [(0, {}), (14, UP_MID), (30, UP)]), False, "rear up onto rear + mid legs, front legs raised")
def up_fn(u, f):
    p = dict(UP); s = math.sin(2 * math.pi * u)
    p["bp"] += 2 * s; p["hy"] = 12 * math.sin(2 * math.pi * u + 0.7)
    for i, L in enumerate(FRONT):
        p["o_" + L] = p["o_" + L] + Vector((0, 0, 0.05 * math.sin(2 * math.pi * u + i * math.pi)))
    return p
bake("Stand_Upright_Loop", proc(48, up_fn), True, "upright threat loop, front legs pawing")
bake("Stand_Upright_End", keyed(30, [(0, UP), (16, UP_MID), (30, dict(NEUTRAL))]), False, "back down to all six")

# ---------------------------------------------------------------- combat-ish extras
bake("Attack_Stab", keyed(30, [
    (0, {}), (8, dict(bp=14, pvy=0.18, by=0.05, bz=0.04, hp=6, **at(FRONT, (0.30, -0.55, GZ + 0.45)))),
    (13, dict(bp=-10, by=-0.10, bz=-0.04, hp=-10, **at(FRONT, (0.18, -0.92, GZ)))),
    (17, dict(bp=-9, by=-0.09, bz=-0.04, **at(FRONT, (0.18, -0.92, GZ)))), (30, dict(NEUTRAL))]),
    False, "front legs rear back and stab down ahead")
bake("Hit_React", keyed(16, [
    (0, {}), (3, dict(by=0.06, bp=10, br=6, bz=0.02, hp=12, hy=-8)), (8, dict(by=0.02, bp=3, br=-2, hp=2)),
    (16, dict(NEUTRAL))]), False, "hit from the front")
DEAD_Z = GROUND - BELLY + 0.01
bake("Death", keyed(48, [
    (0, {}), (6, dict(bz=0.03, bp=6, hp=10)),
    (16, dict(bz=DEAD_Z, br=6, bp=-4, hp=-12, **off(LEGS, (0.08, 0, 0)))), (24, dict(bz=DEAD_Z, br=7, **off(LEGS, (0.10, 0, 0)))),
    (42, dict(bz=DEAD_Z, br=7, hp=-15, pf=-4, **off(LEGS, (-0.20, 0, 0.28)), **wt(LEGS, 1.0))), (48, {})]),
    False, "collapse, legs curl")

# ---------------------------------------------------------------- surface transitions (root motion)
class Plane:
    """clampf pulls a point lying on the plane back into the plane's valid (existing) region."""
    def __init__(s, p0, n, clampf): s.p0, s.n, s.clampf = Vector(p0), Vector(n).normalized(), clampf
def project(pt, planes):
    best = None
    for pl in planes:
        q = pl.clampf(pt - pl.n * (pt - pl.p0).dot(pl.n))
        if best is None or (pt - q).length < best[0]: best = ((pt - q).length, q, pl.n)
    return best[1], best[2]
def cl(q, **kw):
    q = q.copy()
    for ax, (lo, hi) in kw.items(): setattr(q, ax, min(hi, max(lo, getattr(q, ax))))
    return q
INF = 1e9

def tripod_windows(n, first=4, length=8):
    w = {L: [] for L in LEGS}; f = first; grp = 0
    while f + length <= n:
        for L in LEGS:
            if (TRI[L] == 0.0) == (grp == 0): w[L].append((f, f + length))
        f += length; grp ^= 1
    for L in LEGS:          # finish exactly on the last frame
        if w[L][-1][1] != n: w[L].append((max(w[L][-1][1], n - length), n))
    return w

def crawl(roots, bodies, planes, windows, lift=0.09, look=4, via=None, extras=None):
    n = len(roots) - 1
    ideal = lambda f, L: project(roots[f] @ bodies[f] @ TIP[L], planes)
    track = {}
    for L in LEGS:
        cur, cn = ideal(0, L); tr = [(cur, cn)] * (n + 1)
        ws = windows[L]
        for wi, (fs, fe) in enumerate(ws):
            tgt, tn = ideal(n if wi == len(ws) - 1 else min(fe + look, n), L)
            ctrl = None
            if via and cn.dot(tn) < 0.5:
                V = via(cur, tgt); ctrl = 2 * V - 0.5 * (cur + tgt)
            for f in range(fs, fe + 1):
                a = (f - fs) / (fe - fs); s = smooth(a)
                if ctrl is not None:
                    P = (1 - s) ** 2 * cur + 2 * (1 - s) * s * ctrl + s * s * tgt
                else:
                    P = cur.lerp(tgt, s) + (cn + tn).normalized() * lift * math.sin(math.pi * a)
                tr[f] = (P, cn.lerp(tn, s).normalized())
            for f in range(fe + 1, n + 1): tr[f] = (tgt, tn)
            cur, cn = tgt, tn
        track[L] = tr
    specs = []
    for f in range(n + 1):
        sp = dict(root=roots[f], body=bodies[f], feet={L: (track[L][f][0], track[L][f][1], 0.6, 0.0) for L in LEGS})
        if extras: sp.update(extras(f / n))
        specs.append(sp)
    return specs

def smooth_track(vals, passes=3):
    v = list(vals)
    for _ in range(passes):
        v = [v[0]] + [(v[i - 1] + 2 * v[i] + v[i + 1]) / 4 for i in range(1, len(v) - 1)] + [v[-1]]
    return v

def penetration(roots, bodies, solid):
    return max(solid(r @ b @ c) for r, b in zip(roots, bodies) for c in BOX)

# Inside corner (floor -> wall ahead). Wall-to-ceiling and wall-to-floor (head first) are the same move
# in the actor's local frame, so the same data is baked under each name.
def concave(n=48, Dw=0.85, zf=0.72, m=0.04):
    yf = -Dw + HB; ys, zs, As = [], [], []
    for f in range(n + 1):
        t = f / n; a = 90 * smooth((t - 0.12) / 0.63)
        R = Matrix.Rotation(math.radians(-a), 3, 'X')
        miny = min((R @ c).y for c in BOX); minz = min((R @ c).z for c in BOX)
        ys.append(max(yf * smooth(t), -Dw + m - miny)); zs.append(max(zf * smooth((t - 0.08) / 0.92), GROUND + m - minz)); As.append(a)
    ys, zs = smooth_track(ys), smooth_track(zs)
    roots = [T((0, y, z)) @ Matrix.Rotation(math.radians(-a), 4, 'X') for y, z, a in zip(ys, zs, As)]
    bodies = [Matrix()] * (n + 1)
    planes = [Plane((0, 0, GROUND), Z, lambda q: cl(q, y=(-Dw + 0.06, INF))), Plane((0, -Dw, 0), (0, 1, 0), lambda q: cl(q, z=(GROUND + 0.06, INF)))]
    pen = penetration(roots, bodies, lambda p: max(0.0, min(GROUND - p.z, 1)) if p.z < GROUND else max(0.0, -Dw - p.y))
    log(f"concave: wall {Dw} m ahead, ends {zf} m up, body penetration {pen:.3f}")
    return crawl(roots, bodies, planes, tripod_windows(n), extras=lambda t: dict(head=(18 * math.sin(math.pi * t), 0))), Dw, zf

cs, Dw, zf = concave()

# Every inside-corner crawl is the same move relative to the surface it starts on; S places it in the world
# so each action starts on the surface its name says (floor, wall or ceiling). Segments chain for floor<->ceiling.
QUADS = {}
def concave_quads(S):
    out = []
    for c, u, v in (((0, 0.4, GROUND), (1.5, 0, 0), (0, 1.25, 0)), ((0, -Dw, GROUND + 1.2), (1.5, 0, 0), (0, 0, 1.2))):
        c, u, v = Vector(c), Vector(u), Vector(v)
        out.append([tuple(S @ p) for p in (c - u - v, c + u - v, c + u + v, c - u + v)])
    return out
def xform(specs, S):
    R = S.to_3x3(); out = []
    for sp in specs:
        sp = dict(sp); sp["root"] = S @ sp["root"]
        sp["feet"] = {L: (S @ P, R @ n if n is not None else None, k, fa) for L, (P, n, k, fa) in sp["feet"].items()}
        out.append(sp)
    return out
def corner_chain(name, S, segs, note, ground_end=False):
    specs, quads, M = [], [], S
    for i in range(segs):
        part = xform(cs, M); quads += concave_quads(M)
        specs += part if i == 0 else part[1:]
        M = M @ cs[-1]["root"]
    if ground_end:      # shift so it finishes standing on the floor at the rest height
        return corner_chain(name, T((0, 0, -specs[-1]["root"].translation.z)) @ S, segs, note)
    QUADS[name] = quads
    bake(name, specs, False, "root motion; crawl, starts in world orientation. " + note, root_motion=True)
RX = lambda d: Matrix.Rotation(math.radians(d), 4, 'X')
RY = lambda d: Matrix.Rotation(math.radians(d), 4, 'Y')
corner_chain("Trans_Floor_To_Wall", Matrix(), 1, f"wall {Dw} m ahead; ends {zf} m up the wall")
corner_chain("Trans_Wall_To_Ceiling", T((0, 0, 0.6)) @ RX(-90), 1, f"starts head-up on a wall, ceiling {Dw} m above the root; ends upside down")
corner_chain("Trans_Floor_To_Ceiling", Matrix(), 2, f"floor -> wall -> ceiling in one crawl; wall {Dw} m ahead")
corner_chain("Trans_Ceiling_To_Wall", T((0, 0, 1.6)) @ RY(180), 1, f"starts upside down, wall {Dw} m ahead; crawls head-first down the wall")
corner_chain("Trans_Wall_To_Floor", RX(90), 1, f"starts head-down on a wall {Dw} m above the floor; crawls onto the floor", ground_end=True)
corner_chain("Trans_Ceiling_To_Floor", RY(180), 2, f"ceiling -> wall -> floor in one crawl, head first", ground_end=True)

def convex(n=48, E=0.62, cf=0.65):
    corner = Vector((0, -E, GROUND)); roots = []
    for f in range(n + 1):
        c = -E + (cf + E) * smooth(f / n)
        a = math.radians(90 * smooth((c + 0.35) / 0.7))
        up = Vector((0, -math.sin(a), math.cos(a))); fw = Vector((0, -math.cos(a), -math.sin(a)))
        roots.append(T(corner + HB * up + c * fw) @ Matrix.Rotation(a, 4, 'X'))
    bodies = [Matrix()] * (n + 1)
    planes = [Plane((0, 0, GROUND), Z, lambda q: cl(q, y=(-E + 0.05, INF))), Plane((0, -E, 0), (0, -1, 0), lambda q: cl(q, z=(-INF, GROUND - 0.05)))]
    via = lambda a, b: Vector(((a.x + b.x) / 2, -E, GROUND)) + Vector((0, -1, 1)).normalized() * 0.12
    pen = penetration(roots, bodies, lambda p: max(0.0, min(p.y + E, GROUND - p.z)))
    log(f"convex: edge {E} m ahead, ends {cf} m below, body penetration {pen:.3f}")
    return crawl(roots, bodies, planes, tripod_windows(n), via=via, extras=lambda t: dict(head=(-15 * math.sin(math.pi * t), 0))), E, cf
cv, E, cf = convex()
bake("Trans_Over_Ledge", cv, False, f"root motion; outside corner: crawl over a ledge {E} m ahead and down the face "
     f"(also ceiling->wall around an outer edge); ends {cf} m below the edge, pitched 90 deg", root_motion=True)

# ---------------------------------------------------------------- leaps off walls / ceilings (root motion)
GRAV = 9.81
def leap(n_pre, launch_v, gdir, fall_frames, rot_fn, land_plane_n, absorb=16, tuck=AIR):
    """Root starts on its surface (local up = surface normal). gdir = gravity in that local frame."""
    fl = n_pre + fall_frames; n = fl + absorb
    def root_at(f):
        t = max(0, f - n_pre) / FPS if f <= fl else fall_frames / FPS
        p = launch_v * t + 0.5 * GRAV * t * t * gdir
        return T(p) @ rot_fn(clamp((f - n_pre) / (fall_frames - 1)))
    roots = [root_at(f) for f in range(n + 1)]
    pland = roots[fl].translation
    # landing plane passes HB below the landing root, along its local -Z
    floor_p0 = roots[fl] @ Vector((0, 0, GROUND)); nrm = Vector(land_plane_n)
    land = {L: project(roots[fl] @ TIP[L], [Plane(floor_p0, nrm, lambda q: q)])[0] for L in LEGS}
    tk = tracks(n, [(0, {}), (n_pre - 3, dict(bz=-0.06, bp=-3)), (n_pre, dict(bz=0.05, bp=2)), (n_pre + 4, dict(bz=0.0, bp=4, hp=6)),
                    (fl, dict(bz=0.0, bp=0, hp=0)), (fl + 3, dict(bz=-0.11, bp=-3, hp=-6)), (fl + 9, dict(bz=-0.02, bp=1)), (n, dict(NEUTRAL))])
    specs = []
    for f in range(n + 1):
        Mr = roots[f]; Mb = T((0, 0, tk["bz"][f])) @ rot_about(Vector(), (tk["bp"][f], 0, 0)); feet = {}
        for L in LEGS:
            wall = TIP[L]; air = Mr @ Mb @ (TIP[L] + Vector(tuck["o_" + L]))
            if f <= n_pre: P, nn, k = wall, Z, 0.6
            elif f < n_pre + 4: P, nn, k = wall.lerp(air, smooth((f - n_pre) / 4)), Z, 0.0
            elif f < fl - 4: P, nn, k = air, None, 0.0
            elif f < fl: P, nn, k = air.lerp(land[L], smooth((f - fl + 4) / 4)), nrm, 0.3
            else: P, nn, k = land[L], nrm, 0.6
            feet[L] = (P, nn, k, 0.0)
        specs.append(dict(root=Mr, body=Mb, feet=feet, head=(tk["hp"][f], 0)))
    return specs, pland, fl

# Head-down on a wall (local -Y = world down): push off, nose swings out, land on the floor facing away.
ws, wl, wfl = leap(8, Vector((0, 0, 2.0)), Vector((0, -1, 0)), 11,
                   lambda s: Matrix.Rotation(math.radians(-90 * smooth(s)), 4, 'X'), (0, 1, 0))
bake("Jump_Wall_To_Floor", ws, False,
     f"root motion; starts head-DOWN on a wall; lands at frame {wfl} on a floor {(-wl.y + HB):.2f} m below the root, "
     f"{wl.z:.2f} m out from the wall, facing away", root_motion=True)
# Head-up on a wall: let go, fall back, rotate feet-down, land facing the wall.
hs, hl, hfl = leap(8, Vector((0, 0, 1.2)), Vector((0, 1, 0)), 11,
                   lambda s: Matrix.Rotation(math.radians(90 * smooth(s)), 4, 'X'), (0, -1, 0))
bake("Drop_Wall_To_Floor", hs, False,
     f"root motion; starts head-UP on a wall; pushes off backwards, lands at frame {hfl} facing the wall, "
     f"{(hl.y + HB):.2f} m below the root", root_motion=True)
# Upside down on a ceiling (local +Z = world down): release, half-roll, land.
cs2, cl, cfl = leap(6, Vector((0, 0, 0.3)), Vector((0, 0, 1)), 14,
                    lambda s: Matrix.Rotation(math.radians(180 * smooth(s)), 4, 'Y'), (0, 0, -1))
bake("Drop_Ceiling_To_Floor", cs2, False,
     f"root motion; starts upside down on a ceiling; half-rolls and lands at frame {cfl}, "
     f"{(cl.z + HB):.2f} m below the root (ceiling ~{cl.z + 2 * HB:.2f} m above the floor)", root_motion=True)

# ---------------------------------------------------------------- locked-leg gaits (walk while holding / reared up)
TROT4 = {"L_mid": 0.0, "R_rear": 0.0, "R_mid": 0.5, "L_rear": 0.5}          # diagonal pairs on the 4 walking legs
def held_gait(n, dirv, stride, lift, duty, pose, locked):
    """Mid+rear legs walk in place under `pose` (a keyed-param dict); `locked` legs keep the pose's targets."""
    base = dict(DEF); base.update(pose)
    ref = spec_from(base)
    specs = []
    for f in range(n + 1):
        u = (f % n) / n
        p = dict(base)
        p["bz"] = base["bz"] + 0.006 * math.cos(4 * math.pi * u)
        p["br"] = base.get("br", 0.0) + 1.0 * math.sin(2 * math.pi * u)
        sp = spec_from(p)
        for L in LEGS:
            if L in locked: continue
            ph = (u + TROT4[L]) % 1.0
            if ph < duty: a = ph / duty; s = 0.5 - a; z = 0.0
            else: a = (ph - duty) / (1 - duty); s = -0.5 + smooth(a); z = lift * math.sin(math.pi * a)
            home = ref["feet"][L][0]
            sp["feet"][L] = (home + dirv * stride * s + Z * z, Z, 0.6, 0.0)
        specs.append(sp)
    return specs
for name, dirv in (("GrabWalk_Fwd", FWD), ("GrabWalk_Back", -FWD), ("GrabStrafe_L", LEFT), ("GrabStrafe_R", -LEFT)):
    bake(name, held_gait(32, dirv, 0.20, 0.06, 0.6, HOLD, FRONT), True, "walk while holding with the front legs", speed=spd(0.20, 0.6, 32))
for name, dirv in (("Upright_Walk_Fwd", FWD), ("Upright_Walk_Back", -FWD)):
    bake(name, held_gait(36, dirv, 0.14, 0.05, 0.65, UP, FRONT), True, "shuffle while reared up", speed=spd(0.14, 0.65, 36))

# ---------------------------------------------------------------- attacks from the reared-up pose
COCK = dict(bp=45, pvy=0.18, bz=0.02, by=0.08, hp=-20, **off(FRONT, (-0.10, 0.05, 0.18)), **wt(FRONT, 1.0),
            **off(MID, (0, 0.10, 0)), **wt(MID, 0.0), **wt(REAR, 0.0))
bake("Upright_Pounce", keyed(40, [
    (0, UP), (8, COCK),
    (13, dict(bp=15, by=-0.05, bz=0.12, hp=-5, **off(FRONT, (0.05, -0.35, 0.05)), **wt(FRONT, 1.0), **off(MID, (0, 0, 0)))),
    (16, dict(bp=5, by=-0.05, bz=0.15, **off(FRONT, (0.05, -0.30, -0.05)), **wt(FRONT, 1.0),
              **{"o_" + L: mir(L, (0.05, 0.0, 0.07)) for L in MID + REAR}, **wt(MID + REAR, 1.0))),
    (24, dict(bp=-8, by=-0.02, bz=0.05, hp=4, **at(FRONT, (0.22, -0.78, GZ)), **wt(FRONT, 0.0))),
    (28, dict(bp=-6, by=0.0, bz=-0.10, hp=-6, **off(FRONT, (0, -0.12, 0)), **off(MID + REAR, (0, 0, 0)), **wt(LEGS, 0.0), pf=0, pm=0, pod=0)),
    (40, dict(NEUTRAL))]),
    False, "from Stand_Upright_Loop: sink back, leap forward claws first, land on all six (game launches the capsule at f13)")
bake("Upright_Buck", keyed(34, [
    (0, UP), (10, dict(bp=70, bz=0.14, hp=-40, pf=25, pm=20, **off(FRONT, (0.20, -0.05, 0.40)), **wt(FRONT, 1.0))),
    (16, dict(bp=-6, by=-0.10, bz=-0.06, hp=-8, **at(FRONT, (0.25, -0.85, GZ)), **wt(FRONT, 0.0))),
    (20, dict(bp=-5, by=-0.09, bz=-0.05, **at(FRONT, (0.25, -0.85, GZ)), **wt(FRONT, 0.0))),
    (34, UP)]),
    False, "from Stand_Upright_Loop: rear higher and slam the front claws down; ends back in the upright pose")

# ---------------------------------------------------------------- save
arm.animation_data.action = bpy.data.actions["Walk_Fwd"]
sc.frame_start, sc.frame_end = 0, 32
bpy.ops.wm.save_as_mainfile(filepath=OUT)
json.dump(dict(fps=FPS, ground_z=GROUND, actions=MANIFEST, preview_quads=QUADS), open(os.path.join(W, "anim_manifest_v2.json"), "w"), indent=1)
log("SAVED", OUT, "actions:", len(bpy.data.actions))
open(os.path.join(W, "build_log_v2.txt"), "w").write("\n".join(LOG))
