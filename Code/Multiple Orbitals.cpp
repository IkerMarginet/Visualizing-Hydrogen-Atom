#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <GL/glu.h>
#include <cmath>
#include <vector>
#include <random>
#include <iostream>

// =======================
// Constants and Parameters
// =======================

constexpr float PI = 3.14159265359f;
constexpr float BOHR_RADIUS = 1.0f;
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr int NUM_POINTS = 10000;
constexpr float ROTATION_SPEED = 0.01f;
constexpr float VIBRATION_FREQ = 0.1f;

// =======================
// Orbital Definition
// =======================

struct Orbital {
    int n, l, m;
    float scale;
    std::string name;
    sf::Vector3f color; // RGB color
};

// =======================
// Quantum Functions
// =======================

// Real spherical harmonics for s and p orbitals
float real_spherical_harmonic(const Orbital& orbital, float theta, float phi) {
    int l = orbital.l;
    int m = orbital.m;

    if (l == 0 && m == 0) // 1s
        return 0.5f * std::sqrt(1.0f / PI);

    if (l == 1 && m == 0) // 2pz
        return std::sqrt(3.0f / (4.0f * PI)) * std::cos(theta);

    if (l == 1 && m == 1) // 2px
        return -std::sqrt(3.0f / (4.0f * PI)) * std::sin(theta) * std::cos(phi);

    if (l == 1 && m == -1) // 2py
        return -std::sqrt(3.0f / (4.0f * PI)) * std::sin(theta) * std::sin(phi);

    return 0.0f; // Unimplemented
}

float radial_function(int n, float r) {
    float a0 = BOHR_RADIUS;

    if (n == 1) // 1s
        return 2.0f * std::exp(-r / a0) / std::pow(a0, 1.5f);

    if (n == 2) // 2s or 2p
        return (1.0f / (2.0f * std::sqrt(2.0f))) * (1.0f - r / (2.0f * a0)) * std::exp(-r / (2.0f * a0)) / std::pow(a0, 1.5f);

    return 0.0f; // Unimplemented
}

float probability_density(const Orbital& orbital, float r, float theta, float phi, float time) {
    float R = radial_function(orbital.n, r);
    float Y = real_spherical_harmonic(orbital, theta, phi);
    float psi = R * Y;
    float vibration = 1.0f + 0.1f * std::sin(VIBRATION_FREQ * time);
    return psi * psi * vibration;
}

// =======================
// Orbital Point Generator
// =======================

std::vector<sf::Vector3f> generate_orbital_points(const Orbital& orbital, float time) {
    std::vector<sf::Vector3f> points;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> r_dist(0.0f, 8.0f * BOHR_RADIUS);
    std::uniform_real_distribution<float> theta_dist(0.0f, PI);
    std::uniform_real_distribution<float> phi_dist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);

    float max_prob = 1.0f; // Conservative or precomputed

    while (points.size() < NUM_POINTS) {
        float r = r_dist(gen);
        float theta = theta_dist(gen);
        float phi = phi_dist(gen);
        float prob = probability_density(orbital, r, theta, phi, time);

        if (prob_dist(gen) < prob / max_prob) {
            float x = r * std::sin(theta) * std::cos(phi);
            float y = r * std::sin(theta) * std::sin(phi);
            float z = r * std::cos(theta);
            points.emplace_back(x, y, z);
        }
    }

    return points;
}

// =======================
// Main
// =======================

int main() {
    // SFML + OpenGL setup
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 3;
    settings.minorVersion = 3;

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Hydrogen Orbital Viewer", sf::Style::Default, settings);
    window.setFramerateLimit(60);
    window.setActive(true);

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(2.0f);

    // Define orbitals
    std::vector<Orbital> orbitals = {
        {1, 0, 0, 2.0f, "1s", sf::Vector3f(1.0f, 0.0f, 0.0f)},       // 1
        {2, 1, 1, 2.0f, "2px", sf::Vector3f(0.0f, 1.0f, 0.0f)},      // 2
        {2, 1, -1, 2.0f, "2py", sf::Vector3f(0.0f, 0.5f, 1.0f)},     // 3
        {2, 1, 0, 2.0f, "2pz", sf::Vector3f(1.0f, 1.0f, 0.0f)}       // 4
    };

    int current_orbital = 0;
    std::vector<sf::Vector3f> points;

    float camera_distance = 10.0f;
    float angle = 0.0f;
    sf::Clock clock;
    float last_generation_time = -100.0f;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num4) {
                    int index = event.key.code - sf::Keyboard::Num1;
                    if (index < orbitals.size()) {
                        current_orbital = index;
                        std::cout << "Switched to orbital: " << orbitals[current_orbital].name << "\n";
                        last_generation_time = -100.0f;
                    }
                }
            }
        }

        float time = clock.getElapsedTime().asSeconds();
        angle += ROTATION_SPEED;

        // Regenerate points only every 0.5s
        if (time - last_generation_time > 0.5f) {
            points = generate_orbital_points(orbitals[current_orbital], time);
            last_generation_time = time;
        }

        window.clear();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(camera_distance * std::sin(angle), 0.0f, camera_distance * std::cos(angle),
                  0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f);

        // Render points      
        glBegin(GL_POINTS);
        for (const auto& p : points) {
            sf::Vector3f c = orbitals[current_orbital].color;
            glColor4f(c.x, c.y, c.z, 0.5f);
            glVertex3f(p.x * orbitals[current_orbital].scale, p.y * orbitals[current_orbital].scale, p.z * orbitals[current_orbital].scale);
        }
        glEnd();

        window.display();
    }

    return 0;
}
