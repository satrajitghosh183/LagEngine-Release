import bpy
import math
import random
import mathutils

class SCENE_OT_interactive_lighting(bpy.types.Operator):
    """Add interactive lighting with viewport controls"""
    bl_idname = "scene.interactive_lighting"
    bl_label = "Add Interactive Lighting"
    bl_options = {'REGISTER', 'UNDO'}
    
    setup_type: bpy.props.EnumProperty(
        name="Lighting Setup",
        description="Type of lighting setup to create",
        items=[
            ('three_point', "Three Point", "Classic three-point lighting setup"),
            ('film_noir', "Film Noir", "High-contrast dramatic lighting"),
            ('studio', "Studio", "Soft, even studio lighting"),
            ('outdoor', "Outdoor", "Natural sunlight with fill"),
            ('sunset', "Sunset", "Warm sunset lighting"),
            ('sci_fi', "Sci-Fi", "Dramatic colored sci-fi lighting")
        ],
        default='three_point'
    )
    
    def _create_collection(self, name):
        """Create or get a collection for lights"""
        if name in bpy.data.collections:
            return bpy.data.collections[name]
        
        collection = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(collection)
        return collection
    
    def _get_scene_center_and_size(self):
        """Calculate the center and size of selected objects or all scene objects"""
        objects = bpy.context.selected_objects
        
        if not objects:
            objects = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
        
        if not objects:
            return (0, 0, 0), 5
        
        # Calculate bounding box
        min_x, min_y, min_z = float('inf'), float('inf'), float('inf')
        max_x, max_y, max_z = float('-inf'), float('-inf'), float('-inf')
        
        for obj in objects:
            matrix = obj.matrix_world
            if obj.type == 'MESH':
                for v in obj.bound_box:
                    v_world = matrix @ mathutils.Vector(v)
                    min_x = min(min_x, v_world.x)
                    min_y = min(min_y, v_world.y)
                    min_z = min(min_z, v_world.z)
                    max_x = max(max_x, v_world.x)
                    max_y = max(max_y, v_world.y)
                    max_z = max(max_z, v_world.z)
        
        # Calculate center and radius
        center = ((min_x + max_x) / 2, (min_y + max_y) / 2, (min_z + max_z) / 2)
        size = max(max_x - min_x, max_y - min_y, max_z - min_z) / 2
        
        return center, size
    
    def _create_control_empty(self, name, location):
        """Create an empty object with custom properties to control lights"""
        empty = bpy.data.objects.new(name, None)
        empty.empty_display_type = 'SPHERE'
        empty.empty_display_size = 0.2
        bpy.context.scene.collection.objects.link(empty)
        empty.location = location
        
        # Add custom properties for lighting control
        empty["key_intensity"] = 1000.0
        empty["fill_intensity"] = 300.0
        empty["back_intensity"] = 600.0
        empty["key_color"] = [1.0, 0.95, 0.9]
        empty["fill_color"] = [0.9, 0.95, 1.0]
        empty["back_color"] = [1.0, 1.0, 1.0]
        
        # Make the properties visible and animatable in the UI
        prop_names = ["key_intensity", "fill_intensity", "back_intensity", 
                     "key_color", "fill_color", "back_color"]
        
        for prop_name in prop_names:
            prop = empty.id_properties_ui(prop_name)
            prop.update(min=0.0)
            if "intensity" in prop_name:
                prop.update(max=5000.0)
                prop.update(soft_min=0.0, soft_max=2000.0)
            elif "color" in prop_name:
                prop.update(subtype='COLOR')
        
        return empty
    
    def _create_area_light(self, name, location, rotation, size, color, energy, collection):
        """Create an area light"""
        light_data = bpy.data.lights.new(name=name, type='AREA')
        light_obj = bpy.data.objects.new(name=name, object_data=light_data)
        
        collection.objects.link(light_obj)
        
        light_obj.location = location
        light_obj.rotation_euler = rotation
        
        light_data.size = size[0]
        light_data.size_y = size[1]
        light_data.color = color
        light_data.energy = energy
        
        light_data.use_contact_shadow = True
        light_data.contact_shadow_distance = 2.0
        light_data.use_shadow = True
        
        return light_obj
    
    def _create_spot_light(self, name, location, target, size, color, energy, spot_size=30, spot_blend=0.2, collection=None):
        """Create a spot light"""
        light_data = bpy.data.lights.new(name=name, type='SPOT')
        light_obj = bpy.data.objects.new(name=name, object_data=light_data)
        
        collection.objects.link(light_obj)
        
        light_obj.location = location
        
        # Point to target
        direction = mathutils.Vector(target) - mathutils.Vector(location)
        rot_quat = direction.to_track_quat('-Z', 'Y')
        light_obj.rotation_euler = rot_quat.to_euler()
        
        light_data.shadow_soft_size = size
        light_data.color = color
        light_data.energy = energy
        light_data.spot_size = math.radians(spot_size)
        light_data.spot_blend = spot_blend
        
        light_data.use_contact_shadow = True
        light_data.contact_shadow_distance = 2.0
        light_data.use_shadow = True
            
        return light_obj
    
    def _create_sun_light(self, name, location, target, color, energy, angle=1.0, collection=None):
        """Create a sun light"""
        light_data = bpy.data.lights.new(name=name, type='SUN')
        light_obj = bpy.data.objects.new(name=name, object_data=light_data)
        
        collection.objects.link(light_obj)
        
        light_obj.location = location
        
        # Point to target
        direction = mathutils.Vector(target) - mathutils.Vector(location)
        rot_quat = direction.to_track_quat('-Z', 'Y')
        light_obj.rotation_euler = rot_quat.to_euler()
        
        light_data.color = color
        light_data.energy = energy
        light_data.angle = math.radians(angle)
        
        return light_obj
    
    def _add_driver(self, obj, data_path, control_obj, property_name, influence=1.0):
        """Add a driver connecting a light property to a control property"""
        if obj.animation_data is None:
            obj.animation_data_create()
            
        # Create driver
        driver = obj.animation_data.drivers.new(data_path)
        
        # Create a variable
        var = driver.driver.variables.new()
        var.name = "control"
        var.type = 'SINGLE_PROP'
        
        # Target the control object's property
        target = var.targets[0]
        target.id = control_obj
        target.data_path = f'["{property_name}"]'
        
        # Set up the driver expression
        if isinstance(influence, (float, int)) and influence != 1.0:
            driver.driver.expression = f"control * {influence}"
        else:
            driver.driver.expression = "control"
            
    def _add_color_drivers(self, light_obj, control_obj, property_name):
        """Add drivers for RGB color components"""
        if light_obj.data.animation_data is None:
            light_obj.data.animation_data_create()
            
        # Add drivers for each color component
        for i, component in enumerate(['r', 'g', 'b']):
            driver = light_obj.data.animation_data.drivers.new(f'color.{component}')
            
            # Create variable
            var = driver.driver.variables.new()
            var.name = "control"
            var.type = 'SINGLE_PROP'
            
            # Target control property array element
            target = var.targets[0]
            target.id = control_obj
            target.data_path = f'["{property_name}"][{i}]'
            
            # Set expression
            driver.driver.expression = "control"
    
    def setup_three_point_lighting(self, center, radius, collection, control_empty):
        """Create a classic three-point lighting setup with interactive controls"""
        # Adjust radius for better lighting
        working_radius = radius * 2
        
        # Key light position (front-right-top)
        key_angle = math.radians(45)
        key_height = center[2] + working_radius * 0.8
        key_distance = working_radius * 1.5
        key_x = center[0] + math.sin(key_angle) * key_distance
        key_y = center[1] - math.cos(key_angle) * key_distance
        key_pos = (key_x, key_y, key_height)
        
        # Fill light position (front-left-middle)
        fill_angle = math.radians(-30)
        fill_height = center[2] + working_radius * 0.3
        fill_distance = working_radius * 1.3
        fill_x = center[0] + math.sin(fill_angle) * fill_distance
        fill_y = center[1] - math.cos(fill_angle) * fill_distance
        fill_pos = (fill_x, fill_y, fill_height)
        
        # Back light position (behind-top)
        back_angle = math.radians(180)
        back_height = center[2] + working_radius * 1.5
        back_distance = working_radius * 1.0
        back_x = center[0] + math.sin(back_angle) * back_distance
        back_y = center[1] - math.cos(back_angle) * back_distance
        back_pos = (back_x, back_y, back_height)
        
        # Create key light
        key_light = self._create_area_light(
            name="Key_Light",
            location=key_pos,
            rotation=(math.radians(50), 0, math.radians(-45)),
            size=(working_radius/2, working_radius/3),
            color=(1.0, 0.95, 0.9),  # Default color, will be driven
            energy=1000,  # Default energy, will be driven
            collection=collection
        )
        
        # Create fill light
        fill_light = self._create_area_light(
            name="Fill_Light",
            location=fill_pos,
            rotation=(math.radians(30), 0, math.radians(30)),
            size=(working_radius/2, working_radius/2),
            color=(0.9, 0.95, 1.0),  # Default color, will be driven
            energy=300,  # Default energy, will be driven
            collection=collection
        )
        
        # Create back light
        back_light = self._create_spot_light(
            name="Back_Light",
            location=back_pos,
            target=center,
            size=working_radius/10,
            color=(1.0, 1.0, 1.0),  # Default color, will be driven
            energy=600,  # Default energy, will be driven
            spot_size=45,
            spot_blend=0.15,
            collection=collection
        )
        
        # Add drivers to connect lights to control object
        self._add_driver(key_light.data, 'energy', control_empty, 'key_intensity')
        self._add_driver(fill_light.data, 'energy', control_empty, 'fill_intensity')
        self._add_driver(back_light.data, 'energy', control_empty, 'back_intensity')
        
        # Add color drivers
        self._add_color_drivers(key_light, control_empty, 'key_color')
        self._add_color_drivers(fill_light, control_empty, 'fill_color')
        self._add_color_drivers(back_light, control_empty, 'back_color')
        
        return [key_light, fill_light, back_light]
    
    def setup_film_noir_lighting(self, center, radius, collection, control_empty):
        """Create a dramatic film noir style lighting with interactive controls"""
        # Adjust radius for better lighting
        working_radius = radius * 2
        
        # Key light position (high side angle)
        key_angle = math.radians(70)
        key_height = center[2] + working_radius * 1.2
        key_distance = working_radius * 1.8
        key_x = center[0] + math.sin(key_angle) * key_distance
        key_y = center[1] - math.cos(key_angle) * key_distance
        key_pos = (key_x, key_y, key_height)
        
        # Fill light position (very dim, opposite side)
        fill_angle = math.radians(-60)
        fill_height = center[2] + working_radius * 0.2
        fill_distance = working_radius * 2.0
        fill_x = center[0] + math.sin(fill_angle) * fill_distance
        fill_y = center[1] - math.cos(fill_angle) * fill_distance
        fill_pos = (fill_x, fill_y, fill_height)
        
        # Create key light (window-like)
        key_light = self._create_area_light(
            name="Noir_Key",
            location=key_pos,
            rotation=(math.radians(40), 0, math.radians(-70)),
            size=(working_radius/3, working_radius),
            color=(1.0, 0.97, 0.9),  # Slightly warm, will be driven
            energy=1500,  # Will be driven
            collection=collection
        )
        
        # Create fill light (very dim)
        fill_light = self._create_area_light(
            name="Noir_Fill",
            location=fill_pos,
            rotation=(math.radians(20), 0, math.radians(60)),
            size=(working_radius, working_radius),
            color=(0.8, 0.9, 1.0),  # Cool fill, will be driven
            energy=45,  # Will be driven
            collection=collection
        )
        
        # Add edge light
        edge_light = self._create_spot_light(
            name="Noir_Edge",
            location=(center[0], center[1], center[2] + working_radius * 2),
            target=center,
            size=working_radius/12,
            color=(1.0, 1.0, 1.0),  # Will be driven
            energy=180,  # Will be driven
            spot_size=30,
            spot_blend=0.1,
            collection=collection
        )
        
        # Add drivers with adjustment factors for the noir look
        self._add_driver(key_light.data, 'energy', control_empty, 'key_intensity', 1.5)
        self._add_driver(fill_light.data, 'energy', control_empty, 'fill_intensity', 0.15)
        self._add_driver(edge_light.data, 'energy', control_empty, 'back_intensity', 0.3)
        
        # Add color drivers
        self._add_color_drivers(key_light, control_empty, 'key_color')
        self._add_color_drivers(fill_light, control_empty, 'fill_color')
        self._add_color_drivers(edge_light, control_empty, 'back_color')
        
        return [key_light, fill_light, edge_light]
    
    def setup_sunset_lighting(self, center, radius, collection, control_empty):
        """Create warm sunset lighting with interactive controls"""
        # Adjust radius for better lighting
        working_radius = radius * 2
        
        # Main sun light position (low angle from horizon)
        sun_angle = math.radians(-130)  # Coming from behind and to the side
        sun_height = center[2] + working_radius * 0.2  # Low height
        sun_distance = working_radius * 2.5
        sun_x = center[0] + math.sin(sun_angle) * sun_distance
        sun_y = center[1] - math.cos(sun_angle) * sun_distance
        sun_pos = (sun_x, sun_y, sun_height)
        
        # Ambient fill (sky light, cool blue)
        fill_height = center[2] + working_radius * 1.5  # Coming from above
        fill_pos = (center[0], center[1] - working_radius * 1.0, fill_height)
        
        # Bounce light (warm from ground reflection)
        bounce_angle = math.radians(20)  # Coming from front-side low
        bounce_height = center[2] - working_radius * 0.2  # Below center
        bounce_distance = working_radius * 1.5
        bounce_x = center[0] + math.sin(bounce_angle) * bounce_distance
        bounce_y = center[1] - math.cos(bounce_angle) * bounce_distance
        bounce_pos = (bounce_x, bounce_y, bounce_height)
        
        # Create sun light
        sun_light = self._create_sun_light(
            name="Sunset_Sun",
            location=sun_pos,
            target=center,
            color=(1.0, 0.6, 0.3),  # Orange-red sunset color, will be driven
            energy=1200,  # Will be driven
            angle=2.0,
            collection=collection
        )
        
        # Create sky fill light (ambient)
        sky_light = self._create_area_light(
            name="Sunset_Sky",
            location=fill_pos,
            rotation=(math.radians(90), 0, 0),  # Pointing down
            size=(working_radius*2, working_radius*2),
            color=(0.5, 0.7, 1.0),  # Cool blue, will be driven
            energy=150,  # Will be driven
            collection=collection
        )
        
        # Create bounce light
        bounce_light = self._create_area_light(
            name="Sunset_Bounce",
            location=bounce_pos,
            rotation=(math.radians(-20), 0, math.radians(-20)),
            size=(working_radius, working_radius/2),
            color=(1.0, 0.8, 0.5),  # Warm bounce, will be driven
            energy=300,  # Will be driven
            collection=collection
        )
        
        # Add drivers with adjustment factors
        self._add_driver(sun_light.data, 'energy', control_empty, 'key_intensity', 1.2)
        self._add_driver(sky_light.data, 'energy', control_empty, 'fill_intensity', 0.5)
        self._add_driver(bounce_light.data, 'energy', control_empty, 'back_intensity', 0.5)
        
        # Add color drivers
        control_empty["key_color"] = [1.0, 0.6, 0.3]  # Set default sunset colors
        control_empty["fill_color"] = [0.5, 0.7, 1.0]
        control_empty["back_color"] = [1.0, 0.8, 0.5]
        
        self._add_color_drivers(sun_light, control_empty, 'key_color')
        self._add_color_drivers(sky_light, control_empty, 'fill_color')
        self._add_color_drivers(bounce_light, control_empty, 'back_color')
        
        return [sun_light, sky_light, bounce_light]
    
    def setup_sci_fi_lighting(self, center, radius, collection, control_empty):
        """Create dramatic sci-fi lighting with interactive controls"""
        # Adjust radius for better lighting
        working_radius = radius * 2
        
        # Primary light from below (dramatic uplighting)
        key_angle = math.radians(0)  # Front
        key_height = center[2] - working_radius * 0.3  # Below
        key_distance = working_radius * 1.2
        key_x = center[0] + math.sin(key_angle) * key_distance
        key_y = center[1] - math.cos(key_angle) * key_distance
        key_pos = (key_x, key_y, key_height)
        
        # Rim light 1 (left side)
        rim1_angle = math.radians(-100)
        rim1_height = center[2] + working_radius * 0.5
        rim1_distance = working_radius * 1.5
        rim1_x = center[0] + math.sin(rim1_angle) * rim1_distance
        rim1_y = center[1] - math.cos(rim1_angle) * rim1_distance
        rim1_pos = (rim1_x, rim1_y, rim1_height)
        
        # Rim light 2 (right side)
        rim2_angle = math.radians(100)
        rim2_height = center[2] + working_radius * 0.5
        rim2_distance = working_radius * 1.5
        rim2_x = center[0] + math.sin(rim2_angle) * rim2_distance
        rim2_y = center[1] - math.cos(rim2_angle) * rim2_distance
        rim2_pos = (rim2_x, rim2_y, rim2_height)
        
        # Create key light (dramatic up-light)
        key_light = self._create_area_light(
            name="SciFi_Key",
            location=key_pos,
            rotation=(math.radians(-60), 0, 0),  # Pointing up
            size=(working_radius/2, working_radius/4),
            color=(0.2, 0.8, 1.0),  # Cyan/blue, will be driven
            energy=800,  # Will be driven
            collection=collection
        )
        
        # Create rim light 1
        rim1_light = self._create_spot_light(
            name="SciFi_Rim1",
            location=rim1_pos,
            target=center,
            size=working_radius/15,
            color=(1.0, 0.2, 0.8),  # Magenta, will be driven
            energy=600,  # Will be driven
            spot_size=25,
            spot_blend=0.1,
            collection=collection
        )
        
        # Create rim light 2
        rim2_light = self._create_spot_light(
            name="SciFi_Rim2",
            location=rim2_pos,
            target=center,
            size=working_radius/15,
            color=(0.2, 1.0, 0.5),  # Green, will be driven
            energy=500,  # Will be driven
            spot_size=25,
            spot_blend=0.1,
            collection=collection
        )
        
        # Add drivers with adjustment factors
        self._add_driver(key_light.data, 'energy', control_empty, 'key_intensity', 0.8)
        self._add_driver(rim1_light.data, 'energy', control_empty, 'fill_intensity', 2.0)
        self._add_driver(rim2_light.data, 'energy', control_empty, 'back_intensity', 0.8)
        
        # Add color drivers
        control_empty["key_color"] = [0.2, 0.8, 1.0]  # Set default sci-fi colors
        control_empty["fill_color"] = [1.0, 0.2, 0.8]
        control_empty["back_color"] = [0.2, 1.0, 0.5]
        
        self._add_color_drivers(key_light, control_empty, 'key_color')
        self._add_color_drivers(rim1_light, control_empty, 'fill_color')
        self._add_color_drivers(rim2_light, control_empty, 'back_color')
        
        return [key_light, rim1_light, rim2_light]
        
    def execute(self, context):
        # Create or get lighting collection
        collection = self._create_collection("Cinematic_Lighting")
        
        # Clear existing lights in collection
        for obj in collection.objects:
            bpy.data.objects.remove(obj, do_unlink=True)
        
        # Also remove any existing control objects
        for obj in bpy.data.objects:
            if obj.name.startswith("Lighting_Control"):
                bpy.data.objects.remove(obj, do_unlink=True)
        
        # Calculate scene center and size
        center, radius = self._get_scene_center_and_size()
        
        # Create control empty slightly offset from center
        control_empty = self._create_control_empty(
            "Lighting_Control", 
            (center[0], center[1] - radius * 0.5, center[2] + radius * 0.5)
        )
        
        # Create lighting based on selected setup
        if self.setup_type == 'three_point':
            self.setup_three_point_lighting(center, radius, collection, control_empty)
        elif self.setup_type == 'film_noir':
            self.setup_film_noir_lighting(center, radius, collection, control_empty)
        elif self.setup_type == 'sunset':
            self.setup_sunset_lighting(center, radius, collection, control_empty)
        elif self.setup_type == 'sci_fi':
            self.setup_sci_fi_lighting(center, radius, collection, control_empty)
        # Additional setups would be added here
        
        # Select the control empty so it's easy to find
        bpy.ops.object.select_all(action='DESELECT')
        control_empty.select_set(True)
        context.view_layer.objects.active = control_empty
        
        # Add a helpful message
        self.report({'INFO'}, "Lighting created! Adjust the lighting using the Lighting_Control object's properties.")
        
        return {'FINISHED'}


class SCENE_PT_interactive_lighting(bpy.types.Panel):
    bl_label = "Interactive Lighting"
    bl_idname = "SCENE_PT_interactive_lighting"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"
    
    def draw(self, context):
        layout = self.layout
        
        layout.label(text="Add Interactive Lighting:")
        
        # Three point button
        row = layout.row()
        row.scale_y = 1.5
        row.operator("scene.interactive_lighting", text="Three Point Lighting").setup_type = 'three_point'
        
        # Film noir button
        row = layout.row()
        row.scale_y = 1.5
        row.operator("scene.interactive_lighting", text="Film Noir Lighting").setup_type = 'film_noir'
        
        # Sunset button
        row = layout.row()
        row.scale_y = 1.5
        row.operator("scene.interactive_lighting", text="Sunset Lighting").setup_type = 'sunset'
        
        # Sci-Fi button
        row = layout.row()
        row.scale_y = 1.5
        row.operator("scene.interactive_lighting", text="Sci-Fi Lighting").setup_type = 'sci_fi'
        
        # Instructions
        box = layout.box()
        box.label(text="How to use:")
        box.label(text="1. Select a lighting style above")
        box.label(text="2. Find the 'Lighting_Control' object")
        box.label(text="3. Adjust properties in the sidebar (N)")


def register():
    bpy.utils.register_class(SCENE_OT_interactive_lighting)
    bpy.utils.register_class(SCENE_PT_interactive_lighting)

def unregister():
    bpy.utils.unregister_class(SCENE_OT_interactive_lighting)
    bpy.utils.unregister_class(SCENE_PT_interactive_lighting)

if __name__ == "__main__":
    register()

