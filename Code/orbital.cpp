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

// Simplified real-valued spherical harmonics
float spherical_harmonic(int l, int m, float theta, float phi) {
    if (l == 0 && m == 0)
        return 0.5f * std::sqrt(1.0f / PI);

    if (l == 1 && m == 0)
        return std::sqrt(3.0f / (4.0f * PI)) * std::cos(theta);

    if (l == 1 && m == 1)
        return -std::sqrt(3.0f / (8.0f * PI)) * std::sin(theta) * std::cos(phi);

    if (l == 1 && m == -1)
        return std::sqrt(3.0f / (8.0f * PI)) * std::sin(theta) * std::sin(phi);

    return 0.0f; // Unimplemented
}

float radial_function(int n, float r) {
    float a0 = BOHR_RADIUS;

    if (n == 1)
        return 2.0f * std::exp(-r / a0) / std::pow(a0, 1.5f);

    if (n == 2)
        return (1.0f / (2.0f * std::sqrt(2.0f))) * (1.0f - r / (2.0f * a0)) * std::exp(-r / (2.0f * a0)) / std::pow(a0, 1.5f);

    return 0.0f; // Unimplemented
}

float probability_density(int n, int l, int m, float r, float theta, float phi, float time) {
    float R = radial_function(n, r);
    float Y = spherical_harmonic(l, m, theta, phi);
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
    std::uniform_real_distribution<float> r_dist(0.0f, 5.0f * BOHR_RADIUS);
    std::uniform_real_distribution<float> theta_dist(0.0f, PI);
    std::uniform_real_distribution<float> phi_dist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);

    float max_prob = 1.0f; // Conservative or precomputed

    while (points.size() < NUM_POINTS) {
        float r = r_dist(gen);
        float theta = theta_dist(gen);
        float phi = phi_dist(gen);
        float prob = probability_density(orbital.n, orbital.l, orbital.m, r, theta, phi, time);

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

    // Define orbital (1s by default)
    Orbital orbital = {1, 0, 0, 2.0f, "1s", sf::Vector3f(1.0f, 0.0f, 0.0f)}; // red
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
        }

        float time = clock.getElapsedTime().asSeconds();
        angle += ROTATION_SPEED;

        // Regenerate points only every 0.5s
        if (time - last_generation_time > 0.5f) {
            points = generate_orbital_points(orbital, time);
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
            glColor4f(orbital.color.x, orbital.color.y, orbital.color.z, 0.5f);
            glVertex3f(p.x * orbital.scale, p.y * orbital.scale, p.z * orbital.scale);
        }
        glEnd();

        window.display();
    }

    return 0;
}
