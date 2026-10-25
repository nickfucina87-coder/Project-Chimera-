import bpy, os, math, shutil, datetime, traceback
from mathutils import Vector, Matrix
W=r"C:\Users\nickf\Documents\Chimera_Hexapod_Anim"
SRC=os.path.join(W,"Ironclad_Hexapod_v2_Skinned_src.glb")
BLEND=os.path.join(W,"Ironclad_Hexapod_Anim.blend")
LOG=[]
def log(*a):
    s=" ".join(str(x) for x in a); print("[HEX]",s); LOG.append(s)
def backup(tag):
    bpy.ops.wm.save_as_mainfile(filepath=BLEND)
    ts=datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    d=os.path.join(W,"backups"); os.makedirs(d,exist_ok=True)
    shutil.copy2(BLEND,os.path.join(d,f"Ironclad_Hexapod_Anim_{tag}_{ts}.blend")); log("saved",tag)
def fcurves(act,obj=None):
    if hasattr(act,"fcurves"):
        try: return list(act.fcurves)
        except Exception: pass
    out=[]
    for l in act.layers:
        for s in l.strips:
            for cb in s.channelbags: out+=list(cb.fcurves)
    return out

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=SRC)
sc=bpy.context.scene; sc.render.fps=24
arm=[o for o in sc.objects if o.type=='ARMATURE'][0]
log("armature",arm.name,"bones",len(arm.data.bones))
log("bones:",[b.name for b in arm.data.bones])
log("actions:",[(a.name,tuple(a.frame_range)) for a in bpy.data.actions])
backup("01_import")

bn={b.name.lower():b.name for b in arm.data.bones}
def find(*keys):
    for l,n in bn.items():
        if all(k in l for k in keys): return n
    return None
rootb=find("root"); bodyb=find("body"); headb=find("head")
LEGS={}
for side in ("l","r"):
    for pos in ("front","mid","rear"):
        an=None
        for l,n in bn.items():
            if "ankle" in l and pos in l:
                toks=l.replace(".","_").replace("-","_").split("_")
                if side in toks or l.endswith("."+side) or l.startswith(side+"_"): an=n
        if an: LEGS[(side.upper(),pos)]=an
log("root",rootb,"body",bodyb,"head",headb,"legs",LEGS)
assert len(LEGS)==6, "leg detection failed"

# rest-pose evaluation
orig_walk=[a for a in bpy.data.actions][0] if bpy.data.actions else None
for a in bpy.data.actions:
    if "walk" in a.name.lower(): orig_walk=a
arm.animation_data_create()
arm.animation_data.action=None
for pb in arm.pose.bones: pb.matrix_basis=Matrix()
bpy.context.view_layer.update()
MW=arm.matrix_world
fwd_list=[];info={}
for k,an in LEGS.items():
    pa=arm.pose.bones[an]; knee=pa.parent; hip=knee.parent
    tip=MW@pa.tail; kn=MW@knee.head; hp=MW@hip.head
    info[k]=dict(tip=tip.copy(),knee=kn.copy(),hip=hp.copy(),L=(tip-hp).length)
front=sum((info[(s,"front")]["tip"] for s in "LR"),Vector())/2
rear=sum((info[(s,"rear")]["tip"] for s in "LR"),Vector())/2
fwd=(front-rear); fwd.z=0; fwd.normalize()
up=Vector((0,0,1)); right=fwd.cross(up).normalized()
# make sure L side is actually left
if (info[("L","mid")]["tip"]-info[("R","mid")]["tip"]).dot(right)>0: right=-right if False else right
left=-right
if (info[("L","mid")]["tip"]-info[("R","mid")]["tip"]).dot(left)<0: log("WARN: L/R appear swapped relative to computed left; using bone names as truth"); left=-left; right=-right
LL=sum(v["L"] for v in info.values())/6
log("fwd",fwd,"left",left,"legLen",LL)

# 2) IK controls
col=bpy.data.collections.new("CTRL_IK"); sc.collection.children.link(col)
CT={}
for (s,p),an in LEGS.items():
    t=bpy.data.objects.new(f"CTRL_IK_{s}_{p}",None); t.empty_display_type='SPHERE'; t.empty_display_size=LL*0.08
    pl=bpy.data.objects.new(f"CTRL_POLE_{s}_{p}",None); pl.empty_display_type='CONE'; pl.empty_display_size=LL*0.06
    for o in (t,pl):
        col.objects.link(o); o.parent=arm
        if rootb: o.parent_type='BONE'; o.parent_bone=rootb
    bpy.context.view_layer.update()
    side=left if s=="L" else right
    t.matrix_world=Matrix.Translation(info[(s,p)]["tip"])
    pl.matrix_world=Matrix.Translation(info[(s,p)]["knee"]+side*LL*0.6+up*LL*0.3)
    c=arm.pose.bones[an].constraints.new('IK'); c.name="IK_"+s+"_"+p
    c.target=t; c.pole_target=pl; c.chain_count=3; c.use_tail=True
    c.influence=0.0
    CT[(s,p)]=(t,pl,c)
bpy.context.view_layer.update()
# auto pole angle: try angles, keep one where knee moves least
def knee_err(an,ang):
    c=CT_by[an]; c.influence=1; c.pole_angle=ang; bpy.context.view_layer.update()
    pb=arm.pose.bones[an].parent; e=((MW@pb.head)-kref[an]).length+((MW@pb.tail)-kref2[an]).length
    return e
CT_by={an:CT[k][2] for k,an in LEGS.items()}
kref={an:MW@arm.pose.bones[an].parent.head for an in LEGS.values()}
kref2={an:MW@arm.pose.bones[an].parent.tail for an in LEGS.values()}
for k,an in LEGS.items():
    best=min((knee_err(an,math.radians(a)),a) for a in range(-180,180,5))
    CT_by[an].pole_angle=math.radians(best[1]); CT_by[an].influence=0
    log("pole",k,"angle",best[1],"err",round(best[0],4))
bpy.context.view_layer.update()
backup("02_ik")

# 3) head damping
if orig_walk:
    o=orig_walk.copy(); o.name=orig_walk.name+"_ORIG"; o.use_fake_user=True
    orig_walk.use_fake_user=True
    n=0
    if headb:
        for fc in fcurves(orig_walk):
            if f'"{headb}"' in fc.data_path and ("rotation" in fc.data_path or "location" in fc.data_path):
                pts=fc.keyframe_points
                if not len(pts): continue
                rest=1.0 if (fc.data_path.endswith("rotation_quaternion") and fc.array_index==0) else 0.0
                f=0.55
                for kp in pts:
                    for attr in ("co","handle_left","handle_right"):
                        v=getattr(kp,attr); v[1]=rest+(v[1]-rest)*f; setattr(kp,attr,v)
                fc.update(); n+=1
    log("head curves damped",n,"in",orig_walk.name)
    # key IK influence 0 into walk so IK doesn't override FK
    arm.animation_data.action=orig_walk
    fr=orig_walk.frame_range
    for c in CT_by.values():
        c.influence=0
        for f in (fr[0],fr[1]): arm.pose.bones[[a for a in LEGS.values() if CT_by[a]==c][0]].constraints[c.name].keyframe_insert("influence",frame=f)
    arm.animation_data.action=None
backup("03_head")

# 4) gaits
TRI_A={("L","front"),("R","mid"),("L","rear")}
def gait(name,frames,direction,stride,lift,duty,phases,bob,sway=0.0):
    log("gait",name)
    for pb in arm.pose.bones: pb.matrix_basis=Matrix()
    # empties action per gait
    for (s,p),(t,pl,c) in CT.items():
        t.animation_data_create(); t.animation_data.action=bpy.data.actions.new(f"CTRL_{name}_{s}_{p}")
        t.animation_data.action.use_fake_user=True
    for c in CT_by.values(): c.influence=1
    caps=[]
    bodyrest=arm.pose.bones[bodyb].bone.matrix_local.copy() if bodyb else None
    for f in range(frames+1):
        u=(f%frames)/frames
        for (s,p),(t,pl,c) in CT.items():
            ph=(u+phases[(s,p)])%1.0
            if ph<duty:
                a=ph/duty; off=direction*stride*(0.5-a); z=0
            else:
                a=(ph-duty)/(1-duty); off=direction*stride*(-0.5+a); z=lift*math.sin(math.pi*a)
            t.matrix_world=Matrix.Translation(info[(s,p)]["tip"]+off+up*z)
            bpy.context.view_layer.update()
            t.keyframe_insert("location",frame=f)
        sc.frame_set(f)
        if bodyb:
            pb=arm.pose.bones[bodyb]
            dz=bob*math.sin(2*math.pi*u*2); dx=sway*math.sin(2*math.pi*u)
            wdelta=(up*dz+left*dx)
            ldelta=(MW.to_3x3().inverted()@wdelta)
            pb.matrix=Matrix.Translation(ldelta)@bodyrest
            bpy.context.view_layer.update()
        caps.append({pb.name:pb.matrix.copy() for pb in arm.pose.bones})
    # bake to FK action
    for c in CT_by.values(): c.influence=0
    for (s,p),(t,pl,c) in CT.items(): t.animation_data.action=None
    for (s,p),(t,pl,c) in CT.items():
        t.matrix_world=Matrix.Translation(info[(s,p)]["tip"])
    act=bpy.data.actions.new(name); act.use_fake_user=True
    arm.animation_data.action=act
    for f,cap in enumerate(caps):
        for pb in arm.pose.bones:
            b=pb.bone; m=cap[pb.name]
            if b.parent:
                rl=b.parent.matrix_local.inverted()@b.matrix_local
                basis=rl.inverted()@cap[b.parent.name].inverted()@m
                basis=(b.parent.matrix_local.inverted()@b.matrix_local).inverted()@(cap[b.parent.name].inverted()@m)
            else:
                basis=b.matrix_local.inverted()@m
            pb.rotation_mode='QUATERNION'; pb.matrix_basis=basis
            g=pb.name
            pb.keyframe_insert("location",frame=f,group=g); pb.keyframe_insert("rotation_quaternion",frame=f,group=g)
    for an,c in CT_by.items():
        arm.pose.bones[an].constraints[c.name].keyframe_insert("influence",frame=0)
    for fc in fcurves(act):
        for kp in fc.keyframe_points: kp.interpolation='LINEAR'
    arm.animation_data.action=None
    for pb in arm.pose.bones: pb.matrix_basis=Matrix()
    log("baked",name,"0-",frames)
    return act

tri={k:(0.0 if k in TRI_A else 0.5) for k in LEGS}
row={"front":0.0,"mid":0.2,"rear":0.4}
gal={(s,p):row[p]+(0.05 if s=="R" else 0) for (s,p) in LEGS}
made=[]
for args in [
    ("Sprint_Gallop",16,fwd,LL*0.9,LL*0.35,0.4,gal,LL*0.06),
    ("Strafe_L",28,left,LL*0.4,LL*0.22,0.5,tri,LL*0.02,LL*0.03),
    ("Strafe_R",28,right,LL*0.4,LL*0.22,0.5,tri,LL*0.02,LL*0.03),
    ("Walk_Backup",32,-fwd,LL*0.4,LL*0.2,0.5,tri,LL*0.02),
]:
    try: made.append(gait(*args).name)
    except Exception: log("FAIL",args[0],traceback.format_exc())
if orig_walk: arm.animation_data.action=orig_walk
backup("04_gaits")

# 5) export
bpy.ops.object.select_all(action='DESELECT')
for o in sc.objects:
    if o.type in ('ARMATURE','MESH'): o.select_set(True)
bpy.context.view_layer.objects.active=arm
try:
    bpy.ops.export_scene.fbx(filepath=os.path.join(W,"Ironclad_Hexapod_Anims_v1.fbx"),use_selection=True,
        object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=True,bake_anim_use_all_actions=True,
        bake_anim_use_nla_strips=False,bake_anim_force_startend_keying=True,apply_scale_options='FBX_SCALE_ALL',
        axis_forward='-Z',axis_up='Y',mesh_smooth_type='FACE',use_armature_deform_only=False)
    log("FBX ok")
except Exception: log("FBX FAIL",traceback.format_exc())

# 6) renders
try:
    sc.render.engine='BLENDER_WORKBENCH'; sc.render.resolution_x=960; sc.render.resolution_y=540
    cam=bpy.data.objects.new("PreviewCam",bpy.data.cameras.new("PreviewCam")); sc.collection.objects.link(cam); sc.camera=cam
    ctr=sum((v["tip"] for v in info.values()),Vector())/6
    cam.location=ctr-fwd*LL*3.5+left*LL*3.5+up*LL*2.5
    cam.rotation_euler=(ctr-cam.location).to_track_quat('-Z','Y').to_euler()
    for nm,fr in [(orig_walk.name if orig_walk else None,7),("Sprint_Gallop",4),("Strafe_L",7)]:
        if not nm: continue
        arm.animation_data.action=bpy.data.actions[nm]; sc.frame_set(fr)
        sc.render.filepath=os.path.join(W,f"preview_{nm}_f{fr}.png"); bpy.ops.render.render(write_still=True); log("render",nm)
    arm.animation_data.action=orig_walk
except Exception: log("RENDER FAIL",traceback.format_exc())
bpy.ops.wm.save_as_mainfile(filepath=BLEND)
log("FINAL actions:",[(a.name,tuple(a.frame_range)) for a in bpy.data.actions])
open(os.path.join(W,"build_log.txt"),"w").write("\n".join(LOG))
