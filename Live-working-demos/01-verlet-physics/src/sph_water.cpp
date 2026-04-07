// Single-file 3D SPH Water (CPU) + OpenGL billboard spheres (WSL/WSLg OK)
// Build: g++ -O3 -std=c++17 sph_water.cpp -o sph_water -lglfw -lGLEW -lGL -ldl -lpthread -lX11 -lXrandr -lXi

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static int   WIN_W=1280, WIN_H=800;
static bool  paused=false;

struct float3 { float x,y,z; };
static inline float3 make3(float x=0,float y=0,float z=0){ return {x,y,z}; }
static inline float3 operator+(const float3&a,const float3&b){ return {a.x+b.x,a.y+b.y,a.z+b.z}; }
static inline float3 operator-(const float3&a,const float3&b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
static inline float3 operator*(const float3&a,float s){ return {a.x*s,a.y*s,a.z*s}; }
static inline float3 operator/(const float3&a,float s){ return {a.x/s,a.y/s,a.z/s}; }
static inline float  dot(const float3&a,const float3&b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static inline float3 cross(const float3&a,const float3&b){ return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
static inline float  len(const float3&a){ return std::sqrt(dot(a,a)); }
static inline float3 norm(const float3&a){ float l=len(a); return l>1e-6f?a/l:make3(); }

struct Camera {
    float3 target=make3(0.0f,0.6f,0.0f);
    float  dist=2.2f;
    float  yaw=1.9f, pitch=-0.25f;
    float  fov=45.0f;
    float3 eye() const {
        float cy=std::cos(yaw), sy=std::sin(yaw);
        float cp=std::cos(pitch), sp=std::sin(pitch);
        float3 d = {cp*cy, sp, cp*sy};
        return target - d*dist;
    }
} cam;

static bool lmb=false, rmb=false; static double lastX=0,lastY=0;

static void cursor_cb(GLFWwindow*, double x, double y){
    double dx=x-lastX, dy=y-lastY; lastX=x; lastY=y;
    if(lmb){ cam.yaw   -= (float)dx*0.005f; cam.pitch -= (float)dy*0.005f; cam.pitch = std::max(-1.55f,std::min(1.2f,cam.pitch)); }
    if(rmb){ float3 f={std::cos(cam.yaw),0,std::sin(cam.yaw)};
             float3 r={-f.z,0,f.x};
             cam.target = cam.target + r*(float)(-dx*0.002f*cam.dist) + f*(float)(dy*0.002f*cam.dist); }
}
static void button_cb(GLFWwindow*, int b,int a,int){ if(b==GLFW_MOUSE_BUTTON_LEFT) lmb=a!=GLFW_RELEASE; if(b==GLFW_MOUSE_BUTTON_RIGHT) rmb=a!=GLFW_RELEASE; }
static void scroll_cb(GLFWwindow*, double , double o){ cam.dist = std::max(0.5f, std::min(6.0f, cam.dist*(float)std::pow(0.9, o))); }
static void size_cb(GLFWwindow*, int w,int h){ WIN_W=w; WIN_H=h; glViewport(0,0,w,h); }
static void key_cb(GLFWwindow* w,int key,int, int action, int){
    if(action!=GLFW_PRESS) return;
    if(key==GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w,1);
    if(key==GLFW_KEY_SPACE) paused=!paused;
}

// ----------- SPH -----------
struct SPH {
    // Tuned for “nice” look
    int   N = 18000;         // particles (can bump to ~30k)
    float h = 0.045f;        // smoothing length
    float rest = 1000.0f;    // rest density
    float k = 1.6f;          // pressure coeff
    float visc = 0.15f;      // viscosity
    float dt = 0.004f;
    float3 gravity = {0,-9.81f,0};
    float radius = 0.014f;   // render radius
    float bounce = 0.4f;

    // domain (box)
    float3 minB = {-0.6f, 0.0f, -0.3f};
    float3 maxB = { 0.6f, 1.0f,  0.3f};

    // state
    std::vector<float3> x, v, f;
    std::vector<float>  rho, P;

    // uniform grid
    int gx,gy,gz;
    float cell;
    std::vector<int> head, next, order;

    SPH(){ init(); }

    void init(){
        // spawn a block of water
        int nx=36, ny=25, nz=18; // yields ~16200
        N=nx*ny*nz;
        x.resize(N); v.assign(N, make3()); f.assign(N, make3());
        rho.resize(N, rest); P.resize(N); order.resize(N);
        cell = h; gx = (int)std::ceil((maxB.x-minB.x)/cell);
        gy = (int)std::ceil((maxB.y-minB.y)/cell);
        gz = (int)std::ceil((maxB.z-minB.z)/cell);
        head.assign(gx*gy*gz, -1); next.resize(N);

        float3 start = {minB.x+0.1f, minB.y+0.1f, minB.z+0.08f};
        float3 step = {0.028f, 0.028f, 0.028f};
        int i=0;
        for(int iz=0; iz<nz; ++iz)
        for(int iy=0; iy<ny; ++iy)
        for(int ix=0; ix<nx; ++ix){
            x[i] = {start.x + ix*step.x, start.y + iy*step.y, start.z + iz*step.z};
            ++i;
        }
    }

    inline int clampi(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }
    inline int hash3(int ix,int iy,int iz){
        ix=clampi(ix,0,gx-1); iy=clampi(iy,0,gy-1); iz=clampi(iz,0,gz-1);
        return ix + iy*gx + iz*gx*gy;
    }
    inline void cellOf(const float3& p, int& ix,int& iy,int& iz){
        ix = (int)std::floor((p.x-minB.x)/cell);
        iy = (int)std::floor((p.y-minB.y)/cell);
        iz = (int)std::floor((p.z-minB.z)/cell);
    }
    void buildGrid(){
        std::fill(head.begin(), head.end(), -1);
        for(int i=0;i<N;++i){
            int ix,iy,iz; cellOf(x[i],ix,iy,iz);
            int hsh = hash3(ix,iy,iz);
            next[i] = head[hsh];
            head[hsh] = i;
        }
    }

    // kernels
    float Wpoly6(float r2){
        float c = (315.0f/(64.0f*M_PI*std::pow(h,9)));
        float t = h*h - r2; if (t<=0) return 0.0f; return c*t*t*t;
    }
    float3 gradSpiky(const float3& rij, float r){
        if(r<=0 || r>=h) return make3();
        float c = -45.0f/(M_PI*std::pow(h,6));
        float s = c * ((h-r)*(h-r)) / r;
        return rij * s;
    }
    float lapVisc(float r){
        if(r>=h) return 0.0f;
        float c = 45.0f/(M_PI*std::pow(h,6));
        return c*(h-r);
    }

    void step(){
        buildGrid();

        // density
        std::fill(rho.begin(), rho.end(), 0.0f);
        for(int i=0;i<N;++i){
            int ix,iy,iz; cellOf(x[i],ix,iy,iz);
            for(int dz=-1; dz<=1; ++dz)
            for(int dy=-1; dy<=1; ++dy)
            for(int dx=-1; dx<=1; ++dx){
                int hsh = hash3(ix+dx,iy+dy,iz+dz);
                for(int j=head[hsh]; j!=-1; j=next[j]){
                    float3 rij = x[i]-x[j];
                    float r2 = dot(rij,rij);
                    rho[i] += Wpoly6(r2);
                }
            }
            rho[i] = std::max(rho[i], rest*0.5f);
            P[i] = k*(rho[i]-rest);
        }

        // forces
        std::fill(f.begin(), f.end(), make3());
        for(int i=0;i<N;++i){
            int ix,iy,iz; cellOf(x[i],ix,iy,iz);
            float3 fP=make3(), fV=make3();
            for(int dz=-1; dz<=1; ++dz)
            for(int dy=-1; dy<=1; ++dy)
            for(int dx=-1; dx<=1; ++dx){
                int hsh = hash3(ix+dx,iy+dy,iz+dz);
                for(int j=head[hsh]; j!=-1; j=next[j]){
                    if(j==i) continue;
                    float3 rij = x[i]-x[j];
                    float r = len(rij);
                    if(r>=h || r<1e-6f) continue;
                    float3 gW = gradSpiky(rij, r);
                    float  viscL = lapVisc(r);
                    // Symmetric pressure force
                    fP = fP - gW * ((P[i]+P[j])/(2.0f*rho[j]));
                    // Viscosity
                    fV = fV + (v[j]-v[i]) * (visc * viscL / rho[j]);
                }
            }
            float3 fg = gravity;
            f[i] = fP + fV + fg;
        }

        // integrate + collisions
        const float dtc = dt;
        for(int i=0;i<N;++i){
            v[i] = v[i] + f[i]*dtc;
            x[i] = x[i] + v[i]*dtc;

            // collide with box
            if(x[i].x < minB.x){ x[i].x = minB.x; v[i].x *= -bounce; }
            if(x[i].x > maxB.x){ x[i].x = maxB.x; v[i].x *= -bounce; }
            if(x[i].y < minB.y){ x[i].y = minB.y; v[i].y *= -bounce; }
            if(x[i].y > maxB.y){ x[i].y = maxB.y; v[i].y *= -bounce; }
            if(x[i].z < minB.z){ x[i].z = minB.z; v[i].z *= -bounce; }
            if(x[i].z > maxB.z){ x[i].z = maxB.z; v[i].z *= -bounce; }
        }
    }
} sph;

// ---------- GL ----------
static const char* VS = R"GLSL(
#version 130
uniform mat4 uView, uProj;
uniform float uPointScale;
in vec3 aPos;        // particle position
out vec3 vEyePos;    // eye-space position
void main(){
    vec4 eye = uView * vec4(aPos,1.0);
    vEyePos = eye.xyz;
    gl_Position = uProj * eye;
    // compute point size so each particle keeps constant world-space radius
    float dist = -eye.z;
    gl_PointSize = max(1.0, uPointScale / max(0.001, dist));
}
)GLSL";

static const char* FS = R"GLSL(
#version 130
in vec3 vEyePos;
out vec4 FragColor;
// Pretty water look with a sphere impostor + Fresnel + simple specular
uniform vec3 uLightDir;      // normalized (eye space)
uniform vec3 uEnvTop;        // skylight tint
uniform vec3 uEnvBot;        // ground tint
uniform float uRadius;       // world radius
void main(){
    // gl_PointCoord in [0,1]^2 → map to [-1,1]^2
    vec2 uv = gl_PointCoord*2.0 - 1.0;
    float r2 = dot(uv,uv);
    if(r2>1.0) discard;               // outside circle = discard
    // sphere normal in billboard space
    float z = sqrt(max(0.0, 1.0 - r2));
    vec3 n = normalize(vec3(uv.x, uv.y, z)); // local normal
    // eye vector points toward camera in eye-space
    vec3 V = normalize(-vEyePos);
    // simple shading
    float NdotL = max(0.0, dot(n, uLightDir));
    float spec = pow(max(0.0, dot(reflect(-uLightDir, n), V)), 64.0);

    // fake environment gradient
    float fres = pow(1.0 - max(0.0, dot(n,V)), 3.0);
    vec3 env = mix(uEnvBot, uEnvTop, clamp(n.y*0.5+0.5,0.0,1.0));
    vec3 base = mix(vec3(0.02,0.10,0.9), env, fres); // deep blue → sky at grazing
    vec3 col = base*(0.2 + 0.8*NdotL) + spec*vec3(1.0);

    // a touch of translucency
    float alpha = 0.35 + 0.25*fres;
    FragColor = vec4(col, alpha);
}
)GLSL";

static GLuint prog=0, vao=0, vbo=0;
static GLint locView=-1, locProj=-1, locPoint=-1, locLight=-1, locEnvT=-1, locEnvB=-1, locRad=-1;

static GLuint compile(GLenum t,const char* s){
    GLuint sh=glCreateShader(t); glShaderSource(sh,1,&s,nullptr); glCompileShader(sh);
    GLint ok; glGetShaderiv(sh,GL_COMPILE_STATUS,&ok); if(!ok){ char log[4096]; glGetShaderInfoLog(sh,4096,nullptr,log); fprintf(stderr,"%s\n",log); exit(1);}
    return sh;
}
static void gl_init(){
    GLuint vs=compile(GL_VERTEX_SHADER,VS), fs=compile(GL_FRAGMENT_SHADER,FS);
    prog=glCreateProgram(); glAttachShader(prog,vs); glAttachShader(prog,fs);
    glBindAttribLocation(prog,0,"aPos");
    glLinkProgram(prog); GLint ok; glGetProgramiv(prog,GL_LINK_STATUS,&ok);
    if(!ok){ char log[4096]; glGetProgramInfoLog(prog,4096,nullptr,log); fprintf(stderr,"%s\n",log); exit(1);}
    glDeleteShader(vs); glDeleteShader(fs);

    glGenVertexArrays(1,&vao); glBindVertexArray(vao);
    glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER, sph.N*sizeof(float3), sph.x.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float3),(void*)0);
    glEnableVertexAttribArray(0);

    locView  = glGetUniformLocation(prog,"uView");
    locProj  = glGetUniformLocation(prog,"uProj");
    locPoint = glGetUniformLocation(prog,"uPointScale");
    locLight = glGetUniformLocation(prog,"uLightDir");
    locEnvT  = glGetUniformLocation(prog,"uEnvTop");
    locEnvB  = glGetUniformLocation(prog,"uEnvBot");
    locRad   = glGetUniformLocation(prog,"uRadius");

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(1,1,1,1);
}

// minimal mat4
struct Mat4{ float m[16]; };
static Mat4 lookAt(const float3& eye,const float3& center,const float3& up){
    float3 f=norm(center-eye); float3 r=norm(cross(f,up)); float3 u=cross(r,f);
    Mat4 M={ r.x, u.x,-f.x, 0,
             r.y, u.y,-f.y, 0,
             r.z, u.z,-f.z, 0,
            -dot(r,eye), -dot(u,eye), dot(f,eye), 1 };
    return M;
}
static Mat4 perspective(float fovy,float aspect,float znear,float zfar){
    float f=1.0f/std::tan(fovy*0.5f*(float)M_PI/180.0f);
    Mat4 M={ f/aspect,0,0,0,  0,f,0,0,  0,0,(zfar+znear)/(znear-zfar),-1,  0,0,(2*zfar*znear)/(znear-zfar),0 };
    return M;
}

int main(){
    if(!glfwInit()){ fprintf(stderr,"GLFW init failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,0); // compat with WSLg
    GLFWwindow* win=glfwCreateWindow(WIN_W,WIN_H,"SPH Water — LMB orbit | RMB pan | Scroll zoom | Space pause",nullptr,nullptr);
    if(!win){ fprintf(stderr,"Failed to create window\n"); return 1; }
    glfwMakeContextCurrent(win); glfwSwapInterval(1);
    if(glewInit()!=GLEW_OK){ fprintf(stderr,"GLEW init failed\n"); return 1; }
    glfwSetCursorPosCallback(win,cursor_cb);
    glfwSetMouseButtonCallback(win,button_cb);
    glfwSetScrollCallback(win,scroll_cb);
    glfwSetFramebufferSizeCallback(win,size_cb);
    glfwSetKeyCallback(win,key_cb);

    gl_init();

    double lastTime=glfwGetTime();
    while(!glfwWindowShouldClose(win)){
        double t=glfwGetTime(); double dt=t-lastTime; lastTime=t;
        // physics (fixed-step for stability)
        if(!paused){
            const int sub=2; // 2 substeps/frame
            for(int s=0;s<sub;++s) sph.step();
        }

        // upload positions
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sph.N*sizeof(float3), sph.x.data());

        // camera and draw
        float3 eye=cam.eye();
        Mat4 V=lookAt(eye, cam.target, make3(0,1,0));
        Mat4 P=perspective(cam.fov, (float)WIN_W/(float)WIN_H, 0.02f, 20.0f);

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog);
        glUniformMatrix4fv(locView,1,GL_FALSE,V.m);
        glUniformMatrix4fv(locProj,1,GL_FALSE,P.m);
        glUniform1f(locPoint, sph.radius * 1400.0f); // tune sprite size vs distance
        glUniform3f(locLight, norm(make3(0.6f,0.8f,0.3f)).x, norm(make3(0.6f,0.8f,0.3f)).y, norm(make3(0.6f,0.8f,0.3f)).z);
        glUniform3f(locEnvT, 0.7f,0.85f,1.0f);
        glUniform3f(locEnvB, 0.9f,0.95f,1.0f);
        glUniform1f(locRad, sph.radius);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, sph.N);

        // draw a simple floor rectangle (optional)
        // (omitted for brevity — fluid is the star)

        glfwSwapBuffers(win);
        glfwPollEvents();
    }
    glfwTerminate(); return 0;
}
