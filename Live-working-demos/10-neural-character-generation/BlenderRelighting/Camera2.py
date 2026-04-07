import bpy, math, mathutils, os, random
from bpy.props import (
    FloatProperty, IntProperty, BoolProperty,
    StringProperty, EnumProperty, PointerProperty
)
from bpy.types import Panel, Operator, PropertyGroup

# -----------------------------------------------------------------------------
# Force Cycles for real rendering of lights & volumetrics
# -----------------------------------------------------------------------------
bpy.context.scene.render.engine = 'CYCLES'

# -----------------------------------------------------------------------------
# Settings
# -----------------------------------------------------------------------------
class CameraLightingProperties(PropertyGroup):
    # Animation
    fps: IntProperty(name="Frame Rate",       default=30, min=1, max=120)
    anim_duration: IntProperty(name="Duration (s)", default=12, min=1, max=60)

    # Camera orbit
    camera_distance: FloatProperty(name="Distance", default=5.0, min=0.1, max=50.0)
    camera_height:   FloatProperty(name="Height",   default=1.0, min=-10.0, max=10.0)

    # Preset cycling
    lighting_mode: EnumProperty(
        name="Lighting Mode",
        items=[
            ('PRESETS',"Preset Cycle",""),
            ('HDRI',   "HDRI Rot.",""),
            ('BOTH',   "Both","")
        ], default='PRESETS'
    )
    lighting_duration: IntProperty(name="Seconds per Preset", default=3, min=1, max=30)

    # Which presets?
    use_three_point: BoolProperty(name="Three‑Point",   default=True)
    use_film_noir:   BoolProperty(name="Film Noir",     default=True)
    use_sunset:      BoolProperty(name="Sunset",        default=True)
    use_scifi:       BoolProperty(name="Sci‑Fi",        default=True)
    use_horror:      BoolProperty(name="Horror",        default=False)
    use_contrast:    BoolProperty(name="High Contrast", default=False)
    use_silhouette:  BoolProperty(name="Silhouette",    default=False)

    # Intensity variation & flicker
    intensity_variation: BoolProperty(name="Segment Bump", default=True)
    variation_amount:    FloatProperty(name="Bump Amount", default=0.3, min=0, max=1)
    use_flicker:         BoolProperty(name="Flicker",       default=False)
    flicker_speed:       FloatProperty(name="Flicker Hz",    default=5.0, min=0.1, max=30.0)
    flicker_amount:      FloatProperty(name="Flicker Amt",   default=0.2, min=0, max=1)

    # HDRI
    use_hdri:       BoolProperty(name="Use HDRI",    default=False)
    hdri_path:     StringProperty(name="HDRI Path",   subtype='FILE_PATH')
    hdri_strength: FloatProperty(name="HDRI Strength",default=1.0, min=0, max=10)
    hdri_rotation: BoolProperty(name="Rotate HDRI", default=False)
    hdri_speed:    FloatProperty(name="Rot. Speed",  default=30.0, min=-360, max=360)

    # Volumetrics
    use_volumetrics: BoolProperty(name="Volumetric Fog", default=False)
    vol_density:     FloatProperty(name="Fog Density",   default=0.05, min=0.001, max=1.0)

# -----------------------------------------------------------------------------
# UI Panel
# -----------------------------------------------------------------------------
class VIEW3D_PT_camera_lighting_panel(Panel):
    bl_space_type  = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context     = "scene"
    bl_label       = "Camera & Lighting Animation"

    def draw(self, context):
        p = context.scene.cam_light_props
        l = self.layout

        # Animation
        box = l.box(); box.label(text="Animation")
        row=box.row(); row.prop(p,"fps"); row.prop(p,"anim_duration")

        # Camera
        box = l.box(); box.label(text="Camera Orbit")
        box.prop(p,"camera_distance"); box.prop(p,"camera_height")

        # Lighting Mode
        box = l.box(); box.label(text="Lighting Mode"); box.prop(p,"lighting_mode")
        if p.lighting_mode in {'PRESETS','BOTH'}:
            box.prop(p,"lighting_duration")
            box.label(text="Presets to Cycle:")
            row=box.row(); row.prop(p,"use_three_point"); row.prop(p,"use_film_noir")
            row=box.row(); row.prop(p,"use_sunset");      row.prop(p,"use_scifi")
            row=box.row(); row.prop(p,"use_horror");      row.prop(p,"use_contrast")
            row=box.row(); row.prop(p,"use_silhouette")
            box.separator()
            box.prop(p,"intensity_variation"); box.prop(p,"variation_amount")
            box.prop(p,"use_flicker");         box.prop(p,"flicker_speed"); box.prop(p,"flicker_amount")

        if p.lighting_mode in {'HDRI','BOTH'}:
            box = l.box(); box.label(text="HDRI Settings")
            box.prop(p,"use_hdri")
            if p.use_hdri:
                box.prop(p,"hdri_path"); box.prop(p,"hdri_strength")
                box.prop(p,"hdri_rotation")
                if p.hdri_rotation: box.prop(p,"hdri_speed")

        box = l.box(); box.prop(p,"use_volumetrics")
        if p.use_volumetrics: box.prop(p,"vol_density")

        l.separator()
        l.operator("scene.advanced_dolly_setup", icon='RENDER_ANIMATION')

# -----------------------------------------------------------------------------
# Operator
# -----------------------------------------------------------------------------
class SCENE_OT_advanced_dolly_setup(Operator):
    bl_idname = "scene.advanced_dolly_setup"
    bl_label  = "Setup Animation"
    bl_options= {'REGISTER','UNDO'}

    def execute(self, context):
        setup_advanced_camera_lighting(context, context.scene.cam_light_props)
        return {'FINISHED'}

# -----------------------------------------------------------------------------
# Light creation helper
# -----------------------------------------------------------------------------
def create_light(name, ltype, loc, energy, color, size=1.0, rot=(0,0,0)):
    if name in bpy.data.objects:
        bpy.data.objects.remove(bpy.data.objects[name], do_unlink=True)
    ld = bpy.data.lights.new(name, ltype)
    ld.energy = energy; ld.color = color
    if ltype=='AREA':
        ld.size=size; ld.shape='RECTANGLE'; ld.size_y=size*0.5
    if ltype=='SPOT':
        ld.spot_size=math.radians(45); ld.spot_blend=0.15; ld.shadow_soft_size=size*0.2
    obj = bpy.data.objects.new(name, ld)
    bpy.context.collection.objects.link(obj)
    obj.location = loc; obj.rotation_euler = rot
    return obj

# -----------------------------------------------------------------------------
# Presets
# -----------------------------------------------------------------------------
def setup_three_point(center, d, tag):
    return {
        f"{tag}_Key":  create_light(f"{tag}_Key",'AREA',(center[0]+.7*d,center[1]-.7*d,center[2]+.5*d),1500,(1,0.95,0.9),1.5,(math.radians(-30),math.radians(30),0)),
        f"{tag}_Fill": create_light(f"{tag}_Fill",'AREA',(center[0]-.7*d,center[1]-.5*d,center[2]+.3*d),600,(0.9,0.95,1),2.0,(math.radians(-15),math.radians(-40),0)),
        f"{tag}_Back": create_light(f"{tag}_Back",'SPOT',(center[0],center[1]+.8*d,center[2]+.7*d),1200,(1,1,1),0.5,(math.radians(-45),0,0))
    }

def setup_film_noir(center, d, tag):
    return {
        f"{tag}_Key":  create_light(f"{tag}_Key",'SPOT',(center[0]+.3*d,center[1]-.3*d,center[2]+.8*d),2500,(1,0.95,0.9),0.3,(math.radians(-60),math.radians(30),0)),
        f"{tag}_Fill": create_light(f"{tag}_Fill",'AREA',(center[0]-.7*d,center[1]-.5*d,center[2]+.3*d),50,(0.8,0.85,1),3.0,(math.radians(-15),math.radians(-40),0)),
        f"{tag}_Back": create_light(f"{tag}_Back",'SPOT',(center[0],center[1]+.8*d,center[2]+.7*d),1800,(1,1,1),0.2,(math.radians(-45),0,0))
    }

def setup_sunset(center, d, tag):
    return {
        f"{tag}_Key":  create_light(f"{tag}_Key",'SUN',(center[0]+.8*d,center[1]-.2*d,center[2]+.1*d),3,(1,0.4,0.1),1.0,(math.radians(-5),math.radians(20),0)),
        f"{tag}_Fill": create_light(f"{tag}_Fill",'AREA',(center[0],center[1],center[2]+1.2*d),300,(0.2,0.4,0.9),5.0,(math.radians(-90),0,0)),
        f"{tag}_Back": create_light(f"{tag}_Back",'AREA',(center[0]-.7*d,center[1]+.7*d,center[2]+.5*d),1500,(1,0.7,0.2),1.0,(math.radians(-30),math.radians(-135),0))
    }

def setup_scifi(center, d, tag):
    return {
        f"{tag}_Key":  create_light(f"{tag}_Key",'AREA',(center[0]+.7*d,center[1]-.7*d,center[2]+.5*d),1800,(0.3,0.9,1),1.5,(math.radians(-30),math.radians(30),0)),
        f"{tag}_Fill": create_light(f"{tag}_Fill",'AREA',(center[0]-.7*d,center[1]-.5*d,center[2]+.3*d),1000,(0.5,0.1,1),1.0,(math.radians(-15),math.radians(-40),0)),
        f"{tag}_Back": create_light(f"{tag}_Back",'SPOT',(center[0],center[1]+.8*d,center[2]+.3*d),2000,(0,1,0.7),0.3,(math.radians(-20),0,0))
    }

def setup_horror(center,d,tag):
    return {
        f"{tag}_Key":    create_light(f"{tag}_Key",'SPOT',(center[0],center[1]-.5*d,center[2]-.3*d),800,(0.9,0.8,0.7),0.5,(math.radians(60),0,0)),
        f"{tag}_Rim":    create_light(f"{tag}_Rim",'SPOT',(center[0],center[1]+.7*d,center[2]+.5*d),1500,(0.8,0.8,1),0.3,(math.radians(-30),0,0)),
        f"{tag}_Fill":   create_light(f"{tag}_Fill",'AREA',(center[0]+.8*d,center[1],center[2]),100,(0.6,0.7,1),2.0,(0,math.radians(90),0))
    }

def setup_contrast(center,d,tag):
    return {
        f"{tag}_Key":  create_light(f"{tag}_Key",'SPOT',(center[0]+.8*d,center[1]-.3*d,center[2]+.7*d),3000,(1,0.98,0.95),0.5,(math.radians(-40),math.radians(20),0)),
        f"{tag}_Fill": create_light(f"{tag}_Fill",'AREA',(center[0]-.7*d,center[1]-.4*d,center[2]),40,(0.8,0.9,1),3.0,(0,math.radians(-60),0))
    }

def setup_silhouette(center,d,tag):
    return {
        f"{tag}_Back": create_light(f"{tag}_Back",'AREA',(center[0],center[1]+.8*d,center[2]+.2*d),3000,(1,0.9,0.8),3.0,(math.radians(-15),0,0)),
        f"{tag}_Fill": create_light(f"{tag}_Fill",'AREA',(center[0],center[1]-.9*d,center[2]),30,(0.6,0.7,0.9),4.0,(0,math.radians(180),0))
    }

# -----------------------------------------------------------------------------
# Volumetrics & HDRI
# -----------------------------------------------------------------------------
def setup_volumetric_atmosphere(context, density):
    bpy.ops.mesh.primitive_cube_add(size=20)
    cube = context.active_object; cube.name="Volumetric_Atmosphere"
    mat  = bpy.data.materials.new("Volume_Mat"); mat.use_nodes=True
    nodes=mat.node_tree.nodes; links=mat.node_tree.links; nodes.clear()
    vol=nodes.new('ShaderNodeVolumePrincipled'); out=nodes.new('ShaderNodeOutputMaterial')
    vol.inputs['Density'].default_value=density
    links.new(vol.outputs['Volume'], out.inputs['Volume'])
    cube.data.materials.append(mat); cube.display_type='WIRE'; cube.hide_render=False

def setup_hdri_environment(context,path,strength):
    world = context.scene.world or bpy.data.worlds.new("World")
    context.scene.world=world; world.use_nodes=True
    tree=world.node_tree; tree.nodes.clear()
    bg=tree.nodes.new('ShaderNodeBackground'); tex=tree.nodes.new('ShaderNodeTexEnvironment')
    mapn=tree.nodes.new('ShaderNodeMapping'); coord=tree.nodes.new('ShaderNodeTexCoord')
    out=tree.nodes.new('ShaderNodeOutputWorld')
    coord.location=(-800,0); mapn.location=(-600,0); tex.location=(-400,0)
    bg.location=(-200,0); out.location=(0,0)
    tree.links.new(coord.outputs['Generated'], mapn.inputs['Vector'])
    tree.links.new(mapn.outputs['Vector'],     tex.inputs['Vector'])
    tree.links.new(tex.outputs['Color'],       bg.inputs['Color'])
    tree.links.new(bg.outputs['Background'],   out.inputs['Surface'])
    bg.inputs['Strength'].default_value=strength
    if path and os.path.exists(path):
        try:
            img = bpy.data.images.load(path) if os.path.basename(path) not in bpy.data.images else bpy.data.images[os.path.basename(path)]
            if path.lower().endswith(('.hdr','.exr')): img.colorspace_settings.name='Non-Color'
            tex.image=img; tex.projection='EQUIRECTANGULAR'
        except Exception as e:
            print("HDRI load error:",e)
    else:
        print("Invalid HDRI path:",path)
    return mapn

# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------
def setup_advanced_camera_lighting(context,p):
    scene=context.scene
    total=p.fps*p.anim_duration
    scene.frame_start,scene.frame_end=1,total

    # Create/orbit camera
    cam=bpy.data.objects.get("Dolly_Camera")
    if not cam:
        cd=bpy.data.cameras.new("Dolly_Camera")
        cam=bpy.data.objects.new("Dolly_Camera",cd)
        scene.collection.objects.link(cam)
    scene.camera=cam
    if cam.animation_data: cam.animation_data_clear()

    # Compute center
    meshes=[o for o in scene.objects if o.type=='MESH']
    if meshes:
        cx=sum(o.location.x for o in meshes)/len(meshes)
        cy=sum(o.location.y for o in meshes)/len(meshes)
        cz=sum(o.location.z for o in meshes)/len(meshes)
        center=(cx,cy,cz)
    else:
        center=(0,0,0)

    # Orbit cam
    for f in range(1,total+1):
        ang=2*math.pi*(f-1)/total
        x=center[0]+p.camera_distance*math.sin(ang)
        y=center[1]+p.camera_distance*math.cos(ang)
        z=center[2]+p.camera_height
        cam.location=(x,y,z)
        cam.rotation_euler=(mathutils.Vector(center)-cam.location).to_track_quat('-Z','Y').to_euler()
        cam.keyframe_insert("location",frame=f)
        cam.keyframe_insert("rotation_euler",frame=f)

    # Gather presets
    presets=[]
    def add(c,fn,tag): 
        if c: presets.append((fn,tag))
    add(p.use_three_point,setup_three_point,"TP")
    add(p.use_film_noir,  setup_film_noir,  "FN")
    add(p.use_sunset,     setup_sunset,     "SS")
    add(p.use_scifi,      setup_scifi,      "SF")
    add(p.use_horror,     setup_horror,     "HR")
    add(p.use_contrast,   setup_contrast,   "HC")
    add(p.use_silhouette, setup_silhouette, "SL")

    # Preset relighting
    if p.lighting_mode in {'PRESETS','BOTH'} and presets:
        seg=max(1,total//len(presets))
        for i,(fn,tag) in enumerate(presets):
            lights=fn(center,p.camera_distance,tag)
            on=1+i*seg; off=min(total,on+seg-1)
            pre=on-1; post=off+1
            for obj in lights.values():
                full=obj.data.energy
                # off
                if pre>=1:
                    obj.data.energy=0; obj.data.keyframe_insert("energy",frame=pre)
                # on
                obj.data.energy=full; obj.data.keyframe_insert("energy",frame=on)
                # optional bump
                if p.intensity_variation:
                    mid=(on+off)//2
                    bump=full*(1+random.uniform(-p.variation_amount,p.variation_amount))
                    obj.data.energy=bump; obj.data.keyframe_insert("energy",frame=mid)
                # stay on
                obj.data.energy=full; obj.data.keyframe_insert("energy",frame=off)
                # flicker
                if p.use_flicker:
                    step=max(1,int(p.fps/p.flicker_speed))
                    for ff in range(on,off+1,step):
                        flick=full*(1+random.uniform(-p.flicker_amount,p.flicker_amount))
                        obj.data.energy=flick; obj.data.keyframe_insert("energy",frame=ff)
                # off after
                if post<=total:
                    obj.data.energy=0; obj.data.keyframe_insert("energy",frame=post)
                # hard snaps
                if obj.animation_data and obj.animation_data.action:
                    for fcu in obj.animation_data.action.fcurves:
                        for kp in fcu.keyframe_points:
                            kp.interpolation='CONSTANT'

    # HDRI
    if p.lighting_mode in {'HDRI','BOTH'} and p.use_hdri and p.hdri_path:
        mapping=setup_hdri_environment(context,p.hdri_path,p.hdri_strength)
        if p.hdri_rotation:
            for f in range(1,total+1):
                r=math.radians((f-1)*p.hdri_speed/p.fps%360)
                mapping.inputs['Rotation'].default_value[2]=r
                mapping.inputs['Rotation'].keyframe_insert("default_value",frame=f)

    # Volumetrics
    if p.use_volumetrics:
        setup_volumetric_atmosphere(context,p.vol_density)

# -----------------------------------------------------------------------------
# Registration
# -----------------------------------------------------------------------------
classes=(
    CameraLightingProperties,
    VIEW3D_PT_camera_lighting_panel,
    SCENE_OT_advanced_dolly_setup
)
def register():
    for c in classes: bpy.utils.register_class(c)
    bpy.types.Scene.cam_light_props=PointerProperty(type=CameraLightingProperties)
def unregister():
    for c in reversed(classes): bpy.utils.unregister_class(c)
    del bpy.types.Scene.cam_light_props

if __name__=="__main__":
    register()
    bpy.context.window_manager.popup_menu(
        lambda s,c: s.layout.label(text="Camera & Lighting Add‑on Loaded!"),
        title="Loaded", icon='INFO'
    )
