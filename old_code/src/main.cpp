// // Single-file 3D Character Controller (C++17 + OpenGL + GLFW + GLEW + GLM)
// //
// // Features:
// // - One-file, self-contained shaders and meshes
// // - Sphere character with gravity, collisions vs static AABBs (boxes)
// // - Rigid-body style response: normal impulse (restitution) + simple friction
// // - Ground detection, coyote time, jump buffer, variable jump height
// // - Ground vs air acceleration, sprint, air control, drag
// // - Sub-stepped physics to prevent tunneling at high speeds
// // - Third-person chase camera with mouse look
// // - Auto-generated test level (ground plane + platforms + a step-ramp)
// // - Colors: sky blue background, bright green floor, gray platforms, cyan/teal player
// //
// // Build (Linux):
// //   g++ -std=c++17 main.cpp -O2 -lglfw -lGLEW -lGL -ldl -lpthread -lX11 -lXrandr -lXi -o character
// //
// // Build (Windows, MinGW):
// //   g++ -std=gnu++17 main.cpp -O2 -lglfw3 -lglew32 -lopengl32 -lgdi32 -lwinmm -o character.exe
// //
// // Build (Windows, MSVC Developer Prompt):
// //   cl /std:c++17 /O2 main.cpp /I"path\\to\\glm" /I"path\\to\\glew\\include" /I"path\\to\\glfw\\include" ^
// //      /link /LIBPATH:"path\\to\\glew\\lib" /LIBPATH:"path\\to\\glfw\\lib" opengl32.lib glew32.lib glfw3.lib gdi32.lib user32.lib shell32.lib
// //
// // Build (macOS, Homebrew):
// //   clang++ -std=c++17 main.cpp -O2 -I/opt/homebrew/include -L/opt/homebrew/lib \
// //           -lglfw -lglew -framework OpenGL -o character
// //
// // Notes:
// // - Requires: GLFW, GLEW, GLM. (Ubuntu: sudo apt install libglfw3-dev libglew-dev libglm-dev)
// // - If using WSL2, ensure OpenGL GUI forwarding is set up.
// //
// // ---------------------------------------------------------------------------

// #include <cstdio>
// #include <cstdlib>
// #include <cmath>
// #include <vector>
// #include <string>
// #include <algorithm>
// #include <chrono>

// #include <GL/glew.h>
// #include <GLFW/glfw3.h>

// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE 0
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>

// // -------------------------------------------------------------
// // Small utilities
// // -------------------------------------------------------------
// static void die(const char* msg) { std::fprintf(stderr, "%s\n", msg); std::exit(EXIT_FAILURE); }

// static const char* kVert = R"GLSL(
// #version 330 core
// layout(location=0) in vec3 aPos;
// uniform mat4 uMVP;
// void main(){ gl_Position = uMVP * vec4(aPos,1.0); }
// )GLSL";

// static const char* kFrag = R"GLSL(
// #version 330 core
// out vec4 FragColor;
// uniform vec3 uColor;
// void main(){ FragColor = vec4(uColor,1.0); }
// )GLSL";

// GLuint makeProgram(const char* vs, const char* fs) {
//     auto compile = [](GLenum type, const char* src)->GLuint{
//         GLuint id = glCreateShader(type);
//         glShaderSource(id,1,&src,nullptr);
//         glCompileShader(id);
//         GLint ok; glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
//         if(!ok){ char log[2048]; glGetShaderInfoLog(id,2048,nullptr,log); std::fprintf(stderr,"Shader error:\n%s\n",log); std::exit(1); }
//         return id;
//     };
//     GLuint v=compile(GL_VERTEX_SHADER,vs), f=compile(GL_FRAGMENT_SHADER,fs);
//     GLuint p=glCreateProgram();
//     glAttachShader(p,v); glAttachShader(p,f);
//     glLinkProgram(p);
//     GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
//     if(!ok){ char log[2048]; glGetProgramInfoLog(p,2048,nullptr,log); std::fprintf(stderr,"Link error:\n%s\n",log); std::exit(1); }
//     glDeleteShader(v); glDeleteShader(f);
//     return p;
// }

// // -------------------------------------------------------------
// // Simple meshes (unit cube)
// // -------------------------------------------------------------
// struct Mesh {
//     GLuint vao=0, vbo=0, ebo=0;
//     GLsizei indexCount=0;
// };

// Mesh makeUnitCube() {
//     Mesh m;
//     // unit cube centered at origin, size 1
//     float v[] = {
//         -0.5f,-0.5f,-0.5f,  // 0
//          0.5f,-0.5f,-0.5f,  // 1
//          0.5f, 0.5f,-0.5f,  // 2
//         -0.5f, 0.5f,-0.5f,  // 3
//         -0.5f,-0.5f, 0.5f,  // 4
//          0.5f,-0.5f, 0.5f,  // 5
//          0.5f, 0.5f, 0.5f,  // 6
//         -0.5f, 0.5f, 0.5f   // 7
//     };
//     unsigned int idx[] = {
//         0,1,2, 2,3,0, // back
//         4,5,6, 6,7,4, // front
//         0,4,7, 7,3,0, // left
//         1,5,6, 6,2,1, // right
//         3,2,6, 6,7,3, // top
//         0,1,5, 5,4,0  // bottom
//     };
//     glGenVertexArrays(1,&m.vao);
//     glGenBuffers(1,&m.vbo);
//     glGenBuffers(1,&m.ebo);
//     glBindVertexArray(m.vao);
//     glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
//     glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
//     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);
//     glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_STATIC_DRAW);
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
//     glBindVertexArray(0);
//     m.indexCount = (GLsizei)(sizeof(idx)/sizeof(idx[0]));
//     return m;
// }

// // -------------------------------------------------------------
// // Math & physics helpers
// // -------------------------------------------------------------
// struct AABB { glm::vec3 mn, mx; }; // axis-aligned box

// static inline float clampf(float x, float a, float b){ return std::max(a, std::min(b, x)); }
// static inline glm::vec3 clampv3(glm::vec3 v, glm::vec3 a, glm::vec3 b){
//     return glm::vec3(clampf(v.x,a.x,b.x), clampf(v.y,a.y,b.y), clampf(v.z,a.z,b.z));
// }
// static inline glm::vec3 closestPointOnAABB(const glm::vec3& p, const AABB& b){
//     return clampv3(p, b.mn, b.mx);
// }

// // Resolves sphere-vs-AABB penetration. Returns true if overlapped; outNormal is the push-out direction (normalized).
// static bool resolveSphereAABB(glm::vec3& sphereCenter, float radius, const AABB& box, glm::vec3& outNormal, float& outDepth) {
//     glm::vec3 q = closestPointOnAABB(sphereCenter, box);
//     glm::vec3 d = sphereCenter - q;
//     float dist2 = glm::dot(d,d);
//     if(dist2 > radius*radius) return false;
//     float dist = std::sqrt(std::max(1e-12f, dist2));
//     if(dist > 1e-6f){
//         outNormal = d / dist;
//         outDepth = radius - dist;
//     }else{
//         // sphere center exactly at closest point; pick the smallest-distance face normal
//         glm::vec3 centerToMin = sphereCenter - box.mn;
//         glm::vec3 maxToCenter = box.mx - sphereCenter;
//         float minPen = centerToMin.x; int axis=0; float sign=-1.f;
//         if(centerToMin.y < minPen){ minPen=centerToMin.y; axis=1; sign=-1.f; }
//         if(centerToMin.z < minPen){ minPen=centerToMin.z; axis=2; sign=-1.f; }
//         if(maxToCenter.x < minPen){ minPen=maxToCenter.x; axis=0; sign=+1.f; }
//         if(maxToCenter.y < minPen){ minPen=maxToCenter.y; axis=1; sign=+1.f; }
//         if(maxToCenter.z < minPen){ minPen=maxToCenter.z; axis=2; sign=+1.f; }
//         outNormal = glm::vec3(0);
//         outNormal[axis] = sign;
//         outDepth = radius;
//     }
//     // push the sphere out
//     sphereCenter += outNormal * outDepth;
//     return true;
// }

// // -------------------------------------------------------------
// // Controller config/state
// // -------------------------------------------------------------
// struct ControllerConfig {
//     float walkSpeed = 6.0f;
//     float sprintSpeed = 9.0f;
//     float groundAccel = 45.0f;
//     float airAccel = 12.0f;
//     float maxAirSpeed = 8.0f;
  
//     float gravity = 9.81f;
//     float jumpHeight = 1.25f;
//     float coyoteTime = 0.12f;
//     float jumpBufferTime = 0.16f;
//     float fallGravityMult = 2.5f;
//     float lowJumpGravityMult = 2.0f;

//     float groundDrag = 8.0f;
//     float airDrag = 0.12f;

//     float restitution = 0.05f;   // bounciness for collisions
//     float friction   = 0.20f;    // tangential damping on contact

//     float radius = 0.45f;        // sphere radius
//     float eyeHeight = 0.9f;   
//     // --- Add these in ControllerConfig ---
// float sprintAirAccelMult      = 1.25f; // more steering while sprinting in air
// float sprintMaxAirSpeedMult   = 1.25f; // higher air top speed when sprinting
// float sprintJumpBoost         = 1.25f; // higher jump if Shift held at jump moment
//    // visual lift when rendering player cube
// };

// struct ControllerState {
//     glm::vec3 pos{0,2.0f,0};
//     glm::vec3 vel{0,0,0};
//     bool grounded=false;
//     glm::vec3 groundNormal{0,1,0};
//     float timeSinceGrounded=1e3f;
//     float timeSinceJumpPressed=1e3f;
//     bool jumpHeld=false;
// };

// struct CameraState {
//     float yaw=0.0f, pitch=10.0f;
//     float dist=6.0f, height=2.0f;
//     bool cursorLocked=true;
//     glm::vec3 targetPos;
// };

// // -------------------------------------------------------------
// // Level generation
// // -------------------------------------------------------------
// static void addBox(std::vector<AABB>& level, glm::vec3 center, glm::vec3 halfSize){
//     level.push_back({center-halfSize, center+halfSize});
// }

// // --- New helpers for richer geometry ----------------------------------------
// static void addWall(std::vector<AABB>& L, glm::vec3 a, glm::vec3 b, float thickness, float height)
// {
//     // Build a long wall (axis-aligned): we assume either x or z differs (not both).
//     glm::vec3 mn = glm::min(a, b);
//     glm::vec3 mx = glm::max(a, b);
//     if (std::abs(mx.x - mn.x) > std::abs(mx.z - mn.z)) {
//         // along X
//         glm::vec3 center = {(mn.x + mx.x) * 0.5f, height * 0.5f, mn.z};
//         glm::vec3 half   = { (mx.x - mn.x) * 0.5f, height * 0.5f, thickness * 0.5f };
//         addBox(L, center, half);
//     } else {
//         // along Z
//         glm::vec3 center = {mn.x, height * 0.5f, (mn.z + mx.z) * 0.5f};
//         glm::vec3 half   = {thickness * 0.5f, height * 0.5f, (mx.z - mn.z) * 0.5f };
//         addBox(L, center, half);
//     }
// }

// static void addStaircase(std::vector<AABB>& L, glm::vec3 start, int steps,
//                          float tread, float rise, float width, float depth)
// {
//     // Axis-aligned forward (positive X) staircase
//     // start = center of first step top surface
//     float y = start.y;
//     float x = start.x;
//     for(int i=0;i<steps;i++){
//         glm::vec3 center = { x + i*tread, y + i*rise, start.z };
//         glm::vec3 half   = { tread*0.5f, rise*0.5f, depth*0.5f };
//         // Solid rectangular prisms approximating proper steps (tread+riser)
//         // Width: 'width' spans along Z (we're centered at start.z)
//         addBox(L, center, { half.x, half.y, width*0.5f });
//         // Make the step surface thicker for nice contact
//         addBox(L, { center.x, center.y + half.y + 0.05f, center.z }, { half.x, 0.05f, width*0.5f });
//     }
// }

// static void addPlatform(std::vector<AABB>& L, glm::vec3 center, glm::vec2 sizeXZ, float thickness)
// {
//     addBox(L, {center.x, center.y - thickness*0.5f, center.z}, {sizeXZ.x*0.5f, thickness*0.5f, sizeXZ.y*0.5f});
// }

// static void addDoorway(std::vector<AABB>& L, glm::vec3 baseCenter, float openingW, float openingH, float wallT, float lintelT)
// {
//     // Two vertical posts
//     float postW = (openingW*0.5f - 0.05f);
//     addBox(L, { baseCenter.x - openingW*0.5f - wallT*0.5f, baseCenter.y + openingH*0.5f, baseCenter.z }, { wallT*0.5f, openingH*0.5f, wallT*0.5f });
//     addBox(L, { baseCenter.x + openingW*0.5f + wallT*0.5f, baseCenter.y + openingH*0.5f, baseCenter.z }, { wallT*0.5f, openingH*0.5f, wallT*0.5f });
//     // Lintel/top beam
//     addBox(L, { baseCenter.x, baseCenter.y + openingH + lintelT*0.5f, baseCenter.z }, { openingW*0.5f + wallT, lintelT*0.5f, wallT*0.5f });
// }


// // std::vector<AABB> buildTestLevel() {
// //     std::vector<AABB> L; L.reserve(64);
// //     // Ground: large flat
// //     addBox(L, {0,-1,0}, {30,1,30});

// //     // A few platforms at different heights
// //     addBox(L, { 3, 0.1f, -4}, {1.5f, 0.1f, 1.5f});
// //     addBox(L, { 7, 0.6f, -4}, {1.5f, 0.1f, 1.5f});
// //     addBox(L, {11, 1.3f, -4}, {1.5f, 0.1f, 1.5f});

// //     // A wide platform to jump down from
// //     addBox(L, {-6, 2.0f, 6}, {3.0f, 0.15f, 3.0f});

// //     // Step-ramp (approximated by shallow stairs)
// //     float x0 = -4.0f, z0 = -6.0f;
// //     for(int i=0;i<14;i++){
// //         float y = 0.1f + 0.12f * i;
// //         addBox(L, {x0 + i*1.0f, y, z0}, {0.5f, 0.08f, 2.0f});
// //     }

// //     // A narrow pillar
// //     addBox(L, {2.0f, 1.0f, 5.0f}, {0.4f, 1.0f, 0.4f});

// //     // A wall to slide along
// //     addBox(L, { -10.0f, 0.5f, 0.0f }, {0.2f, 2.0f, 4.0f});

// //     return L;
// // }



// std::vector<AABB> buildTestLevel() {
//     std::vector<AABB> L; L.reserve(128);

//     // --- Ground plane (big + bright green via render heuristic) --------------
//     // (Center y at -1 with thickness 2 → top at y=0)
//     addBox(L, {0,-1,0}, {30,1,30});

//     // --- Perimeter walls (real walls) ----------------------------------------
//     // Arena bounds ~ [-28..+28] in X and Z; wall height 4, thickness 0.5
//     float Wmin=-28.f, Wmax=28.f, Zmin=-28.f, Zmax=28.f;
//     float wallH=4.0f, wallT=0.6f;

//     // North/South walls (along X)
//     addWall(L, {Wmin, 0, Zmin}, {Wmax, 0, Zmin}, wallT, wallH);
//     addWall(L, {Wmin, 0, Zmax}, {Wmax, 0, Zmax}, wallT, wallH);

//     // East/West walls (along Z), leave a gate on the West side
//     addWall(L, {Wmin, 0, Zmin}, {Wmin, 0, -6.0f}, wallT, wallH);
//     addWall(L, {Wmin, 0, +6.0f}, {Wmin, 0, Zmax}, wallT, wallH);
//     // Full East wall
//     addWall(L, {Wmax, 0, Zmin}, {Wmax, 0, Zmax}, wallT, wallH);

//     // Gate detail (a doorway + lintel just inside the West gap)
//     addDoorway(L, {Wmin + 2.5f, 0.0f, 0.0f}, /*openingW*/4.0f, /*openingH*/3.0f, wallT, /*lintelT*/0.4f);

//     // --- Proper staircase up to a high platform ------------------------------
//     // Staircase runs along +X, 10 steps with nice treads/risers
//     glm::vec3 stairStart = {-20.0f, 0.10f, -8.0f};
//     addStaircase(L, stairStart, /*steps*/10, /*tread*/0.9f, /*rise*/0.25f, /*width*/3.5f, /*depth*/0.9f);

//     // Target platform at top of stairs
//     glm::vec3 platAcenter = { stairStart.x + 9*0.9f + 1.2f, stairStart.y + 9*0.25f + 0.4f, -8.0f };
//     addPlatform(L, platAcenter, /*sizeXZ*/{6.0f, 4.0f}, /*thickness*/0.3f);

//     // --- Narrow bridge to another platform -----------------------------------
//     // Short bridge made of a few thin slabs
//     for (int i=0;i<6;i++){
//         glm::vec3 c = { platAcenter.x + 1.6f + i*1.0f, platAcenter.y + 0.0f, -8.0f };
//         addBox(L, c, {0.45f, 0.06f, 0.7f});
//     }
//     glm::vec3 platBcenter = { platAcenter.x + 1.6f + 6.0f, platAcenter.y + 0.0f, -8.0f };
//     addPlatform(L, platBcenter, {4.5f, 4.0f}, 0.3f);

//     // --- Jump gap challenge ---------------------------------------------------
//     // Two facing platforms with a clean air gap between them
//     addPlatform(L, { 10.0f, 1.2f, 6.0f }, {3.5f, 3.5f}, 0.25f);
//     addPlatform(L, { 14.6f, 1.2f, 6.0f }, {3.5f, 3.5f}, 0.25f);

//     // --- Pillar slalom --------------------------------------------------------
//     for(int i=0;i<6;i++){
//         float x = -6.0f + i*2.2f;
//         float z = 6.0f + (i%2==0 ? 0.0f : 1.2f);
//         addBox(L, {x, 1.0f, z}, {0.4f, 1.0f, 0.4f});
//     }

//     // --- Low ceiling tunnel (forces controlled movement) ---------------------
//     // Floor pads
//     addPlatform(L, {-10.0f, 0.25f, 12.0f}, {6.0f, 2.0f}, 0.2f);
//     addPlatform(L, { -3.0f, 0.25f, 12.0f}, {6.0f, 2.0f}, 0.2f);
//     // Overhead beams at ~1.4m to "duck under" (you'll collide if you jump)
//     for(int i=0;i<4;i++){
//         float x = -11.5f + i*2.5f;
//         addBox(L, {x, 1.4f, 12.0f}, {1.1f, 0.1f, 1.0f});
//     }

//     // --- A stepped ramp (cleaner treads than before) -------------------------
//     float rx0 = -6.0f, rz0 = -14.0f;
//     for(int i=0;i<12;i++){
//         float y  = 0.1f + 0.10f * i;
//         float tx = rx0 + i*0.9f;
//         addBox(L, {tx, y, rz0}, {0.45f, 0.08f, 2.0f});
//     }

//     // --- A few single blocks to hop onto/off of ------------------------------
//     addBox(L, { 4.0f, 0.5f, -2.0f }, {0.6f, 0.1f, 0.6f});
//     addBox(L, { 6.2f, 0.9f, -2.0f }, {0.6f, 0.1f, 0.6f});
//     addBox(L, { 8.4f, 1.4f, -2.0f }, {0.6f, 0.1f, 0.6f});

//     return L;
// }

// // -------------------------------------------------------------
// // Input helpers
// // -------------------------------------------------------------
// struct Input {
//     float mx=0,my=0;  // delta mouse
//     int   lockToggle=0;
//     int   jumpPressed=0;
//     bool  jumpHeld=false;
//     glm::vec2 move{0,0};
//     bool sprint=false;
// };

// static double g_prevMouseX=0, g_prevMouseY=0;
// static bool   g_firstMouse=true;

// void pollInput(GLFWwindow* win, Input& in, CameraState& cam){
//     in = Input{};
//     if(glfwGetKey(win, GLFW_KEY_TAB)==GLFW_PRESS) in.lockToggle=1;

//     double x,y; glfwGetCursorPos(win,&x,&y);
//     if(cam.cursorLocked){
//         if(g_firstMouse){ g_prevMouseX=x; g_prevMouseY=y; g_firstMouse=false; }
//         in.mx = float(x - g_prevMouseX);
//         in.my = float(y - g_prevMouseY);
//         g_prevMouseX = x; g_prevMouseY = y;
//     } else {
//         g_firstMouse=true;
//     }

//     auto key = [&](int k){ return glfwGetKey(win,k)==GLFW_PRESS; };
//     glm::vec2 m(0);
//     if(key(GLFW_KEY_W)) m.y += 1;
//     if(key(GLFW_KEY_S)) m.y -= 1;
//     if(key(GLFW_KEY_D)) m.x += 1;
//     if(key(GLFW_KEY_A)) m.x -= 1;
//     if(glm::length(m)>0) m = glm::normalize(m);
//     in.move = m;
//     in.sprint = key(GLFW_KEY_LEFT_SHIFT) || key(GLFW_KEY_RIGHT_SHIFT);

//     // jump
//     bool jp = key(GLFW_KEY_SPACE);
//     in.jumpHeld = jp;
//     static bool prev=false;
//     in.jumpPressed = (jp && !prev) ? 1 : 0;
//     prev = jp;

//     // Toggle cursor lock on TAB press edge
//     static bool prevTab=false;
//     bool tabNow = key(GLFW_KEY_TAB);
//     if(tabNow && !prevTab){ cam.cursorLocked = !cam.cursorLocked; glfwSetInputMode(win, GLFW_CURSOR, cam.cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL); }
//     prevTab = tabNow;
// }

// // -------------------------------------------------------------
// // Collision response helpers (rigid-body style impulses)
// // -------------------------------------------------------------
// static void applyCollisionResponse(ControllerConfig const& C, ControllerState& S, glm::vec3 n)
// {
//     // Normal impulse with restitution
//     float vn = glm::dot(S.vel, n);
//     if(vn < 0.0f){
//         float vOut = -(1.0f + C.restitution) * vn; // amount to add along n
//         S.vel += n * vOut;
//     }
//     // Simple Coulomb-ish friction: dampen tangential component a bit
//     glm::vec3 vN = glm::dot(S.vel, n) * n;
//     glm::vec3 vT = S.vel - vN;
//     S.vel = vN + vT * std::max(0.0f, 1.0f - C.friction);
// }

// // -------------------------------------------------------------
// // One substep of physics (allows us to sub-divide dt to prevent tunneling)
// // -------------------------------------------------------------
// static void physicsSubstep(const ControllerConfig& C, ControllerState& S, const std::vector<AABB>& level, float dt, const CameraState& cam, const Input& in)
// {
//     // Track timers
//     S.timeSinceGrounded += dt;
//     S.timeSinceJumpPressed += dt;
//     S.jumpHeld = in.jumpHeld;

//     // Movement intent (relative to camera yaw)
//     float cy = std::cos(glm::radians(cam.yaw));
//     float sy = std::sin(glm::radians(cam.yaw));
//     glm::vec3 fwd = glm::normalize(glm::vec3(sy,0,cy));   // camera forward projected to XZ
//     glm::vec3 right= glm::normalize(glm::cross(fwd, glm::vec3(0,1,0)));
//     glm::vec3 wish = fwd * in.move.y + right * in.move.x;
//     if(glm::length(wish)>0) wish = glm::normalize(wish);
//     float targetSpeed = in.sprint ? C.sprintSpeed : C.walkSpeed;

//     // Horizontal acceleration
//     glm::vec3 velXZ = glm::vec3(S.vel.x,0,S.vel.z);
//     if(glm::length(wish) > 0){
//         glm::vec3 target = wish * targetSpeed;
//         glm::vec3 dv = target - velXZ;
//         float accel = S.grounded ? C.groundAccel : C.airAccel;
//         glm::vec3 dir = (glm::length(dv) > 1e-6f) ? glm::normalize(dv) : glm::vec3(0);
//         glm::vec3 add = dir * accel * dt;
//         if(glm::length(dv) <= glm::length(add)) add = dv;
//         velXZ += add;
//     }

//     // Air speed clamp
//     if(!S.grounded){
//         float sp = glm::length(velXZ);
//         if(sp > C.maxAirSpeed) velXZ *= (C.maxAirSpeed / sp);
//     }

//     // Drag
//     float drag = S.grounded ? C.groundDrag : C.airDrag;
//     velXZ *= (1.0f / (1.0f + drag*dt));

//     // Recombine
//     S.vel.x = velXZ.x;
//     S.vel.z = velXZ.z;

//     // Gravity with multipliers
//     float g = -C.gravity;
//     if(S.vel.y < 0){ // falling
//         S.vel.y += g * C.fallGravityMult * dt;
//     } else {
//         float mult = (S.jumpHeld ? 1.0f : C.lowJumpGravityMult);
//         S.vel.y += g * mult * dt;
//     }

//     // Jumping (coyote + buffer)
//     if(in.jumpPressed) S.timeSinceJumpPressed = 0.f;
//     if(S.timeSinceJumpPressed <= C.jumpBufferTime && (S.grounded || S.timeSinceGrounded <= C.coyoteTime)) {
//         S.timeSinceJumpPressed = 1e3f;
//         S.timeSinceGrounded = 1e3f;
//         S.vel.y = std::sqrt(2.0f * C.gravity * C.jumpHeight);
//         S.grounded=false;
//     }

//     // Integrate
//     glm::vec3 newPos = S.pos + S.vel * dt;

//     // Resolve collisions iteratively
//     glm::vec3 groundNormalAccum(0);
//     bool onGround = false;
//     for(int it=0; it<4; ++it){
//         bool any=false;
//         for(const auto& b : level){
//             glm::vec3 n; float depth;
//             glm::vec3 center = newPos;
//             if(resolveSphereAABB(center, C.radius, b, n, depth)){
//                 any=true;
//                 newPos = center;
//                 // Apply rigid-body like response
//                 applyCollisionResponse(C, S, n);

//                 // Collect ground normals
//                 if(n.y > 0.5f){
//                     onGround = true;
//                     groundNormalAccum += n;
//                 }
//             }
//         }
//         if(!any) break;
//     }

//     // Ground state
//     if(onGround){
//         S.grounded = true;
//         S.timeSinceGrounded = 0.f;
//         if(glm::length(groundNormalAccum) > 1e-4f)
//             S.groundNormal = glm::normalize(groundNormalAccum);
//         // Gentle stick-to-ground near contact
//         if(S.vel.y < 0 && S.vel.y > -3.0f) S.vel.y = 0.0f;
//     } else {
//         S.grounded = false;
//     }

//     S.pos = newPos;
// }

// // -------------------------------------------------------------
// // Multi-substep wrapper
// // -------------------------------------------------------------
// void stepController(const ControllerConfig& C, ControllerState& S, const std::vector<AABB>& level, float dt, const CameraState& cam, const Input& in)
// {
//     // Choose substeps based on speed to reduce tunneling (displacement vs radius)
//     float speed = glm::length(S.vel);
//     float maxDisp = std::max(0.001f, C.radius * 0.5f);
//     int subSteps = std::clamp( (int)std::ceil((speed * dt) / maxDisp), 1, 8 );
//     float h = dt / float(subSteps);

//     for(int i=0;i<subSteps;i++){
//         physicsSubstep(C, S, level, h, cam, in);
//     }
// }

// // -------------------------------------------------------------
// // Camera
// // -------------------------------------------------------------
// glm::mat4 computeCamera(const ControllerConfig& C, const ControllerState& S, CameraState& cam, const Input& in, float dt, int width, int height)
// {
//     // mouse look (reduced sensitivity per your feedback)
//     const float sensitivityX = 0.045f, sensitivityY = 0.035f;
//     cam.yaw   += in.mx * sensitivityX;
//     cam.pitch += -in.my * sensitivityY;
//     cam.pitch = clampf(cam.pitch, -40.0f, 75.0f);

//     // chase camera target
//     glm::vec3 target = S.pos + glm::vec3(0, C.eyeHeight, 0);
//     float cy = std::cos(glm::radians(cam.yaw));
//     float sy = std::sin(glm::radians(cam.yaw));
//     float cp = std::cos(glm::radians(cam.pitch));
//     float sp = std::sin(glm::radians(cam.pitch));
//     glm::vec3 dir = glm::normalize(glm::vec3(sy*cp, sp, cy*cp));
//     glm::vec3 camPos = target - dir * cam.dist + glm::vec3(0, cam.height, 0);

//     // smooth follow
//     static glm::vec3 smoothPos = camPos;
//     smoothPos = glm::mix(smoothPos, camPos, 1.0f - std::exp(-12.0f*dt));

//     glm::mat4 view = glm::lookAt(smoothPos, target, glm::vec3(0,1,0));
//     float aspect = float(width)/float(height);
//     glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.05f, 200.0f);
//     return proj * view;
// }

// // -------------------------------------------------------------
// // Rendering helpers
// // -------------------------------------------------------------
// void drawBox(const Mesh& cube, GLuint prog, const glm::mat4& VP, glm::vec3 center, glm::vec3 halfExtents, glm::vec3 color){
//     glm::mat4 M = glm::translate(glm::mat4(1), center) * glm::scale(glm::mat4(1), halfExtents*2.0f);
//     glm::mat4 MVP = VP * M;
//     glUseProgram(prog);
//     glUniformMatrix4fv(glGetUniformLocation(prog,"uMVP"),1,GL_FALSE, glm::value_ptr(MVP));
//     glUniform3fv(glGetUniformLocation(prog,"uColor"),1, glm::value_ptr(color));
//     glBindVertexArray(cube.vao);
//     glDrawElements(GL_TRIANGLES, cube.indexCount, GL_UNSIGNED_INT, 0);
// }

// void drawSphereProxy(const Mesh& cube, GLuint prog, const glm::mat4& VP, glm::vec3 center, float radius, glm::vec3 color){
//     // Visualize the sphere as a cube scaled to radius*2 (simple proxy)
//     drawBox(cube, prog, VP, center, glm::vec3(radius), color);
// }

// // -------------------------------------------------------------
// // Main
// // -------------------------------------------------------------
// int main(){
//     if(!glfwInit()) die("Failed to init GLFW");
//     // Core profile preferred
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
//     glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
// #if __APPLE__
//     glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
// #endif

//     int W=1280, H=800;
//     GLFWwindow* win = glfwCreateWindow(W,H," OpenGL Character Controller ",nullptr,nullptr);
//     if(!win){ glfwTerminate(); die("Failed to create window"); }
//     glfwMakeContextCurrent(win);
//     glfwSwapInterval(1);

//     GLenum err = glewInit();
//     if(err != GLEW_OK) die("Failed to init GLEW");

//     // Set mouse locked by default
//     glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

//     GLuint prog = makeProgram(kVert, kFrag);
//     Mesh cube = makeUnitCube();

//     // Level
//     auto level = buildTestLevel();

//     // Controller & Camera
//     ControllerConfig C;
//     ControllerState  S;
//     CameraState      cam;
//     cam.cursorLocked = true;
//     cam.dist = 6.0f; cam.height=0.8f;

//     // Timing
//     using clock = std::chrono::high_resolution_clock;
//     auto t0 = clock::now();
//     float accumulator=0.f;
//     const float dt = 1.0f/120.0f; // fixed physics step

//     glEnable(GL_DEPTH_TEST);

//     while(!glfwWindowShouldClose(win)){
//         glfwPollEvents();
//         if(glfwGetKey(win, GLFW_KEY_ESCAPE)==GLFW_PRESS) break;

//         // Resize query
//         glfwGetFramebufferSize(win, &W, &H);
//         glViewport(0,0,W,H);

//         // Input
//         Input in; pollInput(win, in, cam);

//         // Timing
//         auto t1 = clock::now();
//         float frame = std::chrono::duration<float>(t1 - t0).count();
//         t0 = t1;
//         frame = std::min(frame, 0.1f); // clamp delta to avoid spiral on stall
//         accumulator += frame;

//         // Physics: fixed timestep with up to N steps per frame
//         int steps=0;
//         while(accumulator >= dt && steps<6){
//             stepController(C, S, level, dt, cam, in);
//             accumulator -= dt;
//             steps++;
//         }

//         // Camera
//         glm::mat4 VP = computeCamera(C, S, cam, in, std::max(dt,frame), W, H);

//         // Render
//         // Sky blue
//         glClearColor(0.52f, 0.80f, 0.98f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

//         // Draw level
//         for(const auto& b : level){
//             glm::vec3 center = 0.5f*(b.mn + b.mx);
//             glm::vec3 he     = 0.5f*(b.mx - b.mn);
//             // Bright green for big ground, soft gray for others
//             glm::vec3 color = (he.x>20.0f && he.z>20.0f) ? glm::vec3(0.30f, 0.90f, 0.30f)
//                                                          : glm::vec3(0.60f, 0.60f, 0.65f);
//             drawBox(cube, prog, VP, center, he, color);
//         }

//         // Draw player (cube proxy for sphere): cyan/teal
//         glm::vec3 playerColor = S.grounded ? glm::vec3(0.00f, 0.90f, 0.90f)
//                                            : glm::vec3(0.00f, 0.70f, 1.00f);
//         // drawSphereProxy(cube, prog, VP, S.pos + glm::vec3(0,C.eyeHeight,0), C.radius, playerColor);
//         drawSphereProxy(cube, prog, VP, S.pos, C.radius, playerColor);

//         glfwSwapBuffers(win);
//     }

//     glDeleteProgram(prog);
//     glDeleteVertexArrays(1,&cube.vao);
//     glDeleteBuffers(1,&cube.vbo);
//     glDeleteBuffers(1,&cube.ebo);

//     glfwTerminate();
//     return 0;
// }


// Single-file 3D Character Controller (C++17 + OpenGL + GLFW + GLEW + GLM)
//
// CONTROLS
//   Move ............. WASD
//   Jump ............. Space (hold = higher)
//   Sprint ........... Shift (faster + higher jump + better air control)
//   Reset Position ... R
//   Slow Motion ...... P (toggle 0.35x)
//   Wireframe ........ T (toggle)
//   Toggle Cursor .... Tab
//   Quit ............. Esc
//
// WHAT'S INCLUDED
// - One-file, self-contained shaders/meshes
// - Sphere character vs static & kinematic AABBs (boxes) using rigid-body-like response
// - Sub-stepped physics (reduces tunneling), coyote time, jump buffering, variable jump height
// - Sprint buffs: faster ground speed, higher jump, better air control + higher air top speed
// - Third-person smooth chase camera with reduced mouse sensitivity
// - 4× MSAA, bright colors (sky/floor/steps/walls), polished “training yard” course
// - Moving platforms (sine motion) + sliding obstacle
//
// BUILD (Linux):
//   g++ -std=c++17 main.cpp -O2 -lglfw -lGLEW -lGL -ldl -lpthread -lX11 -lXrandr -lXi -o character
//
// BUILD (Windows, MinGW):
//   g++ -std=gnu++17 main.cpp -O2 -lglfw3 -lglew32 -lopengl32 -lgdi32 -lwinmm -o character.exe
//
// BUILD (Windows, MSVC Dev Prompt):
//   cl /std:c++17 /O2 main.cpp /I"path\\to\\glm" /I"path\\to\\glew\\include" /I"path\\to\\glfw\\include" ^
//      /link /LIBPATH:"path\\to\\glew\\lib" /LIBPATH:"path\\to\\glfw\\lib" opengl32.lib glew32.lib glfw3.lib gdi32.lib user32.lib shell32.lib
//
// BUILD (macOS, Homebrew):
//   clang++ -std=c++17 main.cpp -O2 -I/opt/homebrew/include -L/opt/homebrew/lib \
//           -lglfw -lglew -framework OpenGL -o character
//
// Notes: Requires GLFW, GLEW, GLM. (Ubuntu: sudo apt install libglfw3-dev libglew-dev libglm-dev)
//
// ---------------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 0
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// -------------------------------------------------------------
// Small utilities
// -------------------------------------------------------------
static void die(const char* msg) { std::fprintf(stderr, "%s\n", msg); std::exit(EXIT_FAILURE); }

static const char* kVert = R"GLSL(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos,1.0); }
)GLSL";

static const char* kFrag = R"GLSL(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main(){ FragColor = vec4(uColor,1.0); }
)GLSL";

GLuint makeProgram(const char* vs, const char* fs) {
    auto compile = [](GLenum type, const char* src)->GLuint{
        GLuint id = glCreateShader(type);
        glShaderSource(id,1,&src,nullptr);
        glCompileShader(id);
        GLint ok; glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
        if(!ok){ char log[2048]; glGetShaderInfoLog(id,2048,nullptr,log); std::fprintf(stderr,"Shader error:\n%s\n",log); std::exit(1); }
        return id;
    };
    GLuint v=compile(GL_VERTEX_SHADER,vs), f=compile(GL_FRAGMENT_SHADER,fs);
    GLuint p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f);
    glLinkProgram(p);
    GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ char log[2048]; glGetProgramInfoLog(p,2048,nullptr,log); std::fprintf(stderr,"Link error:\n%s\n",log); std::exit(1); }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

// -------------------------------------------------------------
// Simple meshes (unit cube)
// -------------------------------------------------------------
struct Mesh {
    GLuint vao=0, vbo=0, ebo=0;
    GLsizei indexCount=0;
};

Mesh makeUnitCube() {
    Mesh m;
    float v[] = {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
    };
    unsigned int idx[] = {
        0,1,2, 2,3,0,  4,5,6, 6,7,4,
        0,4,7, 7,3,0,  1,5,6, 6,2,1,
        3,2,6, 6,7,3,  0,1,5, 5,4,0
    };
    glGenVertexArrays(1,&m.vao);
    glGenBuffers(1,&m.vbo);
    glGenBuffers(1,&m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glBindVertexArray(0);
    m.indexCount = (GLsizei)(sizeof(idx)/sizeof(idx[0]));
    return m;
}

// -------------------------------------------------------------
// Math & physics helpers
// -------------------------------------------------------------
struct AABB { glm::vec3 mn, mx; }; // axis-aligned box

static inline float clampf(float x, float a, float b){ return std::max(a, std::min(b, x)); }
static inline glm::vec3 clampv3(glm::vec3 v, glm::vec3 a, glm::vec3 b){
    return glm::vec3(clampf(v.x,a.x,b.x), clampf(v.y,a.y,b.y), clampf(v.z,a.z,b.z));
}
static inline glm::vec3 closestPointOnAABB(const glm::vec3& p, const AABB& b){
    return clampv3(p, b.mn, b.mx);
}

// Sphere vs AABB penetration resolution: output push normal/depth, push center out.
static bool resolveSphereAABB(glm::vec3& sphereCenter, float radius, const AABB& box, glm::vec3& outNormal, float& outDepth) {
    glm::vec3 q = closestPointOnAABB(sphereCenter, box);
    glm::vec3 d = sphereCenter - q;
    float dist2 = glm::dot(d,d);
    if(dist2 > radius*radius) return false;
    float dist = std::sqrt(std::max(1e-12f, dist2));
    if(dist > 1e-6f){
        outNormal = d / dist;
        outDepth = radius - dist;
    }else{
        glm::vec3 centerToMin = sphereCenter - box.mn;
        glm::vec3 maxToCenter = box.mx - sphereCenter;
        float minPen = centerToMin.x; int axis=0; float sign=-1.f;
        if(centerToMin.y < minPen){ minPen=centerToMin.y; axis=1; sign=-1.f; }
        if(centerToMin.z < minPen){ minPen=centerToMin.z; axis=2; sign=-1.f; }
        if(maxToCenter.x < minPen){ minPen=maxToCenter.x; axis=0; sign=+1.f; }
        if(maxToCenter.y < minPen){ minPen=maxToCenter.y; axis=1; sign=+1.f; }
        if(maxToCenter.z < minPen){ minPen=maxToCenter.z; axis=2; sign=+1.f; }
        outNormal = glm::vec3(0); outNormal[axis] = sign; outDepth = radius;
    }
    sphereCenter += outNormal * outDepth;
    return true;
}

// -------------------------------------------------------------
// Controller config/state
// -------------------------------------------------------------
struct ControllerConfig {
    float walkSpeed = 6.0f;
    float sprintSpeed = 9.0f;
      float groundAccel = 120.0f;   // was 45
    float airAccel    = 20.0f;    // was 12
    float groundDrag  = 5.0f;     // was 8  (less braking while on ground)

    float maxAirSpeed = 8.0f;

    float gravity = 9.81f;
    float jumpHeight = 1.25f;
    float coyoteTime = 0.12f;
    float jumpBufferTime = 0.16f;
    float fallGravityMult = 2.5f;
    float lowJumpGravityMult = 2.0f;

    float airDrag = 0.02f;

    float restitution = 0.05f;   // bounciness
    float friction   = 0.20f;    // tangential damping

    float radius = 0.45f;
    float eyeHeight = 0.9f;

    // Sprint perks
    float sprintAirAccelMult    = 1.25f;
    float sprintMaxAirSpeedMult = 1.25f;
    float sprintJumpBoost       = 1.25f;
};

struct ControllerState {
    glm::vec3 pos{0,2.0f,0};
    glm::vec3 vel{0,0,0};
    bool grounded=false;
    glm::vec3 groundNormal{0,1,0};
    float timeSinceGrounded=1e3f;
    float timeSinceJumpPressed=1e3f;
    bool jumpHeld=false;
};

struct CameraState {
    float yaw=0.0f, pitch=10.0f;
    float dist=6.0f, height=2.0f;
    bool cursorLocked=true;
};

// -------------------------------------------------------------
// Kinematic (moving) AABBs
// -------------------------------------------------------------
struct KinBox {
    AABB base;
    glm::vec3 dir;     // normalized movement direction
    float amplitude;   // displacement amplitude
    float speed;       // angular speed (rad/s)
    float phase;       // phase offset
    glm::vec3 color;   // render color
};
static inline AABB movedAABB(const KinBox& k, float t){
    float s = std::sin(k.speed*t + k.phase) * k.amplitude;
    glm::vec3 off = k.dir * s;
    return { k.base.mn + off, k.base.mx + off };
}

// -------------------------------------------------------------
// Level generation
// -------------------------------------------------------------
static void addBox(std::vector<AABB>& level, glm::vec3 center, glm::vec3 halfSize){
    level.push_back({center-halfSize, center+halfSize});
}
static void addWall(std::vector<AABB>& L, glm::vec3 a, glm::vec3 b, float thickness, float height){
    glm::vec3 mn = glm::min(a, b), mx = glm::max(a, b);
    if (std::abs(mx.x - mn.x) > std::abs(mx.z - mn.z)) {
        glm::vec3 center = {(mn.x + mx.x) * 0.5f, height * 0.5f, mn.z};
        glm::vec3 half   = { (mx.x - mn.x) * 0.5f, height * 0.5f, thickness * 0.5f };
        addBox(L, center, half);
    } else {
        glm::vec3 center = {mn.x, height * 0.5f, (mn.z + mx.z) * 0.5f};
        glm::vec3 half   = {thickness * 0.5f, height * 0.5f, (mx.z - mn.z) * 0.5f };
        addBox(L, center, half);
    }
}
static void addStaircase(std::vector<AABB>& L, glm::vec3 start, int steps, float tread, float rise, float width, float depth){
    float y = start.y, x = start.x;
    for(int i=0;i<steps;i++){
        glm::vec3 c = { x + i*tread, y + i*rise, start.z };
        glm::vec3 half = { tread*0.5f, rise*0.5f, depth*0.5f };
        addBox(L, c, { half.x, half.y, width*0.5f });
        addBox(L, { c.x, c.y + half.y + 0.05f, c.z }, { half.x, 0.05f, width*0.5f });
    }
}
static void addPlatform(std::vector<AABB>& L, glm::vec3 center, glm::vec2 sizeXZ, float thickness){
    addBox(L, {center.x, center.y - thickness*0.5f, center.z}, {sizeXZ.x*0.5f, thickness*0.5f, sizeXZ.y*0.5f});
}
static void addDoorway(std::vector<AABB>& L, glm::vec3 baseCenter, float openingW, float openingH, float wallT, float lintelT){
    addBox(L, { baseCenter.x - openingW*0.5f - wallT*0.5f, baseCenter.y + openingH*0.5f, baseCenter.z }, { wallT*0.5f, openingH*0.5f, wallT*0.5f });
    addBox(L, { baseCenter.x + openingW*0.5f + wallT*0.5f, baseCenter.y + openingH*0.5f, baseCenter.z }, { wallT*0.5f, openingH*0.5f, wallT*0.5f });
    addBox(L, { baseCenter.x, baseCenter.y + openingH + lintelT*0.5f, baseCenter.z }, { openingW*0.5f + wallT, lintelT*0.5f, wallT*0.5f });
}

std::vector<AABB> buildStaticLevel() {
    std::vector<AABB> L; L.reserve(160);
    // Ground (top at y=0)
    addBox(L, {0,-1,0}, {30,1,30});

    // Perimeter walls
    float Wmin=-28.f, Wmax=28.f, Zmin=-28.f, Zmax=28.f;
    float wallH=4.0f, wallT=0.6f;
    addWall(L, {Wmin,0,Zmin},{Wmax,0,Zmin},wallT,wallH);
    addWall(L, {Wmin,0,Zmax},{Wmax,0,Zmax},wallT,wallH);
    addWall(L, {Wmin,0,Zmin},{Wmin,0,-6.0f},wallT,wallH);
    addWall(L, {Wmin,0,+6.0f},{Wmin,0,Zmax},wallT,wallH);
    addWall(L, {Wmax,0,Zmin},{Wmax,0,Zmax},wallT,wallH);
    addDoorway(L, {Wmin + 2.5f, 0.0f, 0.0f}, 4.0f, 3.0f, wallT, 0.4f);

    // Staircase + platforms
    glm::vec3 stairStart = {-20.0f, 0.10f, -8.0f};
    addStaircase(L, stairStart, 10, 0.9f, 0.25f, 3.5f, 0.9f);
    glm::vec3 platA = { stairStart.x + 9*0.9f + 1.2f, stairStart.y + 9*0.25f + 0.4f, -8.0f };
    addPlatform(L, platA, {6.0f, 4.0f}, 0.3f);

    // Narrow bridge slabs + platform
    for (int i=0;i<6;i++) addBox(L, { platA.x + 1.6f + i*1.0f, platA.y, -8.0f }, {0.45f,0.06f,0.7f});
    addPlatform(L, { platA.x + 1.6f + 6.0f, platA.y, -8.0f }, {4.5f,4.0f}, 0.3f);

    // Jump gap pair
    addPlatform(L, { 10.0f, 1.2f, 6.0f }, {3.5f,3.5f}, 0.25f);
    addPlatform(L, { 14.6f, 1.2f, 6.0f }, {3.5f,3.5f}, 0.25f);

    // Pillar slalom
    for(int i=0;i<6;i++){
        float x = -6.0f + i*2.2f;
        float z = 6.0f + (i%2==0 ? 0.0f : 1.2f);
        addBox(L, {x, 1.0f, z}, {0.4f, 1.0f, 0.4f});
    }

    // Low tunnel with beams
    addPlatform(L, {-10.0f, 0.25f, 12.0f}, {6.0f, 2.0f}, 0.2f);
    addPlatform(L, { -3.0f, 0.25f, 12.0f}, {6.0f, 2.0f}, 0.2f);
    for(int i=0;i<4;i++){
        float x = -11.5f + i*2.5f;
        addBox(L, {x, 1.4f, 12.0f}, {1.1f, 0.1f, 1.0f});
    }

    // Stepped ramp
    float rx0 = -6.0f, rz0 = -14.0f;
    for(int i=0;i<12;i++){
        float y  = 0.1f + 0.10f * i;
        float tx = rx0 + i*0.9f;
        addBox(L, {tx, y, rz0}, {0.45f, 0.08f, 2.0f});
    }

    // Singles
    addBox(L, { 4.0f, 0.5f, -2.0f }, {0.6f, 0.1f, 0.6f});
    addBox(L, { 6.2f, 0.9f, -2.0f }, {0.6f, 0.1f, 0.6f});
    addBox(L, { 8.4f, 1.4f, -2.0f }, {0.6f, 0.1f, 0.6f});

    return L;
}
std::vector<KinBox> buildKinematics(){
    std::vector<KinBox> K; K.reserve(16);

    // Moving elevator platform (vertical bob)
    K.push_back(KinBox{
        AABB{ glm::vec3(-2.0f, 0.4f-0.15f, -6.0f), glm::vec3(+2.0f, 0.4f+0.15f, -4.0f) },
        glm::vec3(0,1,0),
        0.6f, 2.4f, 0.0f,
        glm::vec3(0.25f, 0.85f, 0.95f)
    });

    // Horizontal shuttling platform (X)
    K.push_back(KinBox{
        AABB{ glm::vec3(2.5f, 1.0f-0.12f, 6.0f), glm::vec3(4.5f, 1.0f+0.12f, 7.5f) },
        glm::vec3(1,0,0),
        2.0f, 1.6f, 1.2f,
        glm::vec3(0.85f, 0.45f, 0.95f)
    });

    // Sliding obstacle (a “door” sweeping Z)
    K.push_back(KinBox{
        AABB{ glm::vec3(-12.0f, 0.9f-0.9f, -3.0f), glm::vec3(-11.4f, 0.9f+0.9f, +3.0f) },
        glm::vec3(0,0,1),
        3.0f, 1.2f, 0.5f,
        glm::vec3(0.95f, 0.55f, 0.25f)
    });

    return K;
}

// -------------------------------------------------------------
// Input helpers
// -------------------------------------------------------------
struct Input {
    float mx=0,my=0;  // delta mouse
    int   jumpPressed=0;
    bool  jumpHeld=false;
    glm::vec2 move{0,0};
    bool sprint=false;
    bool reset=false, slowmo=false, wire=false, toggleCursor=false;
};

static double g_prevMouseX=0, g_prevMouseY=0;
static bool   g_firstMouse=true;

void pollInput(GLFWwindow* win, Input& in, CameraState& cam){
    in = Input{};
    double x,y; glfwGetCursorPos(win,&x,&y);
    if(cam.cursorLocked){
        if(g_firstMouse){ g_prevMouseX=x; g_prevMouseY=y; g_firstMouse=false; }
        in.mx = float(x - g_prevMouseX);
        in.my = float(y - g_prevMouseY);
        g_prevMouseX = x; g_prevMouseY = y;
    } else {
        g_firstMouse=true;
    }

    auto key = [&](int k){ return glfwGetKey(win,k)==GLFW_PRESS; };
    glm::vec2 m(0);
    if(key(GLFW_KEY_W)) m.y += 1;
    if(key(GLFW_KEY_S)) m.y -= 1;
    if(key(GLFW_KEY_D)) m.x += 1;
    if(key(GLFW_KEY_A)) m.x -= 1;
    if(glm::length(m)>0) m = glm::normalize(m);
    in.move = m;
    in.sprint = key(GLFW_KEY_LEFT_SHIFT) || key(GLFW_KEY_RIGHT_SHIFT);

    bool jp = key(GLFW_KEY_SPACE);
    in.jumpHeld = jp;
    static bool prevJ=false;
    in.jumpPressed = (jp && !prevJ) ? 1 : 0;
    prevJ = jp;

    // toggles
    static bool pPrev=false, tPrev=false, tabPrev=false, rPrev=false;
    bool pNow=key(GLFW_KEY_P), tNow=key(GLFW_KEY_T), tabNow=key(GLFW_KEY_TAB), rNow=key(GLFW_KEY_R);
    if(pNow && !pPrev) in.slowmo = true;
    if(tNow && !tPrev) in.wire   = true;
    if(tabNow && !tabPrev){ in.toggleCursor = true; cam.cursorLocked = !cam.cursorLocked; glfwSetInputMode(win, GLFW_CURSOR, cam.cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL); }
    if(rNow && !rPrev) in.reset = true;
    pPrev=pNow; tPrev=tNow; tabPrev=tabNow; rPrev=rNow;
}

// -------------------------------------------------------------
// Collision response helpers (rigid-body style impulses)
// -------------------------------------------------------------
static void applyCollisionResponse(ControllerConfig const& C, ControllerState& S, glm::vec3 n)
{
    float vn = glm::dot(S.vel, n);
    if(vn < 0.0f){
        float vOut = -(1.0f + C.restitution) * vn;
        S.vel += n * vOut;
    }
    glm::vec3 vN = glm::dot(S.vel, n) * n;
    glm::vec3 vT = S.vel - vN;
    S.vel = vN + vT * std::max(0.0f, 1.0f - C.friction);
}

// -------------------------------------------------------------
// One substep of physics
// -------------------------------------------------------------
static void physicsSubstep(const ControllerConfig& C, ControllerState& S,
                           const std::vector<AABB>& statLevel,
                           const std::vector<KinBox>& kinLevel, float timeNow,
                           float dt, const CameraState& cam, const Input& in)
{
    S.timeSinceGrounded += dt;
    S.timeSinceJumpPressed += dt;
    S.jumpHeld = in.jumpHeld;

    // Desired move (XZ) from camera yaw
    float cy = std::cos(glm::radians(cam.yaw));
    float sy = std::sin(glm::radians(cam.yaw));
    glm::vec3 fwd = glm::normalize(glm::vec3(sy,0,cy));
    glm::vec3 right= glm::normalize(glm::cross(fwd, glm::vec3(0,1,0)));
    glm::vec3 wish = fwd * in.move.y + right * in.move.x;
    if(glm::length(wish)>0) wish = glm::normalize(wish);

    float targetSpeed = in.sprint ? C.sprintSpeed : C.walkSpeed;

    // Horizontal accel
    glm::vec3 velXZ = glm::vec3(S.vel.x,0,S.vel.z);
    if(glm::length(wish) > 0){
        glm::vec3 target = wish * targetSpeed;
        glm::vec3 dv = target - velXZ;
        float accel = S.grounded ? C.groundAccel : C.airAccel;
        glm::vec3 dir = (glm::length(dv) > 1e-6f) ? glm::normalize(dv) : glm::vec3(0);
        glm::vec3 add = dir * accel * dt;
        if(glm::length(dv) <= glm::length(add)) add = dv;
        velXZ += add;
    }

    // Air speed clamp (boost if sprinting)
    if(!S.grounded){
        float sp = glm::length(velXZ);
        float maxAir = C.maxAirSpeed * (in.sprint ? C.sprintMaxAirSpeedMult : 1.0f);
        if(sp > maxAir) velXZ *= (maxAir / sp);

        // a touch more air steering while sprinting
        if(glm::length(wish)>0 && in.sprint){
            glm::vec3 target = wish * maxAir;
            glm::vec3 dv = target - velXZ;
            glm::vec3 dir = (glm::length(dv) > 1e-6f) ? glm::normalize(dv) : glm::vec3(0);
            velXZ += dir * (C.airAccel * (C.sprintAirAccelMult - 1.0f)) * dt;
        }
    }

    // Drag
    float drag = S.grounded ? C.groundDrag : C.airDrag;
    velXZ *= (1.0f / (1.0f + drag*dt));

    // Recombine horizontal + vertical
    S.vel.x = velXZ.x; S.vel.z = velXZ.z;

    // Gravity
    float g = -C.gravity;
    if(S.vel.y < 0) S.vel.y += g * C.fallGravityMult * dt;
    else            S.vel.y += g * (S.jumpHeld ? 1.0f : C.lowJumpGravityMult) * dt;

    // Jumping (buffer + coyote)
    if(in.jumpPressed) S.timeSinceJumpPressed = 0.f;
    if(S.timeSinceJumpPressed <= C.jumpBufferTime && (S.grounded || S.timeSinceGrounded <= C.coyoteTime)) {
        S.timeSinceJumpPressed = 1e3f;
        S.timeSinceGrounded = 1e3f;
        float jumpH = C.jumpHeight * (in.sprint ? C.sprintJumpBoost : 1.0f);
        S.vel.y = std::sqrt(2.0f * C.gravity * jumpH);
        S.grounded=false;
    }

    // Integrate
    glm::vec3 newPos = S.pos + S.vel * dt;

    // Collision resolution (iterate)
    glm::vec3 groundNormalAccum(0);
    bool onGround = false;

    auto collideList = [&](auto&& applyBox){
        for(const auto& b : statLevel) applyBox(b);
        for(const auto& k : kinLevel){ AABB kb = movedAABB(k, timeNow); applyBox(kb); }
    };

    for(int it=0; it<4; ++it){
        bool any=false;
        collideList([&](const AABB& b){
            glm::vec3 n; float depth;
            glm::vec3 c = newPos;
            if(resolveSphereAABB(c, C.radius, b, n, depth)){
                any=true; newPos = c; applyCollisionResponse(C, S, n);
                if(n.y > 0.5f){ onGround = true; groundNormalAccum += n; }
            }
        });
        if(!any) break;
    }

    // Grounding
    if(onGround){
        S.grounded = true; S.timeSinceGrounded = 0.f;
        if(glm::length(groundNormalAccum) > 1e-4f)
            S.groundNormal = glm::normalize(groundNormalAccum);
        if(S.vel.y < 0 && S.vel.y > -3.0f) S.vel.y = 0.0f;
    } else S.grounded = false;

    S.pos = newPos;
}

// -------------------------------------------------------------
// Multi-substep wrapper
// -------------------------------------------------------------
void stepController(const ControllerConfig& C, ControllerState& S,
                    const std::vector<AABB>& statLevel,
                    const std::vector<KinBox>& kinLevel, float timeNow,
                    float dt, const CameraState& cam, const Input& in)
{
    float speed = glm::length(S.vel);
    float maxDisp = std::max(0.001f, C.radius * 0.5f);
    int subSteps = std::clamp( (int)std::ceil((speed * dt) / maxDisp), 1, 8 );
    float h = dt / float(subSteps);

    for(int i=0;i<subSteps;i++){
        physicsSubstep(C, S, statLevel, kinLevel, timeNow + i*h, h, cam, in);
    }
}

// -------------------------------------------------------------
// Camera
// -------------------------------------------------------------
glm::mat4 computeCamera(const ControllerConfig& C, const ControllerState& S, CameraState& cam, const Input& in, float dt, int width, int height)
{
    // gentle mouse sensitivity
    const float sensitivityX = 0.045f, sensitivityY = 0.035f;
    cam.yaw   += in.mx * sensitivityX;
    cam.pitch += -in.my * sensitivityY;
    cam.pitch = clampf(cam.pitch, -40.0f, 75.0f);

    glm::vec3 target = S.pos + glm::vec3(0, C.eyeHeight, 0);
    float cy = std::cos(glm::radians(cam.yaw));
    float sy = std::sin(glm::radians(cam.yaw));
    float cp = std::cos(glm::radians(cam.pitch));
    float sp = std::sin(glm::radians(cam.pitch));
    glm::vec3 dir = glm::normalize(glm::vec3(sy*cp, sp, cy*cp));
    glm::vec3 camPos = target - dir * cam.dist + glm::vec3(0, cam.height, 0);

    static glm::vec3 smoothPos = camPos;
    float follow = 1.0f - std::exp(-12.0f * dt);
    smoothPos = glm::mix(smoothPos, camPos, follow);

    glViewport(0,0,width,height);
    glm::mat4 view = glm::lookAt(smoothPos, target, glm::vec3(0,1,0));
    float aspect = float(width)/float(height);
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.05f, 300.0f);
    return proj * view;
}

// -------------------------------------------------------------
// Rendering helpers
// -------------------------------------------------------------
void drawBox(const Mesh& cube, GLuint prog, const glm::mat4& VP, glm::vec3 center, glm::vec3 halfExtents, glm::vec3 color){
    glm::mat4 M = glm::translate(glm::mat4(1), center) * glm::scale(glm::mat4(1), halfExtents*2.0f);
    glm::mat4 MVP = VP * M;
    glUseProgram(prog);
    glUniformMatrix4fv(glGetUniformLocation(prog,"uMVP"),1,GL_FALSE, glm::value_ptr(MVP));
    glUniform3fv(glGetUniformLocation(prog,"uColor"),1, glm::value_ptr(color));
    glBindVertexArray(cube.vao);
    glDrawElements(GL_TRIANGLES, cube.indexCount, GL_UNSIGNED_INT, 0);
}
void drawSphereProxy(const Mesh& cube, GLuint prog, const glm::mat4& VP, glm::vec3 center, float radius, glm::vec3 color){
    drawBox(cube, prog, VP, center, glm::vec3(radius), color);
}

// -------------------------------------------------------------
// Main
// -------------------------------------------------------------
int main(){
    if(!glfwInit()) die("Failed to init GLFW");
    // MSAA for nicer edges
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    int W=1280, H=800;
    GLFWwindow* win = glfwCreateWindow(W,H,"OpenGL Character Controller ",nullptr,nullptr);
    if(!win){ glfwTerminate(); die("Failed to create window"); }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    GLenum err = glewInit();
    if(err != GLEW_OK) die("Failed to init GLEW");

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    // lock cursor by default
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLuint prog = makeProgram(kVert, kFrag);
    Mesh cube = makeUnitCube();

    // Level
    auto staticLevel = buildStaticLevel();
    auto kinLevel    = buildKinematics();

    // Controller & Camera
    ControllerConfig C;
    ControllerState  S;
    CameraState      cam;
    cam.cursorLocked = true;
    cam.dist = 6.0f; cam.height=0.9f;

    // Timing
    using clock = std::chrono::high_resolution_clock;
    auto t0 = clock::now();
    float accumulator=0.f;
    float simTime=0.f;
    const float fixedDt = 1.0f/120.0f;
    bool slowmo=false, wire=false;

    while(!glfwWindowShouldClose(win)){
        glfwPollEvents();
        if(glfwGetKey(win, GLFW_KEY_ESCAPE)==GLFW_PRESS) break;

        // Size
        glfwGetFramebufferSize(win, &W, &H);

        // Input
        Input in; pollInput(win, in, cam);
        if(in.reset){ S = ControllerState{}; S.pos=glm::vec3(0,2,0); }
        if(in.slowmo) slowmo = !slowmo;
        if(in.wire){ wire=!wire; glPolygonMode(GL_FRONT_AND_BACK, wire?GL_LINE:GL_FILL); }

        // Timing
        auto t1 = clock::now();
        float frame = std::chrono::duration<float>(t1 - t0).count();
        t0 = t1;
        frame = std::min(frame, 0.1f);
        accumulator += frame * (slowmo? 0.35f : 1.0f);

        // Physics: fixed steps
        int steps=0;
        while(accumulator >= fixedDt && steps<6){
            stepController(C, S, staticLevel, kinLevel, simTime, fixedDt, cam, in);
            accumulator -= fixedDt;
            simTime += fixedDt;
            steps++;
        }

        // Camera
        glm::mat4 VP = computeCamera(C, S, cam, in, std::max(fixedDt,frame), W, H);

        // Render
        glClearColor(0.52f, 0.80f, 0.98f, 1.0f); // sky
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        // Draw STATIC level (colored heuristic)
        for(const auto& b : staticLevel){
            glm::vec3 c = 0.5f*(b.mn + b.mx);
            glm::vec3 he= 0.5f*(b.mx - b.mn);
            bool isGround = (he.x>20.0f && he.z>20.0f && he.y>=0.9f);
            bool isWall   = (!isGround) && (he.y>2.0f) && (he.x>8.0f || he.z>8.0f);
            bool isStep   = (!isGround && !isWall) && (he.y < 0.15f) && (he.x > 0.3f || he.z > 0.3f);
            glm::vec3 color = isGround ? glm::vec3(0.30f, 0.90f, 0.30f)
                              : isWall  ? glm::vec3(0.55f, 0.50f, 0.45f)
                              : isStep  ? glm::vec3(0.95f, 0.65f, 0.25f)
                                        : glm::vec3(0.60f, 0.60f, 0.65f);
            drawBox(cube, prog, VP, c, he, color);
        }

        // Draw KINEMATIC level
        for(const auto& k : kinLevel){
            AABB kb = movedAABB(k, simTime);
            glm::vec3 c = 0.5f*(kb.mn + kb.mx);
            glm::vec3 he= 0.5f*(kb.mx - kb.mn);
            drawBox(cube, prog, VP, c, he, k.color);
        }

        // Draw player
        glm::vec3 playerColor = S.grounded ? glm::vec3(0.00f, 0.90f, 0.90f)
                                           : glm::vec3(0.00f, 0.70f, 1.00f);
        drawSphereProxy(cube, prog, VP, S.pos, C.radius, playerColor);

        glfwSwapBuffers(win);
    }

    glDeleteProgram(prog);
    glDeleteVertexArrays(1,&cube.vao);
    glDeleteBuffers(1,&cube.vbo);
    glDeleteBuffers(1,&cube.ebo);

    glfwTerminate();
    return 0;
}
