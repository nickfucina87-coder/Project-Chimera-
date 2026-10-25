# Chimera deep elevator build script
import bpy, math
from mathutils import Vector
P="10 DEEP ELEVATOR | B4 SECRET LEVEL"
cx,cy=-41.0,26.0
B=-82.0
old=bpy.data.collections.get(P)
if old:
    for o in list(old.all_objects): bpy.data.objects.remove(o,do_unlink=True)
    for child in list(old.children): bpy.data.collections.remove(child)
    bpy.data.collections.remove(old)
root=bpy.data.collections.new(P); bpy.context.scene.collection.children.link(root)
def grp(n):
    c=bpy.data.collections.new(n); root.children.link(c); return c
surf=grp("10A | surface portal"); shaft=grp("10B | deep shaft"); car=grp("10C | travel car")
hall=grp("10D | B4 corridor"); arc=grp("10E | hidden archive"); ctrl=grp("10F | hidden control")
def mat(n,c,metal=0,rough=.55,emit=None,power=0):
    m=bpy.data.materials.get(n) or bpy.data.materials.new(n)
    m.diffuse_color=(*c,1); m.use_nodes=True
    bs=next(v for v in m.node_tree.nodes if v.type=="BSDF_PRINCIPLED")
    bs.inputs["Base Color"].default_value=(*c,1); bs.inputs["Metallic"].default_value=metal
    bs.inputs["Roughness"].default_value=rough
    if emit:
        bs.inputs["Emission Color"].default_value=(*emit,1)
        bs.inputs["Emission Strength"].default_value=power
    return m
C=mat("DEEP | cast concrete",(.17,.19,.20),rough=.85)
D=mat("DEEP | damp concrete",(.075,.09,.10),rough=.9)
S=mat("DEEP | structural steel",(.035,.055,.07),.76,.32)
N=mat("DEEP | brushed stainless",(.39,.43,.46),.82,.28)
G=mat("DEEP | gunmetal",(.11,.13,.145),.7,.38)
R=mat("DEEP | rubber",(.018,.024,.028),rough=.9)
T=mat("DEEP | raised tread",(.19,.22,.23),.62,.44)
A=mat("DEEP | safety amber",(.94,.5,.035),.25,.4)
X=mat("DEEP | warning red",(.52,.025,.02),.3,.38)
V=mat("DEEP | smoked glass",(.022,.09,.11),.18,.18)
W=mat("DEEP | stencil white",(.7,.74,.72),rough=.42)
LC=mat("DEEP | cyan light",(.02,.4,.5),.1,.25,(.03,.85,1),4)
LA=mat("DEEP | amber light",(.8,.32,.02),.1,.3,(1,.4,.02),4)
LR=mat("DEEP | red light",(.7,.015,.01),.1,.3,(1,.02,.005),4)
LW=mat("DEEP | white light",(.7,.85,.87),0,.3,(.65,.9,1),4)
def put(o,col,m=None,bevel=0):
    for c in list(o.users_collection): c.objects.unlink(o)
    col.objects.link(o)
    if m: o.data.materials.append(m)
    if bevel:
        b=o.modifiers.new("machined edges","BEVEL"); b.width=bevel; b.segments=2
        o.modifiers.new("weighted normals","WEIGHTED_NORMAL")
    return o
def box(n,at,size,m,col,b=.02,parent=None):
    bpy.ops.mesh.primitive_cube_add(size=1,location=at)
    o=bpy.context.object; o.name=n; o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    put(o,col,m,b)
    if parent:
        o.parent=parent; o.matrix_parent_inverse=parent.matrix_world.inverted()
    return o
def cyl(n,at,r,h,m,col,rot=(0,0,0),parent=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=r,depth=h,location=at,rotation=rot)
    o=bpy.context.object; o.name=n; put(o,col,m,.006)
    if parent:
        o.parent=parent; o.matrix_parent_inverse=parent.matrix_world.inverted()
    return o
def txt(n,s,at,sz,m,col,rot=(math.pi/2,0,0),parent=None):
    c=bpy.data.curves.new(n,"FONT"); c.body=s; c.size=sz; c.extrude=.002; c.bevel_depth=.0008
    o=bpy.data.objects.new(n,c); col.objects.link(o); o.location=at; o.rotation_euler=rot; c.materials.append(m)
    if parent:
        o.parent=parent; o.matrix_parent_inverse=parent.matrix_world.inverted()
    return o
def lamp(n,at,color,power,col):
    d=bpy.data.lights.new(n,"POINT"); d.energy=power; d.color=color; d.shadow_soft_size=1.0
    o=bpy.data.objects.new(n,d); col.objects.link(o); o.location=at
# Cut the existing warehouse floors and foundation exactly where the lift drops.
for name in ("120 x 72 m raft foundation","MAINTENANCE 17 floor","UPPER MAINTENANCE 17 floor"):
    slab=bpy.data.objects.get(name)
    if not slab: raise RuntimeError("Missing "+name)
    cut=box("TEMP | shaft cutter",(cx,cy,-40),(3.18,3.18,105),R,root,0)
    mod=slab.modifiers.new("Deep elevator floor opening","BOOLEAN")
    mod.operation="DIFFERENCE"; mod.solver="EXACT"; mod.object=cut
    bpy.context.view_layer.objects.active=slab
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cut,do_unlink=True)
for name in ("ELEVATOR shaft","ELEVATOR door","ELEVATOR door.001"):
    o=bpy.data.objects.get(name)
    if o: bpy.data.objects.remove(o,do_unlink=True)
zlo,zhi=-85,9.6
for side,x in (("west",cx-1.85),("east",cx+1.85)):
    box("SHAFT | "+side+" wall",(x,cy,(zlo+zhi)/2),(.32,4.02,zhi-zlo),D,shaft,.025)
box("SHAFT | north wall",(cx,cy+1.85,(zlo+zhi)/2),(4.02,.32,zhi-zlo),D,shaft,.025)
for lo,hi in ((-85,-83.3),(-80.2,-.15),(3.45,4.43),(7.85,9.6)):
    box("SHAFT | south wall",(cx,cy-1.85,(lo+hi)/2),(4.02,.32,hi-lo),D,shaft,.025)
for z,tag in ((0,"G"),(4.43,"M"),(B,"B4")):
    col=surf if z>=0 else hall
    for x in (cx-1.35,cx+1.35):
        box("PORTAL | "+tag+" jamb",(x,cy-1.86,z+1.65),(.11,.16,3.3),N,col,.012)
    box("PORTAL | "+tag+" header",(cx,cy-1.86,z+3.35),(2.82,.2,.22),G,col,.012)
    box("PORTAL | "+tag+" sill",(cx,cy-1.85,z+.035),(2.76,.56,.07),T,col,.006)
    txt("PORTAL | "+tag+" level",tag+"  /  "+str(round(z))+" m",(cx-1.2,cy-2.03,z+3.65),.22,W,col)
    box("PORTAL | "+tag+" call housing",(cx+1.65,cy-2.045,z+1.45),(.28,.11,.68),G,col)
    cyl("PORTAL | "+tag+" call lamp",(cx+1.65,cy-2.11,z+1.48),.075,.022,LC,col,(math.pi/2,0,0))
# Shaft guide rails, 8 m service bays, rungs and cabling establish the long descent.
for depth in range(-80,9,8):
    z=float(depth)
    for x in (cx-1.57,cx+1.57):
        box("SHAFT | rail brace %d"%depth,(x,cy,z+3.8),(.15,3.15,.16),S,shaft,.01)
    for y in (cy-1.53,cy+1.53):
        box("SHAFT | transverse brace %d"%depth,(cx,y,z+3.8),(3.36,.18,.18),S,shaft,.01)
    box("SHAFT | inspection luminaire %d"%depth,(cx,cy+1.59,z+3.35),(1.2,.035,.055),LA,shaft,.003)
    txt("SHAFT | depth marker %d"%depth,"%02d m"%abs(depth),(cx-.49,cy+1.66,z+2.35),.27,W,shaft,(math.pi/2,0,math.pi))
    for x in (cx-1.57,cx+1.57):
        for y in (cy-1.52,cy+1.52):
            cyl("SHAFT | anchor bolt",(x,y,z+3.8),.055,.035,N,shaft)
for x in (cx-1.5,cx+1.5):
    for y in (cy-.72,cy+.72):
        box("SHAFT | continuous guide rail",(x,y,-38),(.095,.09,94),N,shaft,.01)
for i,x in enumerate((cx-.7,cx-.52,cx+.52,cx+.7)):
    cyl("SHAFT | cable bundle %d"%i,(x,cy+1.38,-39),.035,91,R if i%2 else G,shaft)
for z in range(-82,9,2):
    box("SHAFT | maintenance ladder rung %d"%z,(cx+1.55,cy+1.05,z),(.12,.72,.045),A,shaft,.005)
for y in (cy+.72,cy+1.38):
    box("SHAFT | ladder stile",(cx+1.55,y,-37),(.07,.07,93),N,shaft,.005)
box("SHAFT | sump floor",(cx,cy,-84.78),(3.36,3.36,.25),D,shaft,.025)
box("SHAFT | drain grate",(cx,cy,-84.64),(1.25,1.25,.06),T,shaft,.005)
for x in (cx-1.22,cx+1.22):
    cyl("SHAFT | buffer cylinder",(x,cy,-83.9),.24,1.45,G,shaft)
    cyl("SHAFT | buffer piston",(x,cy,-83.04),.12,.65,N,shaft)
box("SHAFT | machine cap",(cx,cy,9.55),(4.2,4.2,.42),C,shaft,.03)
for x in (cx-.85,cx+.85):
    cyl("SHAFT | traction sheave",(x,cy,9.83),.45,.18,G,shaft,(math.pi/2,0,0))
lamp("SHAFT | pit work light",(cx,cy,-83.4),(.28,.76,1),420,shaft)
# A self-contained stainless elevator car, parented to an 82 m travel rig.
rig=bpy.data.objects.new("ELEVATOR CAR | animated travel rig",None)
car.objects.link(rig); rig.location=(cx,cy,0)
rig["travel_m"]=82.0; rig["destination"]="B4 hidden level"
def cb(n,local,size,m,b=.02):
    return box(n,(cx+local[0],cy+local[1],local[2]),size,m,car,b,rig)
cb("CAR | structural floor",(0,0,.12),(2.77,2.62,.22),S,.035)
cb("CAR | diamond tread",(0,0,.24),(2.55,2.42,.045),T,.008)
cb("CAR | back wall",(0,1.24,1.65),(2.68,.11,2.84),N)
for x in (-1.32,1.32):
    cb("CAR | side wall",(x,0,1.65),(.12,2.55,2.84),N)
    cb("CAR | waist rail",(x*.92,0,1.12),(.06,2.28,.07),G,.006)
    for y in (-.9,0,.9):
        cb("CAR | panel reveal",(x*.94,y,1.75),(.018,.025,2.4),S,.002)
cb("CAR | ceiling cassette",(0,0,3.13),(2.72,2.6,.16),G)
cb("CAR | luminous ceiling",(0,0,3.035),(1.85,1.7,.025),LW,.005)
cb("CAR | entrance lintel",(0,-1.28,3.12),(2.7,.16,.18),S,.01)
for x in (-1.13,1.13):
    cb("CAR | door jamb",(x,-1.28,1.69),(.13,.13,2.8),G,.008)
for x in (-.63,.63):
    cb("CAR | split door leaf",(x,-1.3,1.66),(1.18,.075,2.72),N,.008)
for y in (-1.1,1.1):
    cb("CAR | perimeter footlight",(0,y,.3),(2.2,.03,.025),LC,.003)
cb("CAR | control panel",(1.225,-.55,1.52),(.035,.46,.94),V,.005)
for i,z in enumerate((1.2,1.44,1.68,1.92)):
    cb("CAR | illuminated button %d"%i,(1.195,-.56,z),(.018,.14,.11),LC if i==3 else LA,.003)
txt("CAR | interior display","G   M   B4",(cx-.55,cy+1.16,2.45),.19,LC,car,(math.pi/2,0,math.pi),rig)
for x in (-1.23,1.23):
    for y in (-1.13,1.13):
        cb("CAR | safety shoe",(x,y,.02),(.18,.18,.2),R,.008)
for f,z in ((1,0),(18,0),(246,B),(269,B),(339,B)):
    rig.location.z=z; rig.keyframe_insert(data_path="location",frame=f)
rig.location.z=0
# B4 landing: concrete, pipework, drainage, security lighting.
box("B4 | corridor floor",(cx,16,B-.16),(4.6,16,.3),C,hall,.02)
box("B4 | landing tread",(cx,23.4,B-.14),(5.3,4.3,.3),T,hall,.02)
box("B4 | corridor ceiling",(cx,16,B+3.65),(4.7,16,.23),D,hall,.02)
for x in (cx-2.3,cx+2.3):
    for y,h in ((10.75,5.5),(20.25,7.5)):
        box("B4 | interrupted hall wall",(x,y,B+1.75),(.28,h,3.5),C,hall,.02)
box("B4 | far bulkhead",(cx,8.1,B+1.75),(4.7,.28,3.5),D,hall,.02)
for y in (10,15,20,23):
    box("B4 | ceiling luminous recess",(cx,y,B+3.5),(2.5,.13,.035),LW,hall,.003)
    box("B4 | grated floor drain",(cx,y-.55,B+.012),(2.1,.17,.025),S,hall,.003)
    lamp("B4 | service light",(cx,y,B+3.1),(.52,.82,1),280,hall)
for x in (cx-2.05,cx+2.05):
    box("B4 | red emergency line",(x,15.7,B+2.9),(.04,15,.035),LR,hall,.003)
    box("B4 | overhead pipe",(x,15.7,B+3.25),(.11,15,.1),G,hall,.006)
txt("B4 | access stencil","B4  /  RESTRICTED",(cx-1.85,8.28,B+2.5),.29,W,hall)
txt("B4 | depth stencil","-82 M  BELOW FACILITY",(cx-1.85,8.28,B+1.95),.19,A,hall)
for side,x,tag in ((-1,-43.3,"A"),(1,-38.7,"B")):
    xinner=-45 if side<0 else -37
    box("B4 | secret "+tag+" connector floor",((x+xinner)/2,15,B-.16),(abs(x-xinner)+.22,3,.3),C,hall,.01)
    box("B4 | secret "+tag+" connector ceiling",((x+xinner)/2,15,B+3.58),(abs(x-xinner)+.22,3,.18),D,hall,.01)
    for y in (13.5,16.5):
        box("B4 | secret "+tag+" connector wall",((x+xinner)/2,y,B+1.75),(abs(x-xinner)+.22,.2,3.5),D,hall,.01)
    door=box("B4 | hidden "+tag+" sliding wall",(x,15,B+1.59),(.12,2.66,3.18),C,hall,.007)
    for z in (.45,2.75):
        box("B4 | hidden "+tag+" track",(x,15,B+z),(.22,3.05,.045),S,hall,.003)
    box("B4 | hidden "+tag+" touch reader",(x-side*.14,12.93,B+1.32),(.07,.22,.34),V,hall,.005)
    box("B4 | hidden "+tag+" reader slit",(x-side*.185,12.93,B+1.32),(.012,.1,.05),LC,hall,.002)
    for frame,yy in ((1,15),(259,15),(280,17.9),(339,17.9)):
        door.location.y=yy; door.keyframe_insert(data_path="location",frame=frame)
# Secret room A: undocumented cryogenic specimen archive.
box("A | reinforced floor",(-51,15,B-.18),(12,10,.35),D,arc,.02)
box("A | ceiling",(-51,15,B+3.9),(12,10,.24),D,arc,.02)
box("A | outer west wall",(-57,15,B+1.88),(.27,10,3.76),C,arc,.02)
for y in (11.75,18.25):
    box("A | interrupted east wall",(-45,y,B+1.88),(.27,3.5,3.76),C,arc,.02)
for y in (10,20):
    box("A | end wall",(-51,y,B+1.88),(12,.27,3.76),C,arc,.02)
for x in (-55.5,-53.1,-50.7):
    for y in (12.8,17.2):
        for z in (.45,1.42,2.39):
            box("A | archive shelf",(x,y,B+z),(1.55,.72,.075),G,arc,.008)
        for dx in (-.72,.72):
            box("A | rack upright",(x+dx,y,B+1.7),(.065,.76,3.25),N,arc,.004)
        box("A | sealed sample crate",(x,y,B+.83),(1.18,.48,.58),S,arc,.008)
        box("A | crate identifier",(x,y-.255,B+.9),(.35,.01,.05),LA,arc,.001)
for i,y in enumerate((12.4,15,17.6)):
    cyl("A | specimen tank base",(-47.6,y,B+.3),.53,.23,G,arc)
    cyl("A | specimen tank shell",(-47.6,y,B+1.43),.43,2.05,V,arc)
    cyl("A | specimen tank cap",(-47.6,y,B+2.51),.54,.21,N,arc)
    cyl("A | specimen luminous core",(-47.6,y,B+1.42),.14,1.62,LC,arc)
    box("A | specimen "+str(i+1)+" ID",(-47.6,y-.53,B+.55),(.55,.035,.15),A,arc,.003)
for x in (-55,-51,-47.5):
    box("A | sterile overhead",(x,15,B+3.7),(1.4,.13,.035),LW,arc,.003)
    lamp("A | cold work light",(x,15,B+3.35),(.48,.8,1),310,arc)
txt("A | warning","ARCHIVE  /  01",(-56.6,10.16,B+2.9),.27,A,arc)
txt("A | warning","UNREGISTERED MATERIAL",(-56.6,10.16,B+2.48),.16,W,arc)
# Secret room B: isolated surveillance and equipment control.
box("B | raised floor",(-31,15,B-.17),(12,10,.34),G,ctrl,.02)
box("B | acoustic ceiling",(-31,15,B+3.9),(12,10,.24),D,ctrl,.02)
box("B | outer east wall",(-25,15,B+1.88),(.27,10,3.76),C,ctrl,.02)
for y in (11.75,18.25):
    box("B | interrupted west wall",(-37,y,B+1.88),(.27,3.5,3.76),C,ctrl,.02)
for y in (10,20):
    box("B | end wall",(-31,y,B+1.88),(12,.27,3.76),C,ctrl,.02)
for x in (-34.8,-32.3,-29.8,-27.3):
    box("B | equipment cabinet",(x,19.17,B+1.55),(1.6,1.05,2.98),S,ctrl,.02)
    for z in (.7,1.02,1.34,1.66,1.98,2.3):
        box("B | cabinet vent",(x,18.62,B+z),(1.22,.018,.055),G,ctrl,.002)
    box("B | active status bar",(x,18.59,B+2.72),(.86,.02,.17),LC,ctrl,.002)
box("B | operations desk",(-31,14,B+1.0),(6.1,1.5,.16),N,ctrl,.025)
for x in (-33.65,-28.35):
    for y in (13.45,14.55):
        box("B | desk leg",(x,y,B+.52),(.13,.13,1.0),S,ctrl,.01)
for x in (-33.3,-31,-28.7):
    box("B | display pedestal",(x,14.75,B+1.35),(.1,.11,.65),S,ctrl,.008)
    box("B | smoked telemetry display",(x,14.67,B+1.82),(1.83,.08,1.2),V,ctrl,.01)
    for z in (1.55,1.82,2.09):
        box("B | telemetry line",(x,14.61,B+z),(1.38,.01,.028),LC,ctrl,.002)
for x in (-34,-31,-28):
    box("B | overhead luminaire",(x,15,B+3.7),(1.8,.12,.04),LW,ctrl,.003)
    lamp("B | work light",(x,15,B+3.3),(.45,.7,1),230,ctrl)
txt("B | title","OBSERVATION  /  02",(-36.6,10.16,B+2.9),.27,A,ctrl)
txt("B | warning","NO NETWORK RECORD",(-36.6,10.16,B+2.48),.16,W,ctrl)
def cam(n,at,aim,col,lens=28):
    d=bpy.data.cameras.new(n); d.lens=lens
    o=bpy.data.objects.new(n,d); col.objects.link(o); o.location=at
    o.rotation_euler=(Vector(aim)-o.location).to_track_quat("-Z","Y").to_euler()
cam("DEEP CAMERA | B4 corridor",(cx,10,B+2.1),(cx,22,B+1.7),hall,24)
cam("DEEP CAMERA | shaft cutaway",(cx+15,cy-21,-37),(cx,cy,-38),shaft,24)
cam("DEEP CAMERA | archive",(-50.7,11.2,B+2.1),(-50.8,16,B+1.3),arc,22)
cam("DEEP CAMERA | control",(-31,11.2,B+2.1),(-31,17,B+1.5),ctrl,22)
bpy.context.scene.frame_set(1)
print("Built",len(root.all_objects),"new objects; 82m travel and two hidden rooms.")
import bpy
from mathutils import Vector
B=-82.0
shaft=bpy.data.collections["10B | deep shaft"]
wall=next(o for o in shaft.objects if o.name.startswith("SHAFT | south wall") and -41 < o.location.z < -39)
wall.location.z=(-78.5-.15)/2
wall.dimensions=(4.02,.32,78.35)
for idx,n in enumerate(("CAR | split door leaf","CAR | split door leaf.001")):
    d=bpy.data.objects[n]
    x0=d.location.x
    sign=-1 if idx==0 else 1
    for frame,x in ((1,x0+sign*.52),(18,x0),(246,x0),(266,x0+sign*.52),(339,x0+sign*.52)):
        d.location.x=x
        d.keyframe_insert(data_path="location",frame=frame)
c=bpy.data.objects["DEEP CAMERA | archive"]
c.location=(-46.2,11.2,B+2)
c.rotation_euler=(Vector((-52,15,B+1.4))-c.location).to_track_quat("-Z","Y").to_euler()
c=bpy.data.objects["DEEP CAMERA | control"]
c.location=(-35.5,11.2,B+2.1)
c.rotation_euler=(Vector((-30,17,B+1.5))-c.location).to_track_quat("-Z","Y").to_euler()
bpy.context.scene.frame_set(280)
print("Refined B4 clearance, car doors, and room cameras")
import bpy
col=bpy.data.collections["10B | deep shaft"]
for z in range(-5,-82,-10):
    d=bpy.data.lights.new("SHAFT | cold inspection lamp %d"%z,"POINT")
    d.energy=330; d.color=(.38,.69,1); d.shadow_soft_size=.55
    o=bpy.data.objects.new(d.name,d); col.objects.link(o); o.location=(-41,27.2,z)
for z in (2.8,7.2):
    d=bpy.data.lights.new("PORTAL | downlight %.1f"%z,"POINT")
    d.energy=420; d.color=(.75,.87,1); d.shadow_soft_size=.7
    o=bpy.data.objects.new(d.name,d); bpy.data.collections["10A | surface portal"].objects.link(o)
    o.location=(-41,23.6,z)
print("Added inspection and portal lighting")
# Final presentation settings.
cam("DEEP CAMERA | surface portal",(-41,19,2.2),(-41,24.5,1.6),surf,26)
cam("DEEP CAMERA | shaft descent",(-41,26,-9),(-41,26,-70),surf,18)
glass=bpy.data.materials["DEEP | smoked glass"]
glass_bs=next(n for n in glass.node_tree.nodes if n.type=="BSDF_PRINCIPLED")
glass_bs.inputs["Transmission Weight"].default_value=.64
glass_bs.inputs["Alpha"].default_value=.42
bpy.context.scene.render.engine="BLENDER_EEVEE"
bpy.context.scene.frame_set(280)
bpy.context.scene.camera=bpy.data.objects["DEEP CAMERA | B4 corridor"]
