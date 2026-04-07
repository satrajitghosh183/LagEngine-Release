#import bpy
#import math
#import mathutils
#import os
#from bpy.props import (FloatProperty, IntProperty, BoolProperty, 
#                      StringProperty, EnumProperty, PointerProperty)
#from bpy.types import (Panel, Operator, PropertyGroup)

## -----------------------------------------------------------------------------
## Property Group for Settings
## -----------------------------------------------------------------------------
#class CameraLightingProperties(PropertyGroup):
#    # Animation settings
#    fps: IntProperty(
#        name="Frame Rate",
#        description="Frames per second for the animation",
#        default=30,
#        min=1,
#        max=120
#    )
#    
#    anim_duration: IntProperty(
#        name="Duration (seconds)",
#        description="Total duration of the animation in seconds",
#        default=12,
#        min=1,
#        max=60
#    )
#    
#    # Camera settings
#    camera_distance: FloatProperty(
#        name="Camera Distance",
#        description="Distance from camera to subject",
#        default=5.0,
#        min=0.1,
#        max=50.0
#    )
#    
#    camera_height: FloatProperty(
#        name="Camera Height",
#        description="Camera height above center",
#        default=1.0,
#        min=-10.0,
#        max=10.0
#    )
#    
#    camera_start_position: EnumProperty(
#        name="Camera Start Position",
#        description="Starting position of the camera",
#        items=[
#            ('FRONT', "Front", "Start from front"),
#            ('BACK', "Back", "Start from back"),
#            ('LEFT', "Left", "Start from left"),
#            ('RIGHT', "Right", "Start from right")
#        ],
#        default='FRONT'
#    )
#    
#    # Lighting settings
#    lighting_mode: EnumProperty(
#        name="Lighting Mode",
#        description="How to control lighting during animation",
#        items=[
#            ('PRESETS', "Preset Cycle", "Cycle through lighting presets"),
#            ('HDRI', "HDRI Rotation", "Rotate an HDRI environment"),
#            ('BOTH', "Both", "Use both presets and HDRI")
#        ],
#        default='PRESETS'
#    )
#    
#    lighting_duration: IntProperty(
#        name="Seconds Per Lighting",
#        description="How many seconds to show each lighting setup",
#        default=3,
#        min=1,
#        max=30
#    )
#    
#    use_three_point: BoolProperty(
#        name="Three Point Lighting",
#        description="Include Three Point Lighting in the cycle",
#        default=True
#    )
#    
#    use_film_noir: BoolProperty(
#        name="Film Noir Lighting",
#        description="Include Film Noir Lighting in the cycle",
#        default=True
#    )
#    
#    use_sunset: BoolProperty(
#        name="Sunset Lighting", 
#        description="Include Sunset Lighting in the cycle",
#        default=True
#    )
#    
#    use_scifi: BoolProperty(
#        name="Sci-Fi Lighting",
#        description="Include Sci-Fi Lighting in the cycle",
#        default=True
#    )
#    
#    # HDRI settings
#    use_hdri: BoolProperty(
#        name="Use HDRI Background",
#        description="Use an HDRI image for lighting and background",
#        default=False
#    )
#    
#    hdri_path: StringProperty(
#        name="HDRI Path",
#        description="Path to HDRI file",
#        default="",
#        subtype='FILE_PATH'
#    )
#    
#    hdri_strength: FloatProperty(
#        name="HDRI Strength",
#        description="Strength of HDRI lighting",
#        default=1.0,
#        min=0.0,
#        max=10.0
#    )
#    
#    hdri_rotation: BoolProperty(
#        name="Rotate HDRI",
#        description="Rotate the HDRI during animation",
#        default=False
#    )
#    
#    hdri_rotation_speed: FloatProperty(
#        name="Rotation Speed",
#        description="Speed of HDRI rotation (degrees per second)",
#        default=30.0,
#        min=-360.0,
#        max=360.0
#    )

## -----------------------------------------------------------------------------
## Main Operator - Setup Camera and Lighting
## -----------------------------------------------------------------------------
#class SCENE_OT_advanced_dolly_setup(Operator):
#    """Create an advanced camera animation with lighting effects"""
#    bl_idname = "scene.advanced_dolly_setup"
#    bl_label = "Setup Camera Animation"
#    bl_options = {'REGISTER', 'UNDO'}
#    
#    def execute(self, context):
#        try:
#            props = context.scene.cam_light_props
#            setup_advanced_camera_lighting(context, props)
#            self.report({'INFO'}, "Camera and lighting setup complete. Press Alt+A to play.")
#            return {'FINISHED'}
#        except Exception as e:
#            self.report({'ERROR'}, f"Error: {str(e)}")
#            print(f"Error details: {str(e)}")
#            return {'CANCELLED'}

## -----------------------------------------------------------------------------
## Setup HDRI Environment
## -----------------------------------------------------------------------------
#def setup_hdri_environment(context, hdri_path, strength=1.0):
#    """Setup HDRI environment map"""
#    
#    # Make sure we're using Cycles or Eevee
#    if context.scene.render.engine not in ['CYCLES', 'BLENDER_EEVEE']:
#        context.scene.render.engine = 'CYCLES'
#    
#    # Get the world or create one if it doesn't exist
#    world = context.scene.world
#    if world is None:
#        world = bpy.data.worlds.new("World")
#        context.scene.world = world
#    
#    # Enable use of nodes
#    world.use_nodes = True
#    tree = world.node_tree
#    
#    # Clear existing nodes
#    for node in tree.nodes:
#        tree.nodes.remove(node)
#    
#    # Create nodes
#    bg_node = tree.nodes.new(type="ShaderNodeBackground")
#    env_node = tree.nodes.new(type="ShaderNodeTexEnvironment")
#    output_node = tree.nodes.new(type="ShaderNodeOutputWorld")
#    
#    # Create a driver-friendly setup for rotation
#    tex_coord_node = tree.nodes.new(type="ShaderNodeTexCoord")
#    # Use separate XYZ input/output to allow animation
#    separate_xyz_node = tree.nodes.new(type="ShaderNodeSeparateXYZ")
#    combine_xyz_node = tree.nodes.new(type="ShaderNodeCombineXYZ")
#    
#    # Create a Value node that we can animate
#    z_rotation_node = tree.nodes.new(type="ShaderNodeValue")
#    z_rotation_node.name = "HDRI_Rotation"
#    z_rotation_node.label = "HDRI Rotation"
#    
#    # Set node locations for better organization
#    tex_coord_node.location = (-800, 0)
#    separate_xyz_node.location = (-600, 0)
#    z_rotation_node.location = (-600, -200)
#    combine_xyz_node.location = (-400, 0)
#    env_node.location = (-200, 0)
#    bg_node.location = (0, 0)
#    output_node.location = (200, 0)
#    
#    # Link nodes
#    tree.links.new(tex_coord_node.outputs["Generated"], separate_xyz_node.inputs["Vector"])
#    tree.links.new(separate_xyz_node.outputs["X"], combine_xyz_node.inputs["X"])
#    tree.links.new(separate_xyz_node.outputs["Y"], combine_xyz_node.inputs["Y"])
#    tree.links.new(z_rotation_node.outputs["Value"], combine_xyz_node.inputs["Z"])
#    tree.links.new(combine_xyz_node.outputs["Vector"], env_node.inputs["Vector"])
#    tree.links.new(env_node.outputs["Color"], bg_node.inputs["Color"])
#    tree.links.new(bg_node.outputs["Background"], output_node.inputs["Surface"])
#    
#    # Set background strength
#    bg_node.inputs["Strength"].default_value = strength
#    
#    # Load HDRI if path exists
#    if hdri_path and os.path.exists(hdri_path):
#        # Load image texture
#        try:
#            img = bpy.data.images.load(hdri_path, check_existing=True)
#            env_node.image = img
#            return z_rotation_node  # Return the rotation node for animation
#        except Exception as e:
#            print(f"Failed to load HDRI: {hdri_path}")
#            print(f"Error: {str(e)}")
#    
#    # Return the rotation node anyway (even if HDRI failed to load)
#    return z_rotation_node

## -----------------------------------------------------------------------------
## Main Setup Function
## -----------------------------------------------------------------------------
#def setup_advanced_camera_lighting(context, props):
#    """Main function to set up camera and lighting"""
#    scene = context.scene
#    
#    # Set FPS
#    scene.render.fps = props.fps
#    
#    # Calculate total frames
#    total_frames = props.anim_duration * props.fps
#    scene.frame_start = 1
#    scene.frame_end = total_frames
#    
#    # 1. Create or find dolly camera
#    if "Dolly_Camera" not in bpy.data.objects:
#        cam_data = bpy.data.cameras.new("Dolly_Camera")
#        cam_obj = bpy.data.objects.new("Dolly_Camera", cam_data)
#        scene.collection.objects.link(cam_obj)
#    else:
#        cam_obj = bpy.data.objects["Dolly_Camera"]
#    
#    # Set as active camera
#    scene.camera = cam_obj
#    
#    # 2. Clear existing camera animation
#    if cam_obj.animation_data and cam_obj.animation_data.action:
#        bpy.data.actions.remove(cam_obj.animation_data.action)
#    
#    # 3. Calculate scene center
#    objects = [obj for obj in scene.objects if obj.type == 'MESH' and obj.visible_get()]
#    
#    if not objects:
#        # If no objects, use world origin
#        center = (0, 0, 0)
#    else:
#        # Calculate center point of all visible objects
#        center_x = sum(obj.location.x for obj in objects) / len(objects)
#        center_y = sum(obj.location.y for obj in objects) / len(objects)
#        center_z = sum(obj.location.z for obj in objects) / len(objects)
#        center = (center_x, center_y, center_z)
#    
#    # 4. Create camera dolly animation
#    distance = props.camera_distance
#    height = props.camera_height
#    
#    # Determine starting angle based on user selection
#    start_angle_map = {
#        'FRONT': math.pi,      # 180 degrees
#        'BACK': 0,             # 0 degrees
#        'LEFT': math.pi / 2,   # 90 degrees
#        'RIGHT': 3 * math.pi / 2  # 270 degrees
#    }
#    start_angle = start_angle_map[props.camera_start_position]
#    
#    # Clear any previous keyframes
#    if cam_obj.animation_data:
#        cam_obj.animation_data_clear()
#    
#    # Create camera animation with more keyframes for smoothness
#    keyframe_step = max(1, total_frames // 60)  # At least 60 keyframes total
#    
#    for frame_idx, frame in enumerate(range(1, total_frames + 1, keyframe_step)):
#        # Calculate position around circle
#        angle = start_angle + (frame - 1) * (2 * math.pi / total_frames)
#        
#        x = center[0] + distance * math.sin(angle)
#        y = center[1] + distance * math.cos(angle)
#        z = center[2] + height  # Height above center
#        
#        # Position camera
#        cam_obj.location = (x, y, z)
#        
#        # Point camera at center
#        direction = mathutils.Vector(center) - mathutils.Vector((x, y, z))
#        rot_quat = direction.to_track_quat('-Z', 'Y')
#        cam_obj.rotation_euler = rot_quat.to_euler()
#        
#        # Keyframe location and rotation
#        cam_obj.keyframe_insert(data_path="location", frame=frame)
#        cam_obj.keyframe_insert(data_path="rotation_euler", frame=frame)
#    
#    # Set interpolation to BEZIER for smoother camera movement
#    if cam_obj.animation_data and cam_obj.animation_data.action:
#        for fcurve in cam_obj.animation_data.action.fcurves:
#            for keyframe_point in fcurve.keyframe_points:
#                keyframe_point.interpolation = 'BEZIER'
#    
#    # 5. Setup HDRI if enabled
#    rotation_node = None
#    if props.use_hdri and props.hdri_path:
#        rotation_node = setup_hdri_environment(context, props.hdri_path, props.hdri_strength)
#        
#        # Setup HDRI rotation animation if enabled and node exists
#        if props.hdri_rotation and rotation_node:
#            # Animate rotation - directly keyframe the Value node
#            keyframe_step = max(1, total_frames // 30)  # Not too many keyframes
#            
#            for frame in range(1, total_frames + 1, keyframe_step):
#                # Calculate rotation value based on frame and speed
#                rotation_value = ((frame - 1) * props.hdri_rotation_speed / props.fps) % 360
#                rotation_radians = math.radians(rotation_value)
#                
#                # Set value and keyframe it
#                rotation_node.outputs[0].default_value = rotation_radians
#                rotation_node.outputs[0].keyframe_insert(data_path="default_value", frame=frame)
#    
#    # 6. Set up lighting presets if enabled
#    if props.lighting_mode in ['PRESETS', 'BOTH']:
#        # Collect enabled presets
#        enabled_presets = []
#        if props.use_three_point:
#            enabled_presets.append("three_point")
#        if props.use_film_noir:
#            enabled_presets.append("film_noir")
#        if props.use_sunset:
#            enabled_presets.append("sunset")
#        if props.use_scifi:
#            enabled_presets.append("scifi")
#        
#        # If no presets are enabled, default to all
#        if not enabled_presets:
#            enabled_presets = ["three_point", "film_noir", "sunset", "scifi"]
#        
#        # Calculate frames per lighting setup
#        frames_per_lighting = props.lighting_duration * props.fps
#        
#        # Create lighting control object for keyframing
#        if "Lighting_Switch" not in bpy.data.objects:
#            lighting_control = bpy.data.objects.new("Lighting_Switch", None)
#            scene.collection.objects.link(lighting_control)
#        else:
#            lighting_control = bpy.data.objects["Lighting_Switch"]
#        
#        # Add a custom property to control lighting type
#        if "lighting_type" not in lighting_control:
#            lighting_control["lighting_type"] = 0
#            lighting_control.id_properties_ui("lighting_type").update(min=0, max=len(enabled_presets) - 1)
#        
#        # Clear any existing animation on the control object
#        if lighting_control.animation_data and lighting_control.animation_data.action:
#            bpy.data.actions.remove(lighting_control.animation_data.action)
#        
#        # Calculate how many full cycles we can fit
#        total_frames_needed = len(enabled_presets) * frames_per_lighting
#        num_full_cycles = total_frames // total_frames_needed
#        remaining_frames = total_frames % total_frames_needed
#        
#        # Set keyframes for full cycles
#        current_frame = 1
#        for cycle in range(num_full_cycles):
#            for preset_idx, preset_name in enumerate(enabled_presets):
#                # Set the lighting type value
#                lighting_control["lighting_type"] = preset_idx
#                
#                # Insert keyframe
#                lighting_control.keyframe_insert(data_path='["lighting_type"]', frame=current_frame)
#                
#                # Move to next lighting timing
#                current_frame += frames_per_lighting
#        
#        # Handle remaining frames if any
#        remaining_presets = remaining_frames // frames_per_lighting
#        for preset_idx in range(remaining_presets):
#            lighting_control["lighting_type"] = preset_idx
#            lighting_control.keyframe_insert(data_path='["lighting_type"]', frame=current_frame)
#            current_frame += frames_per_lighting
#        
#        # Make lighting changes instant (not gradual)
#        if lighting_control.animation_data and lighting_control.animation_data.action:
#            for fcurve in lighting_control.animation_data.action.fcurves:
#                for kfp in fcurve.keyframe_points:
#                    kfp.interpolation = 'CONSTANT'
#        
#        # Create a frame change handler to apply lighting based on the control object
#        # Clear any existing handlers first
#        for handler in bpy.app.handlers.frame_change_pre:
#            if hasattr(handler, "__name__") and handler.__name__ == "apply_lighting_preset":
#                bpy.app.handlers.frame_change_pre.remove(handler)
#        
#        def apply_lighting_preset(scene):
#            # Get the current lighting type from the control object
#            if "Lighting_Switch" not in scene.objects:
#                return
#            
#            lighting_control = scene.objects["Lighting_Switch"]
#            if "lighting_type" not in lighting_control:
#                return
#            
#            lighting_type = int(lighting_control["lighting_type"])
#            
#            # Skip if the lighting is already set to this type
#            if hasattr(apply_lighting_preset, "last_type") and apply_lighting_preset.last_type == lighting_type:
#                return
#            
#            # Get the preset name based on the lighting type
#            if lighting_type < 0 or lighting_type >= len(enabled_presets):
#                return
#            
#            preset_name = enabled_presets[lighting_type]
#            
#            # Try to safely remove any existing lights
#            try:
#                for obj in list(scene.objects):
#                    if any(light_type in obj.name for light_type in ["Key_Light", "Fill_Light", "Back_Light", "Rim_Light"]):
#                        bpy.data.objects.remove(obj, do_unlink=True)
#            except Exception as e:
#                print(f"Warning: Could not remove all lights: {e}")
#            
#            # Call the appropriate lighting setup operator
#            try:
#                if preset_name == "three_point":
#                    bpy.ops.scene.interactive_lighting(setup_type='three_point')
#                elif preset_name == "film_noir":
#                    bpy.ops.scene.interactive_lighting(setup_type='film_noir')
#                elif preset_name == "sunset":
#                    bpy.ops.scene.interactive_lighting(setup_type='sunset')
#                elif preset_name == "scifi":
#                    bpy.ops.scene.interactive_lighting(setup_type='scifi')
#                
#                print(f"Successfully applied {preset_name} lighting at frame {scene.frame_current}")
#                apply_lighting_preset.last_type = lighting_type
#            except Exception as e:
#                print(f"Error applying {preset_name} lighting: {e}")
#        
#        # Initialize the last_type attribute
#        apply_lighting_preset.last_type = -1
#        
#        # Set function name for easier identification
#        apply_lighting_preset.__name__ = "apply_lighting_preset"
#        
#        # Register the handler
#        bpy.app.handlers.frame_change_pre.append(apply_lighting_preset)
#        
#        # Ensure the first lighting preset is applied
#        scene.frame_set(1)
#        lighting_control["lighting_type"] = 0
#        apply_lighting_preset(scene)
#    
#    # 7. Set camera view in viewport
#    for area in context.screen.areas:
#        if area.type == 'VIEW_3D':
#            area.spaces[0].region_3d.view_perspective = 'CAMERA'
#            # Ensure we're looking through the camera
#            override = context.copy()
#            override["area"] = area
#            override["region"] = area.regions[-1]
#            try:
#                bpy.ops.view3d.view_camera(override)
#            except:
#                pass
#            break

## -----------------------------------------------------------------------------
## UI Panels
## -----------------------------------------------------------------------------
#class VIEW3D_PT_camera_lighting_panel(Panel):
#    bl_space_type = 'PROPERTIES'
#    bl_region_type = 'WINDOW'
#    bl_context = "scene"
#    bl_label = "Camera & Lighting Animation"
#    
#    def draw(self, context):
#        layout = self.layout
#        props = context.scene.cam_light_props
#        
#        # Animation Section
#        box = layout.box()
#        box.label(text="Animation Settings:")
#        row = box.row()
#        row.prop(props, "fps")
#        row.prop(props, "anim_duration")
#        
#        # Camera Settings
#        box = layout.box()
#        box.label(text="Camera Settings:")
#        box.prop(props, "camera_distance")
#        box.prop(props, "camera_height")
#        box.prop(props, "camera_start_position")
#        
#        # Lighting Settings
#        box = layout.box()
#        box.label(text="Lighting Settings:")
#        box.prop(props, "lighting_mode")
#        box.prop(props, "lighting_duration")
#        
#        # Only show lighting checkboxes if presets are enabled
#        if props.lighting_mode in ['PRESETS', 'BOTH']:
#            box.label(text="Lighting Presets to Include:")
#            row = box.row()
#            row.prop(props, "use_three_point")
#            row.prop(props, "use_film_noir")
#            row = box.row()
#            row.prop(props, "use_sunset")
#            row.prop(props, "use_scifi")
#        
#        # HDRI Settings
#        box = layout.box()
#        box.label(text="HDRI Settings:")
#        box.prop(props, "use_hdri")
#        
#        if props.use_hdri:
#            box.prop(props, "hdri_path")
#            box.prop(props, "hdri_strength")
#            box.prop(props, "hdri_rotation")
#            
#            if props.hdri_rotation:
#                box.prop(props, "hdri_rotation_speed")
#        
#        # Setup Button
#        layout.separator()
#        layout.operator("scene.advanced_dolly_setup")

## -----------------------------------------------------------------------------
## Registration
## -----------------------------------------------------------------------------
#classes = (
#    CameraLightingProperties,
#    SCENE_OT_advanced_dolly_setup,
#    VIEW3D_PT_camera_lighting_panel,
#)

#def register():
#    for cls in classes:
#        bpy.utils.register_class(cls)
#    
#    bpy.types.Scene.cam_light_props = PointerProperty(type=CameraLightingProperties)

#def unregister():
#    for cls in reversed(classes):
#        bpy.utils.unregister_class(cls)
#    
#    del bpy.types.Scene.cam_light_props

#if __name__ == "__main__":
#    # Unregister if already registered
#    try:
#        unregister()
#    except:
#        pass
#    
#    register()
#    
#    # Show message in the info area
#    def show_message(self, context):
#        self.layout.label(text="Camera & Lighting Animation panel added to Scene properties")
#    
#    bpy.context.window_manager.popup_menu(show_message, title="Script Loaded", icon="INFO")




import bpy
import math
import mathutils
import os
from bpy.props import (FloatProperty, IntProperty, BoolProperty, 
                      StringProperty, EnumProperty, PointerProperty)
from bpy.types import (Panel, Operator, PropertyGroup)

# -----------------------------------------------------------------------------
# Property Group for Settings
# -----------------------------------------------------------------------------
class CameraLightingProperties(PropertyGroup):
    # Animation settings
    fps: IntProperty(
        name="Frame Rate",
        description="Frames per second for the animation",
        default=30,
        min=1,
        max=120
    )
    
    anim_duration: IntProperty(
        name="Duration (seconds)",
        description="Total duration of the animation in seconds",
        default=12,
        min=1,
        max=60
    )
    
    # Camera settings
    camera_distance: FloatProperty(
        name="Camera Distance",
        description="Distance from camera to subject",
        default=5.0,
        min=0.1,
        max=50.0
    )
    
    camera_height: FloatProperty(
        name="Camera Height",
        description="Camera height above center",
        default=1.0,
        min=-10.0,
        max=10.0
    )
    
    camera_start_position: EnumProperty(
        name="Camera Start Position",
        description="Starting position of the camera",
        items=[
            ('FRONT', "Front", "Start from front"),
            ('BACK', "Back", "Start from back"),
            ('LEFT', "Left", "Start from left"),
            ('RIGHT', "Right", "Start from right")
        ],
        default='FRONT'
    )
    
    # Lighting settings
    lighting_mode: EnumProperty(
        name="Lighting Mode",
        description="How to control lighting during animation",
        items=[
            ('PRESETS', "Preset Cycle", "Cycle through lighting presets"),
            ('HDRI', "HDRI Rotation", "Rotate an HDRI environment"),
            ('BOTH', "Both", "Use both presets and HDRI")
        ],
        default='PRESETS'
    )
    
    lighting_duration: IntProperty(
        name="Seconds Per Lighting",
        description="How many seconds to show each lighting setup",
        default=3,
        min=1,
        max=30
    )
    
    use_three_point: BoolProperty(
        name="Three Point Lighting",
        description="Include Three Point Lighting in the cycle",
        default=True
    )
    
    use_film_noir: BoolProperty(
        name="Film Noir Lighting",
        description="Include Film Noir Lighting in the cycle",
        default=True
    )
    
    use_sunset: BoolProperty(
        name="Sunset Lighting", 
        description="Include Sunset Lighting in the cycle",
        default=True
    )
    
    use_scifi: BoolProperty(
        name="Sci-Fi Lighting",
        description="Include Sci-Fi Lighting in the cycle",
        default=True
    )
    
    # HDRI settings
    use_hdri: BoolProperty(
        name="Use HDRI Background",
        description="Use an HDRI image for lighting and background",
        default=False
    )
    
    hdri_path: StringProperty(
        name="HDRI Path",
        description="Path to HDRI file",
        default="",
        subtype='FILE_PATH'
    )
    
    hdri_strength: FloatProperty(
        name="HDRI Strength",
        description="Strength of HDRI lighting",
        default=1.0,
        min=0.0,
        max=10.0
    )
    
    hdri_rotation: BoolProperty(
        name="Rotate HDRI",
        description="Rotate the HDRI during animation",
        default=False
    )
    
    hdri_rotation_speed: FloatProperty(
        name="Rotation Speed",
        description="Speed of HDRI rotation (degrees per second)",
        default=30.0,
        min=-360.0,
        max=360.0
    )

# -----------------------------------------------------------------------------
# Main Operator - Setup Camera and Lighting
# -----------------------------------------------------------------------------
class SCENE_OT_advanced_dolly_setup(Operator):
    """Create an advanced camera animation with lighting effects"""
    bl_idname = "scene.advanced_dolly_setup"
    bl_label = "Setup Camera Animation"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        try:
            props = context.scene.cam_light_props
            setup_advanced_camera_lighting(context, props)
            self.report({'INFO'}, "Camera and lighting setup complete. Press Alt+A to play.")
            return {'FINISHED'}
        except Exception as e:
            self.report({'ERROR'}, f"Error: {str(e)}")
            print(f"Error details: {str(e)}")
            return {'CANCELLED'}

# -----------------------------------------------------------------------------
# Helper Functions for Lighting Creation
# -----------------------------------------------------------------------------
def create_light(name, light_type, location, energy=1000, color=(1, 1, 1), size=1.0, rotation=(0, 0, 0)):
    """Create a light with specified parameters"""
    # Remove existing light if it exists
    if name in bpy.data.objects:
        bpy.data.objects.remove(bpy.data.objects[name], do_unlink=True)
    
    # Create new light data
    light_data = bpy.data.lights.new(name=name, type=light_type)
    light_data.energy = energy
    light_data.color = color
    
    if light_type == 'AREA':
        light_data.size = size
        light_data.shape = 'RECTANGLE'
        light_data.size_y = size * 0.5
    elif light_type == 'SPOT':
        light_data.spot_size = math.radians(45)  # 45 degrees spot angle
        light_data.spot_blend = 0.15  # Soft edge
        light_data.shadow_soft_size = size * 0.2
    elif light_type == 'POINT':
        light_data.shadow_soft_size = size * 0.2
    
    # Create light object
    light_obj = bpy.data.objects.new(name=name, object_data=light_data)
    
    # Link to scene
    bpy.context.collection.objects.link(light_obj)
    
    # Set location and rotation
    light_obj.location = location
    light_obj.rotation_euler = rotation
    
    return light_obj

def setup_three_point_lighting(center, distance=5.0):
    """Create three-point lighting around a center point"""
    lights = {}
    
    # Key Light (Main illumination)
    key_loc = (center[0] + distance * 0.7, center[1] - distance * 0.7, center[2] + distance * 0.5)
    key_rot = (math.radians(-30), math.radians(30), 0)
    lights['key'] = create_light("Key_Light", 'AREA', key_loc, energy=1500, 
                         color=(1.0, 0.95, 0.9), size=1.5, rotation=key_rot)
    
    # Fill Light (Softer, fills shadows)
    fill_loc = (center[0] - distance * 0.7, center[1] - distance * 0.5, center[2] + distance * 0.3)
    fill_rot = (math.radians(-15), math.radians(-40), 0)
    lights['fill'] = create_light("Fill_Light", 'AREA', fill_loc, energy=600, 
                         color=(0.9, 0.95, 1.0), size=2.0, rotation=fill_rot)
    
    # Back Light (Rim/separation light)
    back_loc = (center[0], center[1] + distance * 0.8, center[2] + distance * 0.7)
    back_rot = (math.radians(-45), 0, 0)
    lights['back'] = create_light("Back_Light", 'SPOT', back_loc, energy=1200, 
                         color=(1.0, 1.0, 1.0), size=0.5, rotation=back_rot)
    
    return lights

def setup_film_noir_lighting(center, distance=5.0):
    """Create film noir style lighting - dramatic, high contrast"""
    lights = {}
    
    # Main harsh light from above
    key_loc = (center[0] + distance * 0.3, center[1] - distance * 0.3, center[2] + distance * 0.8)
    key_rot = (math.radians(-60), math.radians(30), 0)
    lights['key'] = create_light("Key_Light", 'SPOT', key_loc, energy=2000, 
                         color=(1.0, 0.95, 0.9), size=0.5, rotation=key_rot)
    
    # Very subtle fill
    fill_loc = (center[0] - distance * 0.7, center[1] - distance * 0.5, center[2] + distance * 0.3)
    fill_rot = (math.radians(-15), math.radians(-40), 0)
    lights['fill'] = create_light("Fill_Light", 'AREA', fill_loc, energy=100, 
                         color=(0.8, 0.85, 1.0), size=3.0, rotation=fill_rot)
    
    # Dramatic back light
    back_loc = (center[0], center[1] + distance * 0.8, center[2] + distance * 0.7)
    back_rot = (math.radians(-45), 0, 0)
    lights['back'] = create_light("Back_Light", 'SPOT', back_loc, energy=1500, 
                         color=(1.0, 1.0, 1.0), size=0.3, rotation=back_rot)
    
    return lights

def setup_sunset_lighting(center, distance=5.0):
    """Create warm sunset lighting with orange main light and blue fill"""
    lights = {}
    
    # Warm "sun" light - orange
    key_loc = (center[0] + distance * 0.8, center[1] - distance * 0.2, center[2] + distance * 0.1)
    key_rot = (math.radians(-5), math.radians(20), 0)
    lights['key'] = create_light("Key_Light", 'SUN', key_loc, energy=2, 
                         color=(1.0, 0.6, 0.3), size=1.0, rotation=key_rot)
    
    # Blue "sky" fill light from above
    fill_loc = (center[0], center[1], center[2] + distance * 1.2)
    fill_rot = (math.radians(-90), 0, 0)
    lights['fill'] = create_light("Fill_Light", 'AREA', fill_loc, energy=400, 
                         color=(0.4, 0.6, 1.0), size=5.0, rotation=fill_rot)
    
    # Golden rim light
    back_loc = (center[0] - distance * 0.7, center[1] + distance * 0.7, center[2] + distance * 0.5)
    back_rot = (math.radians(-30), math.radians(-135), 0)
    lights['back'] = create_light("Back_Light", 'AREA', back_loc, energy=1000, 
                         color=(1.0, 0.8, 0.5), size=1.0, rotation=back_rot)
    
    return lights

def setup_scifi_lighting(center, distance=5.0):
    """Create sci-fi style lighting with blue/teal main light and accent colors"""
    lights = {}
    
    # Blue-teal main light
    key_loc = (center[0] + distance * 0.7, center[1] - distance * 0.7, center[2] + distance * 0.5)
    key_rot = (math.radians(-30), math.radians(30), 0)
    lights['key'] = create_light("Key_Light", 'AREA', key_loc, energy=1200, 
                         color=(0.5, 0.9, 1.0), size=1.5, rotation=key_rot)
    
    # Purple accent light
    fill_loc = (center[0] - distance * 0.7, center[1] - distance * 0.5, center[2] + distance * 0.3)
    fill_rot = (math.radians(-15), math.radians(-40), 0)
    lights['fill'] = create_light("Fill_Light", 'AREA', fill_loc, energy=800, 
                         color=(0.7, 0.5, 1.0), size=1.0, rotation=fill_rot)
    
    # Intense cyan rim light
    back_loc = (center[0], center[1] + distance * 0.8, center[2] + distance * 0.3)
    back_rot = (math.radians(-20), 0, 0)
    lights['back'] = create_light("Back_Light", 'SPOT', back_loc, energy=1500, 
                         color=(0.2, 1.0, 0.8), size=0.3, rotation=back_rot)
    
    # Extra small point lights for "tech" feel
    tech1_loc = (center[0] + distance * 0.5, center[1] + distance * 0.5, center[2] - distance * 0.3)
    lights['tech1'] = create_light("Tech_Light_1", 'POINT', tech1_loc, energy=200, 
                          color=(0.1, 0.8, 1.0), size=0.1)
    
    tech2_loc = (center[0] - distance * 0.5, center[1] + distance * 0.3, center[2] - distance * 0.4)
    lights['tech2'] = create_light("Tech_Light_2", 'POINT', tech2_loc, energy=150, 
                          color=(1.0, 0.2, 0.5), size=0.1)
    
    return lights

# -----------------------------------------------------------------------------
# Setup HDRI Environment
# -----------------------------------------------------------------------------
def setup_hdri_environment(context, hdri_path, strength=1.0):
    """Setup HDRI environment map with improved error handling and debugging"""
    
    # Make sure we're using Cycles or Eevee
    if context.scene.render.engine not in ['CYCLES', 'BLENDER_EEVEE']:
        context.scene.render.engine = 'CYCLES'
    
    # Get the world or create one if it doesn't exist
    world = context.scene.world
    if world is None:
        world = bpy.data.worlds.new("World")
        context.scene.world = world
    
    # Enable use of nodes
    world.use_nodes = True
    tree = world.node_tree
    
    # Clear existing nodes
    for node in tree.nodes:
        tree.nodes.remove(node)
    
    # Create nodes
    bg_node = tree.nodes.new(type="ShaderNodeBackground")
    mapping_node = tree.nodes.new(type="ShaderNodeMapping")
    tex_coord_node = tree.nodes.new(type="ShaderNodeTexCoord")
    env_node = tree.nodes.new(type="ShaderNodeTexEnvironment")
    output_node = tree.nodes.new(type="ShaderNodeOutputWorld")
    
    # Set node locations for better organization
    tex_coord_node.location = (-800, 0)
    mapping_node.location = (-600, 0)
    env_node.location = (-400, 0)
    bg_node.location = (-200, 0)
    output_node.location = (0, 0)
    
    # Link nodes
    tree.links.new(tex_coord_node.outputs["Generated"], mapping_node.inputs["Vector"])
    tree.links.new(mapping_node.outputs["Vector"], env_node.inputs["Vector"])
    tree.links.new(env_node.outputs["Color"], bg_node.inputs["Color"])
    tree.links.new(bg_node.outputs["Background"], output_node.inputs["Surface"])
    
    # Set background strength
    bg_node.inputs["Strength"].default_value = strength
    
    # Create a driver for Z rotation
    mapping_node.inputs["Rotation"].default_value[2] = 0.0
    
    # Load HDRI if path exists with better error handling
    if hdri_path and os.path.exists(hdri_path):
        try:
            # Get or load image
            if os.path.basename(hdri_path) in bpy.data.images:
                print(f"Using existing HDRI: {os.path.basename(hdri_path)}")
                img = bpy.data.images[os.path.basename(hdri_path)]
            else:
                print(f"Loading new HDRI: {hdri_path}")
                img = bpy.data.images.load(hdri_path)
                
            # Set image colorspace to Non-Color for HDR/EXR
            if hdri_path.lower().endswith(('.hdr', '.exr')):
                img.colorspace_settings.name = 'Non-Color'
                
            # Assign to environment node
            env_node.image = img
            
            # Set projection type to Equirectangular (important for HDRIs)
            env_node.projection = 'EQUIRECTANGULAR'
            
            print(f"HDRI setup successful: {hdri_path}")
            return mapping_node  # Return the mapping node for animation
            
        except Exception as e:
            print(f"Failed to load HDRI: {hdri_path}")
            print(f"Error details: {str(e)}")
    else:
        print(f"HDRI path does not exist or is empty: {hdri_path}")
    
    # Return the mapping node anyway (even if HDRI failed to load)
    return mapping_node

# -----------------------------------------------------------------------------
# Main Setup Function
# -----------------------------------------------------------------------------
def setup_advanced_camera_lighting(context, props):
    """Main function to set up camera and lighting"""
    scene = context.scene
    
    # Set FPS
    scene.render.fps = props.fps
    
    # Calculate total frames
    total_frames = props.anim_duration * props.fps
    scene.frame_start = 1
    scene.frame_end = total_frames
    
    # 1. Create or find dolly camera
    if "Dolly_Camera" not in bpy.data.objects:
        cam_data = bpy.data.cameras.new("Dolly_Camera")
        cam_obj = bpy.data.objects.new("Dolly_Camera", cam_data)
        scene.collection.objects.link(cam_obj)
    else:
        cam_obj = bpy.data.objects["Dolly_Camera"]
    
    # Set as active camera
    scene.camera = cam_obj
    
    # 2. Clear existing camera animation
    if cam_obj.animation_data and cam_obj.animation_data.action:
        bpy.data.actions.remove(cam_obj.animation_data.action)
    
    # 3. Calculate scene center
    objects = [obj for obj in scene.objects if obj.type == 'MESH' and obj.visible_get()]
    
    if not objects:
        # If no objects, use world origin
        center = (0, 0, 0)
    else:
        # Calculate center point of all visible objects
        center_x = sum(obj.location.x for obj in objects) / len(objects)
        center_y = sum(obj.location.y for obj in objects) / len(objects)
        center_z = sum(obj.location.z for obj in objects) / len(objects)
        center = (center_x, center_y, center_z)
    
    # 4. Create camera dolly animation
    distance = props.camera_distance
    height = props.camera_height
    
    # Determine starting angle based on user selection
    start_angle_map = {
        'FRONT': math.pi,      # 180 degrees
        'BACK': 0,             # 0 degrees
        'LEFT': math.pi / 2,   # 90 degrees
        'RIGHT': 3 * math.pi / 2  # 270 degrees
    }
    start_angle = start_angle_map[props.camera_start_position]
    
    # Clear any previous keyframes
    if cam_obj.animation_data:
        cam_obj.animation_data_clear()
    
    # Create camera animation with more keyframes for smoothness
    keyframe_step = max(1, total_frames // 60)  # At least 60 keyframes total
    
    for frame_idx, frame in enumerate(range(1, total_frames + 1, keyframe_step)):
        # Calculate position around circle
        angle = start_angle + (frame - 1) * (2 * math.pi / total_frames)
        
        x = center[0] + distance * math.sin(angle)
        y = center[1] + distance * math.cos(angle)
        z = center[2] + height  # Height above center
        
        # Position camera
        cam_obj.location = (x, y, z)
        
        # Point camera at center
        direction = mathutils.Vector(center) - mathutils.Vector((x, y, z))
        rot_quat = direction.to_track_quat('-Z', 'Y')
        cam_obj.rotation_euler = rot_quat.to_euler()
        
        # Keyframe location and rotation
        cam_obj.keyframe_insert(data_path="location", frame=frame)
        cam_obj.keyframe_insert(data_path="rotation_euler", frame=frame)
    
    # Set interpolation to BEZIER for smoother camera movement
    if cam_obj.animation_data and cam_obj.animation_data.action:
        for fcurve in cam_obj.animation_data.action.fcurves:
            for keyframe_point in fcurve.keyframe_points:
                keyframe_point.interpolation = 'BEZIER'
    
    # 5. Setup HDRI if enabled
    mapping_node = None
    if props.use_hdri and props.hdri_path:
        mapping_node = setup_hdri_environment(context, bpy.path.abspath(props.hdri_path), props.hdri_strength)
        
        # Setup HDRI rotation animation if enabled and node exists
        if props.hdri_rotation and mapping_node:
            # Animate rotation by keyframing the Z rotation of the mapping node
            rotation_z_fcurve = mapping_node.inputs['Rotation'].driver_add('default_value', 2)
            if rotation_z_fcurve:
                rotation_z_fcurve.driver.type = 'SCRIPTED'
                rotation_z_fcurve.driver.expression = f"(frame-1) * {props.hdri_rotation_speed / props.fps} * 0.01745"  # degrees to radians
                print("HDRI rotation animation setup via driver")
            else:
                print("Failed to create driver for HDRI rotation")
                
                # Fallback to keyframes if driver fails
                keyframe_step = max(1, total_frames // 30)
                for frame in range(1, total_frames + 1, keyframe_step):
                    rotation_value = ((frame - 1) * props.hdri_rotation_speed / props.fps) % 360
                    rotation_radians = math.radians(rotation_value)
                    mapping_node.inputs['Rotation'].default_value[2] = rotation_radians
                    mapping_node.inputs['Rotation'].keyframe_insert(data_path="default_value", index=2, frame=frame)
    
    # 6. Set up lighting presets if enabled
    if props.lighting_mode in ['PRESETS', 'BOTH']:
        # Collect enabled presets
        enabled_presets = []
        if props.use_three_point:
            enabled_presets.append("three_point")
        if props.use_film_noir:
            enabled_presets.append("film_noir")
        if props.use_sunset:
            enabled_presets.append("sunset")
        if props.use_scifi:
            enabled_presets.append("scifi")
        
        # If no presets are enabled, default to three_point
        if not enabled_presets:
            enabled_presets = ["three_point"]
        
        # Calculate frames per lighting setup
        frames_per_lighting = props.lighting_duration * props.fps
        
        # Initialize all possible lighting setups at frame 1 with visibility off
        # This ensures all lights exist for keyframing visibility
        lights_by_preset = {}
        
        # Setup all presets once so we have all the lights
        print("Creating all lighting setups...")
        lights_by_preset["three_point"] = setup_three_point_lighting(center, distance) 
        lights_by_preset["film_noir"] = setup_film_noir_lighting(center, distance)
        lights_by_preset["sunset"] = setup_sunset_lighting(center, distance)
        lights_by_preset["scifi"] = setup_scifi_lighting(center, distance)
        
        # Initially hide all lights
        for preset_name, lights in lights_by_preset.items():
            for light_name, light_obj in lights.items():
                light_obj.hide_render = True
                light_obj.hide_viewport = True
                light_obj.keyframe_insert(data_path="hide_render", frame=1)
                light_obj.keyframe_insert(data_path="hide_viewport", frame=1)
        
        # Now animate the lighting changes throughout the timeline
        print("Animating lighting changes...")
        current_frame = 1
        
        # Calculate how many lighting changes we'll have in the animation
        cycles_needed = math.ceil(total_frames / frames_per_lighting)
        
        for cycle in range(cycles_needed):
            # Loop through each enabled preset
            for preset_idx, preset_name in enumerate(enabled_presets):
                start_frame = current_frame
                end_frame = min(total_frames, current_frame + frames_per_lighting - 1)
                
                # Skip if we're past the total frames
                if start_frame > total_frames:
                    break
                
                print(f"Setting up {preset_name} from frame {start_frame} to {end_frame}")
                
                # Make current preset lights visible and others invisible
                for check_preset, lights in lights_by_preset.items():
                    for light_name, light_obj in lights.items():
                        # Whether this light should be visible in this segment
                        should_be_visible = (check_preset == preset_name)
                        
                        # Set visibility for start and end of segment
                        # Start frame - make the right lights visible
                        light_obj.hide_render = not should_be_visible
                        light_obj.hide_viewport = not should_be_visible
                        light_obj.keyframe_insert(data_path="hide_render", frame=start_frame)
                        light_obj.keyframe_insert(data_path="hide_viewport", frame=start_frame)
                        
                        # End frame +1 - prepare for next segment
                        if end_frame < total_frames:
                            light_obj.hide_render = True  # Hide all lights by default
                            light_obj.hide_viewport = True
                            light_obj.keyframe_insert(data_path="hide_render", frame=end_frame + 1)
                            light_obj.keyframe_insert(data_path="hide_viewport", frame=end_frame + 1)
                
                # Move to next segment
                current_frame = end_frame + 1
                
                # Stop if we've reached the end
                if current_frame > total_frames:
                    break
        
        # Set keyframe interpolation to CONSTANT for all light visibility fcurves
        for obj in bpy.data.objects:
            if obj.animation_data and obj.animation_data.action:
                for fcurve in obj.animation_data.action.fcurves:
                    if "hide" in fcurve.data_path:
                        for kfp in fcurve.keyframe_points:
                            kfp.interpolation = 'CONSTANT'
    
    # 7. Set camera view in viewport
    for area in context.screen.areas:
        if area.type == 'VIEW_3D':
            area.spaces[0].region_3d.view_perspective = 'CAMERA'
            # Ensure we're looking through the camera
            override = context.copy()
            override["area"] = area
            override["region"] = area.regions[-1]
            try:
                bpy.ops.view3d.view_camera(override)
            except:
                pass
            break

# -----------------------------------------------------------------------------
# UI Panels
# -----------------------------------------------------------------------------
class VIEW3D_PT_camera_lighting_panel(Panel):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"
    bl_label = "Camera & Lighting Animation"
    
    def draw(self, context):
        layout = self.layout
        props = context.scene.cam_light_props
        
        # Animation Section
        box = layout.box()
        box.label(text="Animation Settings:")
        row = box.row()
        row.prop(props, "fps")
        row.prop(props, "anim_duration")
        
        # Camera Settings
        box = layout.box()
        box.label(text="Camera Settings:")
        box.prop(props, "camera_distance")
        box.prop(props, "camera_height")
        box.prop(props, "camera_start_position")
        
        # Lighting Settings
        box = layout.box()
        box.label(text="Lighting Settings:")
        box.prop(props, "lighting_mode")
        box.prop(props, "lighting_duration")
        
        # Only show lighting checkboxes if presets are enabled
        if props.lighting_mode in ['PRESETS', 'BOTH']:
            box.label(text="Lighting Presets to Include:")
            row = box.row()
            row.prop(props, "use_three_point")
            row.prop(props, "use_film_noir")
            row = box.row()
            row.prop(props, "use_sunset")
            row.prop(props, "use_scifi")
        
        # HDRI Settings
        box = layout.box()
        box.label(text="HDRI Settings:")
        box.prop(props, "use_hdri")
        
        if props.use_hdri:
            box.prop(props, "hdri_path")
            box.prop(props, "hdri_strength")
            box.prop(props, "hdri_rotation")
            
            if props.hdri_rotation:
                box.prop(props, "hdri_rotation_speed")
        
        # Setup Button
        layout.separator()
        layout.operator("scene.advanced_dolly_setup")

# -----------------------------------------------------------------------------
# Registration
# -----------------------------------------------------------------------------
classes = (
    CameraLightingProperties,
    SCENE_OT_advanced_dolly_setup,
    VIEW3D_PT_camera_lighting_panel,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    
    bpy.types.Scene.cam_light_props = PointerProperty(type=CameraLightingProperties)

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    
    del bpy.types.Scene.cam_light_props

if __name__ == "__main__":
    # Unregister if already registered
    try:
        unregister()
    except:
        pass
    
    register()
    
    # Show message in the info area
    def show_message(self, context):
        self.layout.label(text="Camera & Lighting Animation panel added to Scene properties")
    
    bpy.context.window_manager.popup_menu(show_message, title="Fixed Script Loaded", icon="INFO")