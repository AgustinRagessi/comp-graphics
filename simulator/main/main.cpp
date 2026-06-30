#include "../opengl/window.h"
#include "../map/skybox.h"
#include "../map/terrain.h"
#include "../airplane/airplane.h"
#include "../camera/camera.h"
#include "../helper/coordinateTranslate.h"

#include "../waypoint/waypoint.h"
#include "../hud/waypointTracker.h"
#include "../hud/hud_data.h"
#include "../hud/hud_manager.h"
#include "../hud/horizon.h"
#include "../hud/airspeed.h"
#include "../hud/altimeter.h"   
#include "../hud/compass.h"
#include "../hud/variometer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <dlfdm/fdmsolver.h>

// Global Configuration
static const int TERRAIN_VIEW_DISTANCE = 4;
static const float TERRAIN_CHUNK_SIZE = 200.0f;
static const int TERRAIN_RESOLUTION = 32;
static const float TERRAIN_HEIGHT = 100.0f;

const char *simpleDepthVertexShader = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";
const char *simpleDepthFragmentShader = R"(
#version 460 core
void main() {
    // OpenGL handles depth implicitly
}
)";

GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

static dlfdm::AircraftParameters buildAircraftParameters()
{
    dlfdm::AircraftParameters p{};
    p.mass = 1815.0f;
    p.Ixx = 1084.6f;
    p.Iyy = 6507.9f;
    p.Izz = 7050.2f;
    p.Ixz = 271.16f;
    p.wingArea = 12.63f;
    p.wingChord = 1.64f;
    p.wingSpan = 8.01f;
    p.maxThrust = 11120.0f;
    p.CL0 = 0.15f;
    p.CLa = 5.5f;
    p.CL_delta_e = 0.38f;
    p.CD0 = 0.0205f;
    p.CDa = 0.12f;
    p.Cm0 = -0.08f;
    p.Cma = -0.24f;
    p.Cm_q = -15.7f;
    p.CY_beta = -1.0f;
    p.CY_r = 0.61f;
    p.CY_delta_r = 0.028f;
    p.Cl_beta = -0.11f;
    p.Cl_p = -0.39f;
    p.Cl_r = 0.28f;
    p.Cn_beta = 0.17f;
    p.Cn_p = 0.09f;
    p.Cn_r = -0.26f;
    p.Cm_delta_e = -0.88f;
    p.Cl_delta_a = 0.10f;
    p.Cn_delta_r = -0.12f;
    p.min_elevator = glm::radians(-15.0f);
    p.max_elevator = glm::radians(20.0f);
    p.min_aileron = glm::radians(-20.0f);
    p.max_aileron = glm::radians(20.0f);
    p.max_rudder = glm::radians(20.0f);
    return p;
}

dlfdm::AircraftParameters g_aircraftParams = buildAircraftParameters();
Map::Terrain *g_terrain = nullptr;
dlfdm::ControlInputs aircraftControls{};
//float maxBankAngleDegrees = 80.0f;

glm::mat4 buildModelMatrix() { return glm::mat4(1.0f); }
glm::mat4 buildProjectionMatrix(float width, float height)
{
    // Prevent division by zero if the window is minimized
    if (height == 0.0f) height = 1.0f;
    return glm::perspective(glm::radians(45.0f), width / height, 0.1f, 1000.0f);
}

bool vectorHasNaN(const glm::vec3 &v)
{
    return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z) ||
           std::isinf(v.x) || std::isinf(v.y) || std::isinf(v.z);
}

bool stateHasNaN(const dlfdm::AircraftState &state)
{
    return vectorHasNaN(state.intertial_position) || vectorHasNaN(state.boby_velocity) ||
           vectorHasNaN(state.body_omega) || std::isnan(state.phi) || std::isnan(state.theta) ||
           std::isnan(state.psi) || std::isinf(state.phi) || std::isinf(state.theta) || std::isinf(state.psi);
}

void processInput(GLFWwindow *window, float deltaTime, const dlfdm::FDMSolver &fdm, Camera &camera)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    static bool cKeyWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        if (!cKeyWasPressed)
        {
            camera.toggleMode();
            cKeyWasPressed = true;
        }
    }
    else
    {
        cKeyWasPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        aircraftControls.throttle += 0.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        aircraftControls.throttle -= 0.5f * deltaTime;
    aircraftControls.throttle = glm::clamp(aircraftControls.throttle, 0.0f, 1.0f);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        aircraftControls.elevator -= 1.5f * deltaTime;
    else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        aircraftControls.elevator += 1.5f * deltaTime;
    else
        aircraftControls.elevator = glm::mix(aircraftControls.elevator, 0.0f, deltaTime * 3.0f);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        aircraftControls.aileron -= 1.5f * deltaTime;
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        aircraftControls.aileron += 1.5f * deltaTime;
    else
        aircraftControls.aileron = glm::mix(aircraftControls.aileron, 0.0f, deltaTime * 3.0f);

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        aircraftControls.rudder -= 1.0f * deltaTime;
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        aircraftControls.rudder += 1.0f * deltaTime;
    else
        aircraftControls.rudder = glm::mix(aircraftControls.rudder, 0.0f, deltaTime * 3.0f);

    //const dlfdm::AircraftState &state = fdm.getState();
    //float maxBankRads = glm::radians(maxBankAngleDegrees);
    //if (state.phi > maxBankRads)
    //{
    //    if (aircraftControls.aileron > 0.0f)
    //        aircraftControls.aileron = 0.0f;
    //    aircraftControls.aileron -= (state.phi - maxBankRads) * 2.0f;
    //}
    //else if (state.phi < -maxBankRads)
    //{
    //    if (aircraftControls.aileron < 0.0f)
    //        aircraftControls.aileron = 0.0f;
    //    aircraftControls.aileron += (-maxBankRads - state.phi) * 2.0f;
    //}

    aircraftControls.elevator = glm::clamp(aircraftControls.elevator, -0.35f, 0.35f);
    aircraftControls.aileron = glm::clamp(aircraftControls.aileron, -0.35f, 0.35f);
    aircraftControls.rudder = glm::clamp(aircraftControls.rudder, -0.35f, 0.35f);
}

int main()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    OpenGL::Window window(1280, 720, "Flight Simulator");
    if (!window.initialize())
        return -1;

    Map::Terrain terrain;
    Map::TerrainConfig terrainConfig;
    terrainConfig.viewDistance = TERRAIN_VIEW_DISTANCE;
    terrainConfig.chunkSize = TERRAIN_CHUNK_SIZE;
    terrainConfig.chunkResolution = TERRAIN_RESOLUTION;
    terrainConfig.heightScale = TERRAIN_HEIGHT;

    if (!terrain.initialize(terrainConfig, "textures/ground/grass_04.png"))
        return -1;
    g_terrain = &terrain;

    Map::Skybox skybox;
    std::vector<std::string> skyboxFaces = {
        "textures/sky/right.bmp", "textures/sky/left.bmp", "textures/sky/top.bmp",
        "textures/sky/bottom.bmp", "textures/sky/front.bmp", "textures/sky/back.bmp"};
    if (!skybox.initialize(skyboxFaces))
        return -1;

    dlfdm::FDMSolver aircraftFdm(g_aircraftParams);
    dlfdm::AircraftState aircraftStartState{};
    const glm::vec3 aircraftSpawnWorld = glm::vec3(0.0f, 150.0f, 0.0f);
    aircraftStartState.intertial_position = Helper::worldToNedPosition(aircraftSpawnWorld);
    aircraftStartState.boby_velocity = glm::vec3(60.0f, 0.0f, 0.0f);
    aircraftStartState.body_omega = glm::vec3(0.0f);
    aircraftStartState.phi = 0.0f;
    aircraftStartState.theta = 0.0f;
    aircraftStartState.psi = 0.0f;
    aircraftFdm.setState(aircraftStartState);

    aircraftControls.throttle = 0.65f;
    aircraftControls.elevator = 0.0f;
    aircraftControls.aileron = 0.0f;
    aircraftControls.rudder = 0.0f;

    Airplane::AirplaneModel airplane;
    if (!airplane.initialize("textures/airplane/s211-v3.obj"))
        return -1;
    Camera camera;

    WaypointSystem waypointSys;
    if (!waypointSys.initialize())
        return -1;

    // UPDATED: Pass the terrain into the initial spawn!
    waypointSys.spawnNext(aircraftStartState.intertial_position, terrain);

    HudManager hudManager;
    if (!hudManager.initialize())
        return -1;

    ArtificialHorizon hudHorizon;
    AirspeedIndicator hudAirspeed;
    Altimeter hudAltimeter;
    Compass hudCompass;
    Variometer hudVariometer;
    WaypointTracker hudWpTracker;
    
    // Time Management Variables
    float deltaTime = 0.0f;
    float lastFrame = glfwGetTime();

    // Fixed Timestep for Physics: 120Hz (0.00833s) guarantees deterministic flight dynamics 
    // regardless of the visual framerate (FPS).
    const float physicsDt = 1.0f / 120.0f;
    float physicsAccumulator = 0.0f;
    bool physicsBroken = false;
    glm::vec3 safeCameraPos = aircraftSpawnWorld + glm::vec3(50.0f, 50.0f, 50.0f);
    
    // ============================================
    // SHADOW MAPPING FRAMEBUFFER SETUP
    // ============================================
    // We create a custom Framebuffer Object (FBO) to render the scene from the sun's 
    // perspective into a depth texture, rather than to the screen.
    const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "\n[CRITICAL] Shadow Framebuffer rejected by GPU driver!\n";
    }

    // Shadow map clamping to prevent artifacts outside the light's projection bounds
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE); // Explicitly tell OpenGL we aren't drawing colors
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2. Setup Depth Shader Program
    GLuint depthVertex = compileShader(GL_VERTEX_SHADER, simpleDepthVertexShader);
    GLuint depthFragment = compileShader(GL_FRAGMENT_SHADER, simpleDepthFragmentShader);
    GLuint depthShaderProgram = glCreateProgram();
    glAttachShader(depthShaderProgram, depthVertex);
    glAttachShader(depthShaderProgram, depthFragment);
    glLinkProgram(depthShaderProgram);

    // ============================================
    // MAIN GAME LOOP
    // ============================================
    while (!window.shouldClose())
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Get the current window size in case of window resizing.
        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window.getHandle(), &currentWidth, &currentHeight);
        if (currentHeight == 0) currentHeight = 1; // Prevent crash when minimized


        // Cap deltaTime to prevent the spiral of death if the game hangs
        if (deltaTime > 0.1f)
            deltaTime = 0.1f;
        physicsAccumulator += deltaTime;

        // FIXED-TIMESTEP PHYSICS LOOP
        // Consume accumulated time in fixed chunks. If visual FPS drops, physics calculates 
        // multiple times to catch up, keeping the simulation perfectly synchronized with real time.
        while (physicsAccumulator >= physicsDt && !physicsBroken)
        {
            processInput(window.getHandle(), physicsDt, aircraftFdm, camera);
            aircraftFdm.setTimeStep(physicsDt);
            aircraftFdm.update(aircraftControls);
            dlfdm::AircraftState stateAfterUpdate = aircraftFdm.getState();

            // NaN check ensures a physics explosion doesn't crash the graphics pipeline
            if (stateHasNaN(stateAfterUpdate))
            {
                std::cerr << "\n[CRITICAL ERROR] NaN Detected! Physics Halted." << std::endl;
                physicsBroken = true;
                break;
            }

            // Terrain Collision check
            if (waypointSys.checkCollision(stateAfterUpdate.intertial_position, terrain))
            {
                std::cout << "[WAYPOINT] Target hit!" << std::endl;
            }

            glm::vec3 worldPos = Helper::nedToWorldPosition(stateAfterUpdate.intertial_position);

            if (terrain.checkCollision(worldPos))
            {
                std::cout << "\n[CRASH] The aircraft hit the terrain at altitude " << worldPos.y << "m! Resetting...\n";
                aircraftFdm.setState(aircraftStartState);
                aircraftControls.throttle = 0.65f;
                aircraftControls.elevator = 0.0f;
                aircraftControls.aileron = 0.0f;
                aircraftControls.rudder = 0.0f;

                // UPDATED: Pass the terrain into the crash respawn!
                waypointSys.spawnNext(aircraftStartState.intertial_position, terrain);
            }

            physicsAccumulator -= physicsDt;
        }

        if (!physicsBroken)
            camera.update(aircraftFdm.getState());
        else
            camera.setSafeMode(safeCameraPos);

        glm::vec3 currentCamPos = camera.getPosition();
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = buildProjectionMatrix((float)currentWidth, (float)currentHeight);
        glm::mat4 model = buildModelMatrix();
        glm::mat4 mvp = projection * view * model;

        // Light calculation
        glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));

        // Light projection
        float orthoSize = 400.0f; // Defines the volumetric box size of the directional light
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 1500.0f);
        
        // Pin the light relative to the camera so the high-resolution shadow map 
        // constantly follows the player, saving GPU memory.        
        glm::vec3 lightPos = currentCamPos - (sunDir * 700.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, currentCamPos, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // ============================================
        // RENDER PASS 1: SHADOW MAP 
        // ============================================
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Cull front faces during shadow mapping to eliminate "peter-panning" 
        // (shadows detaching from the objects casting them).
        glCullFace(GL_FRONT);

        terrain.update(currentCamPos); // Stream new chunks before capturing shadows
        terrain.renderDepth(depthShaderProgram, lightSpaceMatrix);

        if (!physicsBroken && camera.getMode() != CameraMode::FIRST_PERSON)
        {
            airplane.renderDepth(depthShaderProgram, aircraftFdm, lightSpaceMatrix);
        }

        glCullFace(GL_BACK); // Restore normal back-face culling for the visual pass
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ============================================
        // RENDER PASS 2: VISUAL SCENE
        // ============================================
        glViewport(0, 0, currentWidth, currentHeight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render scene objects, passing in the generated depthMap texture for shadow lookups
        terrain.render(mvp, model, lightSpaceMatrix, depthMap, currentCamPos, sunDir);

        if (!physicsBroken && camera.getMode() != CameraMode::FIRST_PERSON)
        {
            airplane.render(aircraftFdm, view, projection, lightSpaceMatrix, depthMap, sunDir, currentCamPos);
        }

        // Skybox and Waypoints
        waypointSys.render(view, projection);
        skybox.render(view, projection);

        if (!physicsBroken && camera.getMode() == CameraMode::FIRST_PERSON)
        {
            const dlfdm::AircraftState &state = aircraftFdm.getState();
            hud::FlightData flightData;

            flightData.pitch = state.theta * (180.0f / 3.14159f);
            flightData.roll = state.phi * (180.0f / 3.14159f);
            flightData.heading = state.psi * (180.0f / 3.14159f);
            if (flightData.heading < 0.0f)
                flightData.heading += 360.0f;
            flightData.altitude = (-state.intertial_position.z) * 3.28084f;
            flightData.speed = glm::length(state.boby_velocity) * 1.94384f;
            float zDot = aircraftFdm.get_state_dot().ned_position_dot.z;
            flightData.vertical_speed = -zDot * 196.85f;

            flightData.aircraft_x = state.intertial_position.x;
            flightData.aircraft_y = state.intertial_position.y;
            flightData.waypoint.latitude = waypointSys.getPositionNed().x;
            flightData.waypoint.longitud = waypointSys.getPositionNed().y;
            flightData.waypoint.altitude = waypointSys.getPositionNed().z;

            glDisable(GL_DEPTH_TEST);
            hudManager.beginRender(currentWidth, currentHeight);

            hudHorizon.render(flightData, hudManager, currentWidth, currentHeight);
            hudAirspeed.render(flightData, hudManager, currentWidth, currentHeight);
            hudAltimeter.render(flightData, hudManager, currentWidth, currentHeight);
            hudCompass.render(flightData, hudManager, currentWidth, currentHeight);
            hudVariometer.render(flightData, hudManager, currentWidth, currentHeight);
            hudWpTracker.render(flightData, hudManager, currentWidth, currentHeight);

            hudManager.endRender();
            glEnable(GL_DEPTH_TEST);
        }

        window.swapBuffers();
        window.pollEvents();
    }
    return 0;
}