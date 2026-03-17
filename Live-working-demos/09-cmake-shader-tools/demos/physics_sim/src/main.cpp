#include "sph.h"
#include "renderer.h"
#include "gui.h"
#include <SFML/Graphics.hpp>
#include <sstream>

static constexpr unsigned int SIM_W   = 720;
static constexpr unsigned int SIM_H   = 720;
static constexpr unsigned int PANEL_W = 280;
static constexpr unsigned int WIN_W   = SIM_W + PANEL_W;
static constexpr unsigned int WIN_H   = SIM_H;

// ═══════════════════════════════════════════════════════════════
//  Scene definitions  (tighter spacing = more particles, smoother look)
// ═══════════════════════════════════════════════════════════════

static std::vector<Scene> buildScenes() {
    std::vector<Scene> scenes;
    float W = static_cast<float>(SIM_W);
    float H = static_cast<float>(SIM_H);
    float sp = 5.0f;   // particle spacing

    // Scene 0 — Dam Break
    {
        Scene s;
        s.name = "Dam Break";
        s.blocks.push_back({60, 200, 320, 580, sp});
        scenes.push_back(s);
    }

    // Scene 1 — Funnel
    {
        Scene s;
        s.name = "Funnel";
        float cx = W * 0.5f;
        float topY  = H * 0.38f;
        float botY  = H * 0.58f;
        float gapW  = 40.f;
        s.obstacles.push_back({{60.f,  topY}, {cx - gapW, botY}});
        s.obstacles.push_back({{W - 60.f, topY}, {cx + gapW, botY}});
        s.obstacles.push_back({{cx - gapW, botY}, {cx - gapW, botY + 50.f}});
        s.obstacles.push_back({{cx + gapW, botY}, {cx + gapW, botY + 50.f}});
        s.blocks.push_back({140, 50, W - 140, topY - 25, sp});
        scenes.push_back(s);
    }

    // Scene 2 — Beaker
    {
        Scene s;
        s.name = "Beaker";
        float bLeft   = W * 0.28f;
        float bRight  = W * 0.72f;
        float bTop    = H * 0.40f;
        float bBottom = H * 0.85f;
        s.obstacles.push_back({{bLeft, bTop}, {bLeft, bBottom}});
        s.obstacles.push_back({{bLeft, bBottom}, {bRight, bBottom}});
        s.obstacles.push_back({{bRight, bTop}, {bRight, bBottom}});
        s.blocks.push_back({bLeft + 30, 50, bRight - 30, bTop - 50, sp});
        scenes.push_back(s);
    }

    // Scene 3 — Cascade
    {
        Scene s;
        s.name = "Cascade";
        float shelfLen = W * 0.45f;
        s.obstacles.push_back({{80.f, H * 0.22f}, {80.f + shelfLen, H * 0.28f}});
        s.obstacles.push_back({{W - 80.f, H * 0.42f}, {W - 80.f - shelfLen, H * 0.48f}});
        s.obstacles.push_back({{80.f, H * 0.62f}, {80.f + shelfLen, H * 0.68f}});
        s.blocks.push_back({60, 30, 280, H * 0.18f, sp});
        scenes.push_back(s);
    }

    return scenes;
}

// ═══════════════════════════════════════════════════════════════

int main() {
    sf::RenderWindow window(sf::VideoMode({WIN_W, WIN_H}),
                            "SPH Fluid Simulation");
    window.setFramerateLimit(60);

    // ─── Font ──────────────────────────────────────────────────
    sf::Font font;
    bool ok = font.openFromFile("/System/Library/Fonts/SFNSMono.ttf")
           || font.openFromFile("/System/Library/Fonts/Menlo.ttc")
           || font.openFromFile("/System/Library/Fonts/Monaco.ttf")
           || font.openFromFile("/System/Library/Fonts/Helvetica.ttc");
    if (!ok) return 1;

    // ─── SPH Presets (each visually & physically distinct) ────
    SPHParams waterP;
    waterP.smoothingRadius = 16.f;
    waterP.restDensity     = 1000.f;
    waterP.gasConstant     = 2000.f;
    waterP.viscosity       = 250.f;
    waterP.gravity         = 12000.f;
    waterP.particleMass    = 65.f;
    waterP.boundDamping    = -0.5f;
    waterP.dt              = 0.0016f;
    waterP.substeps        = 4;
    waterP.colorScheme     = 0;

    SPHParams honeyP = waterP;
    honeyP.viscosity    = 1200.f;
    honeyP.gasConstant  = 400.f;
    honeyP.gravity      = 8000.f;
    honeyP.particleMass = 100.f;
    honeyP.restDensity  = 1400.f;
    honeyP.dt           = 0.0014f;
    honeyP.substeps     = 4;
    honeyP.colorScheme  = 1;

    SPHParams gasP = waterP;
    gasP.viscosity       = 8.f;
    gasP.gasConstant     = 5000.f;
    gasP.gravity         = 400.f;
    gasP.restDensity     = 300.f;
    gasP.particleMass    = 18.f;
    gasP.smoothingRadius = 20.f;
    gasP.dt              = 0.002f;
    gasP.substeps        = 4;
    gasP.colorScheme     = 2;

    SPHParams lavaP = waterP;
    lavaP.viscosity       = 800.f;
    lavaP.gasConstant     = 800.f;
    lavaP.gravity         = 18000.f;
    lavaP.particleMass    = 180.f;
    lavaP.restDensity     = 2000.f;
    lavaP.smoothingRadius = 14.f;
    lavaP.dt              = 0.001f;
    lavaP.substeps        = 4;
    lavaP.colorScheme     = 3;

    // ─── Scenes ──────────────────────────────────────────────
    std::vector<Scene> scenes = buildScenes();
    int activeScene = 0;

    // ─── Solver ──────────────────────────────────────────────
    SPHSolver solver(static_cast<float>(SIM_W), static_cast<float>(SIM_H), waterP);
    solver.loadScene(scenes[activeScene]);

    // ─── Renderer & GUI ─────────────────────────────────────
    FluidRenderer renderer(SIM_W, SIM_H);

    GUI gui(static_cast<float>(SIM_W), static_cast<float>(PANEL_W), font);
    gui.setScenes(scenes);
    gui.addPreset("Water", waterP);
    gui.addPreset("Honey", honeyP);
    gui.addPreset("Gas",   gasP);
    gui.addPreset("Lava",  lavaP);

    gui.addSlider("Gravity",          &solver.params().gravity,         0.f,    30000.f);
    gui.addSlider("Viscosity",        &solver.params().viscosity,       5.f,    1500.f);
    gui.addSlider("Gas Constant",     &solver.params().gasConstant,     200.f,  8000.f);
    gui.addSlider("Rest Density",     &solver.params().restDensity,     200.f,  3000.f);
    gui.addSlider("Particle Mass",    &solver.params().particleMass,    10.f,   200.f);
    gui.addSlider("Smoothing Radius", &solver.params().smoothingRadius, 8.f,    32.f);
    gui.addSlider("Damping",          &solver.params().boundDamping,    -0.9f,  0.0f);
    gui.addSlider("Timestep",         &solver.params().dt,              0.0002f, 0.002f);

    bool paused = false;
    sf::Clock fpsClock;
    float fps = 60.f;

    // ─── Main loop ───────────────────────────────────────────
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();

            int sceneClicked = gui.handleEvent(*event, solver);
            if (sceneClicked >= 0 && sceneClicked < static_cast<int>(scenes.size())) {
                activeScene = sceneClicked;
                solver.loadScene(scenes[activeScene]);
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) window.close();
                if (key->code == sf::Keyboard::Key::Space)  paused = !paused;
                if (key->code == sf::Keyboard::Key::R)
                    solver.loadScene(scenes[activeScene]);
                if (key->code == sf::Keyboard::Key::D)
                    solver.addBlock(460, 60, 680, 380, 5.0f);
            }

            if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
                float mx = static_cast<float>(click->position.x);
                float my = static_cast<float>(click->position.y);
                if (!gui.isOverPanel(mx))
                    solver.addBlock(mx - 20, my - 20, mx + 20, my + 20, 5.0f);
            }
        }

        solver.recalcKernels();
        if (!paused) solver.update();

        float dt = fpsClock.restart().asSeconds();
        fps = fps * 0.93f + (1.f / std::max(dt, 0.001f)) * 0.07f;

        window.clear(sf::Color(8, 10, 18));
        renderer.draw(window, solver);
        gui.draw(window, solver, fps, paused, activeScene);
        window.display();
    }
    return 0;
}
