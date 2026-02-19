#pragma once

class Shader;

class Object3D {
public:
    virtual void update(float dt) = 0;
    virtual void render(Shader& shader) = 0;
    virtual ~Object3D() = default;
};
