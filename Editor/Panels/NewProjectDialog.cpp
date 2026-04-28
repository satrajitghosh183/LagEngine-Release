#include "NewProjectDialog.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Core/ProjectFile.hpp"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

namespace fs = std::filesystem;

namespace GameEngine {

    // -------------------------------------------------------------------------
    // Destructor
    // -------------------------------------------------------------------------
    NewProjectDialog::~NewProjectDialog()
    {
        CleanupThumbnails();
    }

    // -------------------------------------------------------------------------
    // Open
    // -------------------------------------------------------------------------
    void NewProjectDialog::Open()
    {
        m_IsOpen = true;
        m_PendingOpen = true;
        m_SelectedTemplate = -1;
    }

    // -------------------------------------------------------------------------
    // Thumbnail helpers
    // -------------------------------------------------------------------------

    static void ThumbSetPixel(std::vector<uint8_t>& px, int W, int x, int y,
                               uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        if (x < 0 || x >= W || y < 0 || y >= W) return;
        int idx = (y * W + x) * 4;
        px[idx] = r; px[idx+1] = g; px[idx+2] = b; px[idx+3] = a;
    }

    static void ThumbFillBg(std::vector<uint8_t>& px, int W,
                             int r1,int g1,int b1, int r2,int g2,int b2)
    {
        for (int y = 0; y < W; y++) {
            float t = y / float(W - 1);
            uint8_t cr = (uint8_t)(r1 + (r2-r1)*t);
            uint8_t cg = (uint8_t)(g1 + (g2-g1)*t);
            uint8_t cb = (uint8_t)(b1 + (b2-b1)*t);
            for (int x = 0; x < W; x++)
                ThumbSetPixel(px, W, x, y, cr, cg, cb);
        }
    }

    static void ThumbFillRect(std::vector<uint8_t>& px, int W,
                               int x, int y, int w, int h,
                               uint8_t r, uint8_t g, uint8_t b)
    {
        for (int dy = 0; dy < h; dy++)
            for (int dx = 0; dx < w; dx++)
                ThumbSetPixel(px, W, x+dx, y+dy, r, g, b);
    }

    static void ThumbFillCircle(std::vector<uint8_t>& px, int W,
                                 int cx, int cy, int radius,
                                 uint8_t r, uint8_t g, uint8_t b)
    {
        for (int dy = -radius; dy <= radius; dy++)
            for (int dx = -radius; dx <= radius; dx++)
                if (dx*dx + dy*dy <= radius*radius)
                    ThumbSetPixel(px, W, cx+dx, cy+dy, r, g, b);
    }

    static void ThumbDrawLine(std::vector<uint8_t>& px, int W,
                               int x0, int y0, int x1, int y1,
                               uint8_t r, uint8_t g, uint8_t b)
    {
        int dx = abs(x1-x0), dy = abs(y1-y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;
        while (true) {
            ThumbSetPixel(px, W, x0, y0, r, g, b);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 <  dx) { err += dx; y0 += sy; }
        }
    }

    // -------------------------------------------------------------------------
    // GenerateThumbnail – one distinct look per template type
    // -------------------------------------------------------------------------
    void NewProjectDialog::GenerateThumbnail(const std::string& name,
                                              const std::string& category)
    {
        const int W = kThumbSize;
        std::vector<uint8_t> px(W * W * 4, 0);

        // Convenience wrappers that close over px / W
        auto SP  = [&](int x,int y,uint8_t r,uint8_t g,uint8_t b){ ThumbSetPixel(px,W,x,y,r,g,b); };
        auto BG  = [&](int r1,int g1,int b1,int r2,int g2,int b2){ ThumbFillBg(px,W,r1,g1,b1,r2,g2,b2); };
        auto FR  = [&](int x,int y,int w,int h,uint8_t r,uint8_t g,uint8_t b){ ThumbFillRect(px,W,x,y,w,h,r,g,b); };
        auto FC  = [&](int cx,int cy,int rad,uint8_t r,uint8_t g,uint8_t b){ ThumbFillCircle(px,W,cx,cy,rad,r,g,b); };
        auto DL  = [&](int x0,int y0,int x1,int y1,uint8_t r,uint8_t g,uint8_t b){ ThumbDrawLine(px,W,x0,y0,x1,y1,r,g,b); };

        auto has = [&](const char* s){ return name.find(s) != std::string::npos; };

        // ---- Deferred / Rendering ----
        if (has("Deferred") || has("Render") || category == "Graphics")
        {
            BG(22,22,55, 48,42,85);
            float cx = W*0.42f, cy = W*0.52f, rad = W*0.36f;
            glm::vec3 ldir = glm::normalize(glm::vec3(0.55f, 0.75f, 0.35f));
            for (int y = 0; y < W; y++) {
                for (int x = 0; x < W; x++) {
                    float dx = x - cx, dy = y - cy;
                    float dist = sqrtf(dx*dx+dy*dy);
                    if (dist <= rad) {
                        float nx = dx/rad, ny = -dy/rad;
                        float nz = sqrtf(fmaxf(0.f, 1.f - nx*nx - ny*ny));
                        float diff = fmaxf(0.f, nx*ldir.x + ny*ldir.y + nz*ldir.z);
                        glm::vec3 h = glm::normalize(ldir + glm::vec3(0,0,1));
                        float spec = powf(fmaxf(0.f, nx*h.x+ny*h.y+nz*h.z), 48.f);
                        float r = fminf(1.f, 0.12f + diff*0.55f + spec*1.1f);
                        float g = fminf(1.f, 0.20f + diff*0.45f + spec*0.90f);
                        float b = fminf(1.f, 0.35f + diff*0.60f + spec*1.0f);
                        SP(x, y, (uint8_t)(r*255), (uint8_t)(g*255), (uint8_t)(b*255));
                    }
                }
            }
            float cx2 = W*0.75f, cy2 = W*0.28f, rad2 = W*0.16f;
            for (int y = 0; y < W; y++) {
                for (int x = 0; x < W; x++) {
                    float dx = x-cx2, dy = y-cy2;
                    if (dx*dx+dy*dy <= rad2*rad2) {
                        float t = sqrtf(dx*dx+dy*dy)/rad2;
                        SP(x,y,(uint8_t)((1-t*0.6f)*240),(uint8_t)((1-t*0.4f)*200),(uint8_t)((1-t*0.3f)*130));
                    }
                }
            }
            for (int gx = 0; gx < W; gx += 18) DL(gx,W-10,gx+12,W-2, 50,55,70);
            FR(0, W-4, W, 4, 40,44,56);
        }

        // ---- SPH Fluid ----
        else if (has("SPH") || has("Fluid") || category == "Fluid")
        {
            BG(18,62,108, 28,95,155);
            for (int x = 0; x < W; x++) {
                int waveY = (int)(W*0.42f + 9.f*sinf(x*0.18f) + 4.f*sinf(x*0.35f+1.2f));
                for (int y = waveY; y < W-4; y++) {
                    float d = (y - waveY) / float(W-4 - waveY);
                    SP(x, y,
                       (uint8_t)(12  + 28*(1-d)),
                       (uint8_t)(90  + 55*(1-d)),
                       (uint8_t)(155 + 70*(1-d)));
                }
            }
            for (int x = 0; x < W; x++) {
                int waveY = (int)(W*0.42f + 9.f*sinf(x*0.18f) + 4.f*sinf(x*0.35f+1.2f));
                for (int t = -1; t <= 1; t++) SP(x, waveY+t, 200,230,255);
            }
            uint32_t seed = 0xBEEF;
            for (int i = 0; i < 35; i++) {
                seed = seed*1664525u + 1013904223u;
                int px2 = 8 + (seed>>24)%(W-16);
                seed = seed*1664525u + 1013904223u;
                int py2 = 8 + (seed>>24)%(int)(W*0.38f);
                FC(px2, py2, 2, 100, 210, 255);
            }
            FR(0, W-4, W, 4, 55,60,80);
        }

        // ---- Robot Arm ----
        else if (has("Robot") || category == "Robotics")
        {
            BG(48,52,68, 72,80,105);
            FR(38, 104, 52, 12, 75, 80, 95);
            FR(38, 102, 52,  3, 95,100,118);
            FC(64, 103, 9, 130,135,155);
            for (int i = 0; i < 38; i++) {
                int ax = 64 - (int)(i*0.15f);
                int ay = 103 - i;
                FR(ax-5, ay, 10, 3, 155,160,180);
            }
            FC(57, 65, 7, 130,135,155);
            for (int i = 0; i < 32; i++) {
                int ax = 57 + (int)(i*0.9f);
                int ay = 65 - (int)(i*0.5f);
                FR(ax-4, ay, 8, 3, 145,150,170);
            }
            FC(86, 49, 6, 130,135,155);
            for (int i = 0; i < 18; i++) {
                int ax = 86 + (int)(i*0.7f);
                int ay = 49 + (int)(i*0.6f);
                FR(ax-3, ay, 6, 3, 140,145,165);
            }
            FC(99, 59, 5, 130,135,155);
            FR(94, 59, 3, 14, 185,190,210);
            FR(101,59, 3, 14, 185,190,210);
            FR(0, 116, W, 2, 55,60,72);
            for (int gx = 5; gx < W; gx += 14) DL(gx,116,gx-5,W-2, 48,52,64);
            DL(12,108, 22,108, 220,60,60);
            DL(12,108, 12, 98, 60,200,60);
            DL(12,108, 19,101, 60,100,220);
        }

        // ---- Particle Cannon ----
        else if (has("Cannon"))
        {
            BG(28,28,68, 52,50,110);
            FR(0, 100, W, 5, 55,58,72);
            FR(0,  98, W, 3, 70,73,88);
            for (int i = 0; i < 32; i++) {
                int bx = 14 + (int)(i*0.77f);
                int by = 98 - (int)(i*0.64f);
                FR(bx-4, by-4, 8, 8, 90,95,115);
            }
            FC(14, 98, 9, 110,115,135);
            for (int i = 0; i < 22; i++) {
                float t = i / 21.f;
                int ax = (int)(40 + t*82.f);
                int ay = (int)(85 - 62.f*(4*t*(1-t)));
                float age = t;
                uint8_t r = (uint8_t)(255);
                uint8_t g = (uint8_t)(80 + 160*age);
                uint8_t b = (uint8_t)(30*(1-age));
                FC(ax, ay, (i%3==0)?3:2, r, g, b);
            }
            FC(122, 85, 6, 255,220,100);
            FC(122, 85, 3, 255,255,200);
        }

        // ---- Cloth / Flag ----
        else if (has("Cloth") || has("Flag"))
        {
            BG(22,72,32, 42,115,52);
            const int GW = 10, GH = 8;
            float cellX = (W-20.f)/GW;
            float cellY = (W-24.f)/GH;
            for (int gy = 0; gy <= GH; gy++) {
                for (int gx = 0; gx < GW; gx++) {
                    float w0 = 6.f*sinf(gx*0.45f + gy*0.28f);
                    float w1 = 6.f*sinf((gx+1)*0.45f + gy*0.28f);
                    int x0 = (int)(10+gx*cellX), y0 = (int)(12+gy*cellY+w0);
                    int x1 = (int)(10+(gx+1)*cellX), y1 = (int)(12+gy*cellY+w1);
                    DL(x0,y0,x1,y1, 70,190,100);
                }
            }
            for (int gx = 0; gx <= GW; gx++) {
                for (int gy = 0; gy < GH; gy++) {
                    float w0 = 6.f*sinf(gx*0.45f + gy*0.28f);
                    float w1 = 6.f*sinf(gx*0.45f + (gy+1)*0.28f);
                    int x0 = (int)(10+gx*cellX), y0 = (int)(12+gy*cellY+w0);
                    int x1 = (int)(10+gx*cellX), y1 = (int)(12+(gy+1)*cellY+w1);
                    DL(x0,y0,x1,y1, 55,165,80);
                }
            }
            for (int gy = 0; gy <= GH; gy++)
                for (int gx = 0; gx <= GW; gx++) {
                    float w = 6.f*sinf(gx*0.45f + gy*0.28f);
                    int nx = (int)(10+gx*cellX), ny = (int)(12+gy*cellY+w);
                    FC(nx, ny, 2, 160,255,160);
                }
            for (int gx = 0; gx <= GW; gx++) {
                int nx = (int)(10+gx*cellX);
                FC(nx, 12, 3, 255,200,80);
            }
        }

        // ---- Soft Body / XPBD ----
        else if (has("Soft") || has("XPBD") || has("SoftBody"))
        {
            BG(62,22,108, 92,38,148);
            int cx = W/2, cy = W/2+8, numPts = 14;
            for (int i = 0; i < numPts; i++) {
                float a = i * 6.2832f / numPts;
                float r = 28.f + 11.f*sinf(a*3.f + 0.6f);
                int bx = cx + (int)(r*cosf(a));
                int by = cy + (int)(r*sinf(a));
                uint8_t cr = (uint8_t)(155+40*sinf(a));
                uint8_t cg = (uint8_t)(70+20*cosf(a));
                FC(bx, by, 14, cr, cg, 230);
            }
            FC(cx, cy, 22, 185, 105, 245);
            FC(cx-10, cy-13, 7, 225, 190, 255);
            FC(cx- 7, cy-10, 3, 255, 240, 255);
            for (int i = 0; i < numPts; i++) {
                float a0 = i * 6.2832f / numPts;
                float a1 = (i+1) * 6.2832f / numPts;
                float r0 = 28.f + 11.f*sinf(a0*3.f+0.6f);
                float r1 = 28.f + 11.f*sinf(a1*3.f+0.6f);
                int x0 = cx+(int)(r0*cosf(a0)), y0 = cy+(int)(r0*sinf(a0));
                int x1 = cx+(int)(r1*cosf(a1)), y1 = cy+(int)(r1*sinf(a1));
                DL(x0,y0,x1,y1, 120,60,180);
            }
            FR(0, W-4, W, 4, 50,30,72);
        }

        // ---- 2D Platformer ----
        else if (has("2D") || has("Platformer") || category == "2D")
        {
            BG(25,42,100, 48,72,152);
            uint32_t seed = 0xCAFE;
            for (int i = 0; i < 30; i++) {
                seed = seed*1664525u+1013904223u;
                int sx = (seed>>24)%W;
                seed = seed*1664525u+1013904223u;
                int sy = (seed>>24)%(W/2);
                SP(sx, sy, 220,225,235);
            }
            FC(102, 18, 10, 240,240,200);
            FC(107, 14,  8, 28,36,64);
            FR(0, 106, W, 14, 68, 108, 52);
            FR(0, 104, W,  4, 88, 140, 62);
            FR(10, 78, 32, 8, 68,108,52); FR(10, 76, 32, 3, 88,140,62);
            FR(55, 58, 36, 8, 68,108,52); FR(55, 56, 36, 3, 88,140,62);
            FR(82, 84, 32, 8, 68,108,52); FR(82, 82, 32, 3, 88,140,62);
            FR(20, 90, 10, 14, 230, 90, 70);
            FR(22, 85,  7,  7, 250,195,145);
            SP(24, 87, 40,40,40); SP(27, 87, 40,40,40);
            FC(70, 50, 5, 255,210,40);
            FC(70, 50, 3, 255,240,120);
        }

        // ---- GPU Particles ----
        else if (has("Particle") || category == "Particles")
        {
            BG(8,8,22, 22,16,42);
            int emitX = W/2, emitY = W-12;
            FC(emitX, emitY, 7, 255,180,60);
            FC(emitX, emitY, 4, 255,240,160);
            uint32_t seed = 0xF00D;
            for (int i = 0; i < 90; i++) {
                seed = seed*1664525u+1013904223u;
                float angle = -2.7f + (float)((seed>>22)%100)/100.f*2.8f;
                seed = seed*1664525u+1013904223u;
                float speed = 22.f + (float)((seed>>22)%55);
                seed = seed*1664525u+1013904223u;
                float age = (float)((seed>>20)%100)/100.f;
                float ppx = emitX + cosf(angle)*speed*age;
                float ppy = emitY - sinf(angle)*speed*age + 9.8f*age*age*8.f;
                uint8_t r = (uint8_t)fminf(255.f, 255.f);
                uint8_t g = (uint8_t)fminf(255.f, 220.f*(1-age*0.7f));
                uint8_t b = (uint8_t)fminf(255.f, 60.f*(1-age));
                uint8_t a2 = (uint8_t)(255*(1-age*0.6f));
                int px2 = (int)ppx, py2 = (int)ppy;
                if (px2>=0&&px2<W&&py2>=0&&py2<W)
                    ThumbSetPixel(px, W, px2, py2, r, g, b, a2);
                if (i%7==0) FC((int)ppx,(int)ppy, 2, r, g, b);
            }
            FR(0, W-5, W, 5, 38,38,50);
        }

        // ---- Default: Basic Physics (falling boxes) ----
        else
        {
            BG(38,55,120, 58,82,160);
            FR(0, 106, W, 10, 65,70,88);
            FR(0, 104, W,  3, 85,90,108);
            FR(10, 16, 24, 24, 195,55,55);
            FR(10, 16, 24,  3, 240,80,80);
            FR(10, 16,  3, 24, 240,80,80);
            FR(50, 52, 24, 24, 205,125,35);
            FR(50, 52, 24,  3, 245,155,55);
            FR(50, 52,  3, 24, 245,155,55);
            FR(90, 78, 22, 22, 215,198,35);
            FR(90, 78, 22,  3, 248,228,55);
            FR(90, 78,  3, 22, 248,228,55);
            auto arrow = [&](int ax, int topY) {
                DL(ax, topY, ax, topY+12, 100,130,210);
                DL(ax, topY+12, ax-3, topY+7, 100,130,210);
                DL(ax, topY+12, ax+3, topY+7, 100,130,210);
            };
            arrow(22, 42);
            arrow(62, 78);
            arrow(101,102);
            FC(110, 22, 10, 80,180,240);
            FC(106, 18,  4, 160,220,255);
        }

        // Upload pixels to a Vulkan texture and register with ImGui
        ThumbnailEntry entry;
        entry.Texture = CreateRef<Texture2D>(static_cast<uint32_t>(W),
                                              static_cast<uint32_t>(W),
                                              TextureFormat::RGBA);
        entry.Texture->SetData(px.data(), static_cast<uint32_t>(px.size()));
        entry.ImGuiDescriptor = ImGui_ImplVulkan_AddTexture(
            entry.Texture->GetSampler(),
            entry.Texture->GetImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_Thumbnails.push_back(std::move(entry));
    }

    // -------------------------------------------------------------------------
    void NewProjectDialog::InitThumbnails()
    {
        const auto& templates = TemplateManager::GetTemplates();
        m_Thumbnails.clear();
        m_Thumbnails.reserve(templates.size());
        for (const auto& tmpl : templates)
            GenerateThumbnail(tmpl.Name, tmpl.Category);
        m_ThumbnailsInitialized = true;
    }

    void NewProjectDialog::CleanupThumbnails()
    {
        for (auto& entry : m_Thumbnails) {
            if (entry.ImGuiDescriptor != VK_NULL_HANDLE)
                ImGui_ImplVulkan_RemoveTexture(entry.ImGuiDescriptor);
        }
        m_Thumbnails.clear();
        m_ThumbnailsInitialized = false;
    }

    // -------------------------------------------------------------------------
    // OnImGuiRender
    // -------------------------------------------------------------------------
    void NewProjectDialog::OnImGuiRender()
    {
        if (!m_IsOpen) return;

        if (m_PendingOpen) {
            ImGui::OpenPopup("New Project from Template");
            m_PendingOpen = false;
        }

        // Generate thumbnails once (needs Vulkan context → do it here, not in Open())
        if (!m_ThumbnailsInitialized)
            InitThumbnails();

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(760, 620), ImGuiCond_Appearing);

        if (!ImGui::BeginPopupModal("New Project from Template", &m_IsOpen,
                                    ImGuiWindowFlags_NoResize))
            return;

        const auto& templates = TemplateManager::GetTemplates();

        ImGui::Text("Select a template:");
        ImGui::Separator();

        // ---- Template grid --------------------------------------------------
        constexpr int   kColumns  = 3;
        constexpr float kImgH     = 90.f;   // thumbnail display height in card
        constexpr float kCardH    = kImgH + 62.f; // + text area

        ImGui::BeginChild("TemplateGrid", ImVec2(0, 410), true);
        float cellW = ImGui::GetContentRegionAvail().x / kColumns;
        ImGui::Columns(kColumns, nullptr, false);

        for (int i = 0; i < (int)templates.size(); i++)
        {
            const auto& tmpl = templates[i];
            ImGui::PushID(i);

            bool isSelected = (m_SelectedTemplate == i);
            float imgW = cellW - 14.f;

            // ---- Thumbnail (Image acts as the clickable button) ----
            ImVec2 cardTopLeft = ImGui::GetCursorScreenPos();

            // Selected highlight behind the image
            auto* dl = ImGui::GetWindowDrawList();
            if (isSelected) {
                dl->AddRectFilled(
                    cardTopLeft,
                    ImVec2(cardTopLeft.x + imgW, cardTopLeft.y + kCardH),
                    IM_COL32(35,75,155,90), 4.f);
            }

            // Draw thumbnail using ImGui::Image with Vulkan descriptor set
            ImTextureID texId = (i < (int)m_Thumbnails.size() && m_Thumbnails[i].ImGuiDescriptor)
                ? (ImTextureID)m_Thumbnails[i].ImGuiDescriptor
                : (ImTextureID)nullptr;

            if (texId) {
                ImGui::Image(texId, ImVec2(imgW, kImgH));
            } else {
                // Placeholder rect when no texture
                dl->AddRectFilled(cardTopLeft,
                    ImVec2(cardTopLeft.x + imgW, cardTopLeft.y + kImgH),
                    IM_COL32(50,55,75,255), 3.f);
                ImGui::Dummy(ImVec2(imgW, kImgH));
            }

            // Detect click on the image
            if (ImGui::IsItemClicked()) m_SelectedTemplate = i;

            // Border around image
            {
                ImVec2 bMin = ImGui::GetItemRectMin();
                ImVec2 bMax = ImGui::GetItemRectMax();
                uint32_t col = isSelected ? IM_COL32(100,185,255,255) : IM_COL32(75,80,105,180);
                float    bw  = isSelected ? 2.f : 1.f;
                dl->AddRect(bMin, bMax, col, 3.f, 0, bw);
            }

            // ---- Template name ----
            float textY = cardTopLeft.y + kImgH + 5.f;
            ImGui::SetCursorScreenPos(ImVec2(cardTopLeft.x + 4.f, textY));
            ImGui::TextUnformatted(tmpl.Name.c_str());
            if (ImGui::IsItemClicked()) m_SelectedTemplate = i;

            // ---- Category tag ----
            ImGui::SetCursorScreenPos(ImVec2(cardTopLeft.x + 4.f, textY + 16.f));
            ImGui::TextColored(ImVec4(0.55f, 0.80f, 1.0f, 1.0f),
                               "[%s]", tmpl.Category.c_str());
            if (ImGui::IsItemClicked()) m_SelectedTemplate = i;

            // ---- Short description ----
            if (!tmpl.Description.empty()) {
                ImGui::SetCursorScreenPos(ImVec2(cardTopLeft.x + 4.f, textY + 32.f));
                std::string desc = tmpl.Description;
                if (desc.size() > 42) { desc.resize(42); desc += "..."; }
                ImGui::TextDisabled("%s", desc.c_str());
                if (ImGui::IsItemClicked()) m_SelectedTemplate = i;
            }

            // Advance cursor past the full card height so column layout is correct
            ImGui::SetCursorScreenPos(ImVec2(cardTopLeft.x, cardTopLeft.y + kCardH + 4.f));
            ImGui::Dummy(ImVec2(0.f, 0.f));

            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::EndChild();

        // ---- Project settings -----------------------------------------------
        ImGui::Spacing();
        ImGui::InputText("Project Name",    m_ProjectName, sizeof(m_ProjectName));
        ImGui::InputText("Target Directory", m_ProjectPath, sizeof(m_ProjectPath));
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---- Buttons --------------------------------------------------------
        bool canCreate = (m_SelectedTemplate >= 0 &&
                          m_SelectedTemplate < (int)templates.size() &&
                          strlen(m_ProjectName) > 0 &&
                          strlen(m_ProjectPath) > 0);

        if (!canCreate) ImGui::BeginDisabled();

        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            const auto& tmpl = templates[m_SelectedTemplate];
            fs::path targetDir = fs::path(m_ProjectPath) / m_ProjectName;

            if (TemplateManager::CreateProjectFromTemplate(tmpl, targetDir.string()))
            {
                GE_CORE_INFO("NewProjectDialog: project created at '{}'", targetDir.string());

                ProjectFile proj;
                proj.Name         = m_ProjectName;
                proj.DefaultScene = tmpl.SceneFile.empty() ? "" : tmpl.SceneFile;
                proj.AssetsRoot   = "Assets";
                ProjectFile::Save(targetDir.string(), proj);

                if (m_Callback) {
                    std::string scenePath = !tmpl.SceneFile.empty()
                        ? (targetDir / tmpl.SceneFile).string()
                        : targetDir.string();
                    m_Callback(scenePath);
                }

                m_IsOpen = false;
                ImGui::CloseCurrentPopup();
            }
        }

        if (!canCreate) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            m_IsOpen = false;
            ImGui::CloseCurrentPopup();
        }

        // Selected template description panel (bottom-right of buttons)
        if (m_SelectedTemplate >= 0 && m_SelectedTemplate < (int)templates.size())
        {
            ImGui::SameLine();
            ImGui::Spacing(); ImGui::SameLine();
            ImGui::TextDisabled("  %s", templates[m_SelectedTemplate].Description.c_str());
        }

        ImGui::EndPopup();
    }

} // namespace GameEngine
