
#ifndef A3_ENGINE_H
#define A3_ENGINE_H

#include <CSCI441/OpenGLEngine.hpp>
#include <CSCI441/ShaderProgram.hpp>
#include <CSCI441/objects.hpp>

#include "ArcballCamera.h"
#include "Vehicle.h"

#include <vector>

// Forward Declarations of Callback Functions
void a3_engine_keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods );
void a3_engine_cursor_callback(GLFWwindow *window, double x, double y );
void a3_engine_mouse_button_callback(GLFWwindow *window, int button, int action, int mods );

class A3Engine final : public CSCI441::OpenGLEngine {
public:
    A3Engine();
    ~A3Engine() final;

    void run() final;

    // Event Handlers
    void handleKeyEvent(GLint key, GLint action, GLint mods);
    void handleMouseButtonEvent(GLint button, GLint action, GLint mods); // Updated to include mods
    void handleCursorPositionEvent(glm::vec2 currMousePosition);
    static bool checkCollision(const glm::vec3& pos1, float radius1,
                               const glm::vec3& pos2, float radius2);
    bool isMovementValid(const glm::vec3& newPosition) const;

    static constexpr GLfloat MOUSE_UNINITIALIZED = -9999.0f;

private:
    // Engine Setup and Cleanup
    void mSetupGLFW() final;
    void mSetupOpenGL() final;
    void mSetupShaders() final;
    void mSetupBuffers() final;
    void mSetupScene() final;

    void mCleanupBuffers() final;
    void mCleanupShaders() final;

    // Rendering
    void _renderScene(glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _updateScene();

    // Input Tracking
    static constexpr GLuint NUM_KEYS = GLFW_KEY_LAST;
    GLboolean _keys[NUM_KEYS];
    glm::vec2 _mousePosition;
    GLint _leftMouseButtonState;

    // Camera and Vehicle
    ArcballCamera _arcballCam;
    Vehicle* _pVehicle;

    // Animation State
    float _animationTime;

    // Ground
    static constexpr GLfloat WORLD_SIZE = 55.0f;
    GLuint _groundVAO;
    GLsizei _numGroundPoints;

    // Buildings
    struct BuildingData {
        glm::mat4 modelMatrix;
        glm::vec3 color;
        glm::vec3 position;
        float boundingRadius;
    };
    std::vector<BuildingData> _buildings;

    // Shaders
    CSCI441::ShaderProgram* _lightingShaderProgram = nullptr;
    struct LightingShaderUniformLocations {
        GLint mvpMatrix;
        GLint normalMatrix;
        GLint lightDirection;
        GLint lightColor;
        GLint materialAmbient;
        GLint materialDiffuse;
        GLint materialSpecular;
        GLint materialShininess;
    } _lightingShaderUniformLocations;

    struct LightingShaderAttributeLocations {
        GLint vPos;
        GLint vNormal;
    } _lightingShaderAttributeLocations;

    // Ground and Grid
    GLuint _gridVAO;
    GLsizei _numGridVertices;

    // Helper Functions
    void _createGroundBuffers();
    void _generateEnvironment();
    void _computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;

    // Zoom Handling
    bool _shiftPressed = false;    // Tracks if Shift is pressed
    bool _zooming = false;         // Tracks if currently zooming
    float _zoomSensitivity = 0.05f; // Adjust as needed
};

#endif // A3_ENGINE_H
