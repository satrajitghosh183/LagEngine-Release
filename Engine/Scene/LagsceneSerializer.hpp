#pragma once

#include "../Core/Base.hpp"
#include "../Core/UUID.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <fstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace GameEngine {

    class Scene;

    /**
     * LagsceneSerializer — Godot-inspired text format for LAG Engine scenes.
     *
     * Format example:
     *
     *   [lagscene format=1 uid="uid://abc123"]
     *
     *   [ext_resource type="Mesh" path="res://meshes/cube.obj" id="1_mesh"]
     *   [ext_resource type="Material" uid="uid://def456" id="2_mat"]
     *
     *   [sub_resource type="SpriteFrames" id="SpriteFrames_1"]
     *   frames = [1, 2, 3]
     *
     *   [node name="Player" type="Node3D"]
     *   transform = Transform(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0)
     *
     *   [node name="Mesh" type="MeshInstance" parent="Player"]
     *   mesh = ExtResource("1_mesh")
     *   material = ExtResource("2_mat")
     *
     * Round-trip safe: load + save produces identical output.
     */
    class LagsceneSerializer {
    public:
        // Typed property value that can appear after `=`
        struct Property {
            enum class Kind {
                Null, Bool, Int, Float, String,
                Vector2, Vector3, Vector4, Quat, Transform,
                ExtRef, SubRef, UIDRef,
                Array, Dict
            };

            Kind Type = Kind::Null;
            bool BoolValue = false;
            long long IntValue = 0;
            double FloatValue = 0.0;
            std::string StringValue;
            std::vector<float> VectorValue; // used for Vector2/3/4/Quat
            std::vector<float> TransformValue; // 12 floats for 3x4 transform
            std::string RefValue; // ext/sub/uid reference target
            std::vector<Property> ArrayItems;
            // std::unordered_map can't hold an incomplete value type, so a
            // vector of pairs is used to make `Property` recursive-safe.
            std::vector<std::pair<std::string, Property>> DictItems;

            std::string ToText() const;
            static Property FromText(const std::string& s, int& outErrors);
        };

        struct ExtResource {
            std::string Type;
            std::string Path;
            std::string UID;
            std::string ID;
        };

        struct SubResource {
            std::string Type;
            std::string ID;
            std::unordered_map<std::string, Property> Properties;
        };

        struct NodeEntry {
            std::string Name;
            std::string Type;
            std::string Parent;      // empty == root
            std::string InstancePath; // for prefab-instance nodes
            std::unordered_map<std::string, Property> Properties;
        };

        struct Document {
            int FormatVersion = 1;
            std::string UID;
            std::vector<ExtResource> ExtResources;
            std::vector<SubResource> SubResources;
            std::vector<NodeEntry> Nodes;
        };

        // Parsing / serialization
        static bool Parse(const std::string& source, Document& out, std::string* error = nullptr);
        static std::string Write(const Document& doc);

        // File I/O convenience
        static bool LoadFile(const std::string& path, Document& out, std::string* error = nullptr);
        static bool SaveFile(const std::string& path, const Document& doc);

        // Scene bridge — convert to/from engine Scene
        static bool BuildSceneFromDoc(const Document& doc, Scene& outScene);
        static Document BuildDocFromScene(const Scene& scene);
    };

}
