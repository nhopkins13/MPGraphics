
#include "MPEngine.h"

#include <CSCI441/objects.hpp>
#include <ctime>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

GLfloat getRand() {
    return static_cast<GLfloat>(rand()) / static_cast<GLfloat>(RAND_MAX);
}

MPEngine::MPEngine()
         : CSCI441::OpenGLEngine(4, 1,
                                 1280, 720, // Increased window size for better view
                                 "A3: Journey to the Cross-roads"),
           _pVehicle(nullptr),
           _animationTime(0.0f),
           _groundVAO(0),
           _numGroundPoints(0),
           _gridVAO(0),
           _numGridVertices(0)
{
    for(auto& key : _keys) key = GL_FALSE;

    _mousePosition = glm::vec2(MOUSE_UNINITIALIZED, MOUSE_UNINITIALIZED );
    _leftMouseButtonState = GLFW_RELEASE;
}

MPEngine::~MPEngine() {
    delete _lightingShaderProgram;
    delete _pVehicle;
}

bool MPEngine::checkCollision(const glm::vec3& pos1, float radius1,
                               const glm::vec3& pos2, float radius2) {
    float distanceSquared = glm::dot(pos1 - pos2, pos1 - pos2);
    return distanceSquared <= ((radius1 + radius2) * (radius1 + radius2));
}

bool MPEngine::isMovementValid(const glm::vec3& newPosition) const {
    float vehicleRadius = _pVehicle->getBoundingRadius();

    for(const BuildingData& building : _buildings) {
        if(checkCollision(newPosition, vehicleRadius, building.position, building.boundingRadius)) {
            return false;
        }
    }
    return true;
}

void MPEngine::handleKeyEvent(GLint key, GLint action, GLint mods) {
    if(key >= 0 && key < NUM_KEYS)
        _keys[key] = ((action == GLFW_PRESS) || (action == GLFW_REPEAT));

    if(action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch( key ) {
            //quit!
            case GLFW_KEY_Q:
            case GLFW_KEY_ESCAPE:
                setWindowShouldClose();
                break;

            //zoom In with Space
            case GLFW_KEY_SPACE:
                if(mods & GLFW_MOD_SHIFT) {
                    //shift and space = Zoom Out
                    _arcballCam.zoomOut();
                }
                else {
                    // Space = Zoom In
                    _arcballCam.zoomIn();
                }
                break;

            default: break;
        }
    }
}

void MPEngine::handleMouseButtonEvent(GLint button, GLint action, GLint mods) {
    // if the event is for the left mouse button
    if( button == GLFW_MOUSE_BUTTON_LEFT ) {
        // update the left mouse button's state
        _leftMouseButtonState = action;

        if(action == GLFW_PRESS) {
            _shiftPressed = (mods & GLFW_MOD_SHIFT) != 0;

            if(_shiftPressed) {
                _zooming = true;
            }
        }
        else if(action == GLFW_RELEASE) {
            _zooming = false;
        }
    }
}

void MPEngine::handleCursorPositionEvent(glm::vec2 currMousePosition) {
    // if mouse hasn't moved in the window, prevent camera from flipping out
    if(_mousePosition.x == MOUSE_UNINITIALIZED) {
        _mousePosition = currMousePosition;
        return;
    }

    // Calculate mouse movement deltas
    float deltaX = currMousePosition.x - _mousePosition.x;
    float deltaY = currMousePosition.y - _mousePosition.y;

    // Update the mouse position
    _mousePosition = currMousePosition;

    if(_leftMouseButtonState == GLFW_PRESS) {
        if(_zooming) {
            // Perform zooming based on vertical mouse movement
            // Dragging up (deltaY < 0) should zoom in (decrease radius)
            // Dragging down (deltaY > 0) should zoom out (increase radius)
            float deltaZoom = -deltaY * _zoomSensitivity;
            _arcballCam.zoom(deltaZoom);
        }
        else {
            // Perform rotation based on mouse movement
            float deltaTheta = -deltaX * 0.005f;
            float deltaPhi = -deltaY * 0.005f;

            _arcballCam.rotate(deltaTheta, deltaPhi);
        }
    }
}

//*************************************************************************************
//
//engine Setup

void MPEngine::mSetupGLFW() {
    CSCI441::OpenGLEngine::mSetupGLFW();

    // set our callbacks
    glfwSetKeyCallback(mpWindow, a3_engine_keyboard_callback);
    glfwSetMouseButtonCallback(mpWindow, a3_engine_mouse_button_callback);
    glfwSetCursorPosCallback(mpWindow, a3_engine_cursor_callback);

    // Set the user pointer to this instance
    glfwSetWindowUserPointer(mpWindow, this);
}

void MPEngine::mSetupOpenGL() {
    glEnable( GL_DEPTH_TEST );                        // enable depth testing
    glDepthFunc( GL_LESS );                           // use less than depth test

    glEnable(GL_BLEND);                                // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// use one minus blending equation

    glClearColor( 0.5f, 0.7f, 1.0f, 1.0f );
}

void MPEngine::mSetupShaders() {
    _lightingShaderProgram = new CSCI441::ShaderProgram("shaders/A3.v.glsl", "shaders/A3.f.glsl" );
    _lightingShaderUniformLocations.mvpMatrix      = _lightingShaderProgram->getUniformLocation("mvpMatrix");
    _lightingShaderUniformLocations.normalMatrix   = _lightingShaderProgram->getUniformLocation("normalMatrix");
    _lightingShaderUniformLocations.lightDirection = _lightingShaderProgram->getUniformLocation("lightDirection");
    _lightingShaderUniformLocations.lightColor     = _lightingShaderProgram->getUniformLocation("lightColor");
    _lightingShaderUniformLocations.materialAmbient   = _lightingShaderProgram->getUniformLocation("material.ambient");
    _lightingShaderUniformLocations.materialDiffuse   = _lightingShaderProgram->getUniformLocation("material.diffuse");
    _lightingShaderUniformLocations.materialSpecular  = _lightingShaderProgram->getUniformLocation("material.specular");
    _lightingShaderUniformLocations.materialShininess = _lightingShaderProgram->getUniformLocation("material.shininess");

    _lightingShaderAttributeLocations.vPos    = _lightingShaderProgram->getAttributeLocation("vPos");
    _lightingShaderAttributeLocations.vNormal = _lightingShaderProgram->getAttributeLocation("vNormal");
}

void MPEngine::mSetupBuffers() {
    //connect our 3D Object Library to our shader
    CSCI441::setVertexAttributeLocations( _lightingShaderAttributeLocations.vPos, _lightingShaderAttributeLocations.vNormal);

    _createGroundBuffers();

    _generateEnvironment();
}

void MPEngine::_createGroundBuffers() {
    struct Vertex {
        glm::vec3 position;
        float normalX;
        float normalY;
        float normalZ;
    };

    Vertex groundQuad[4] = {
        { {-1.0f, 0.0f, -1.0f}, 0.0f, 1.0f, 0.0f },  // Bottom-left
        { { 1.0f, 0.0f, -1.0f}, 0.0f, 1.0f, 0.0f },  // Bottom-right
        { {-1.0f, 0.0f,  1.0f}, 0.0f, 1.0f, 0.0f },  // Top-left
        { { 1.0f, 0.0f,  1.0f}, 0.0f, 1.0f, 0.0f }   // Top-right
    };

    GLushort indices[4] = {0, 1, 2, 3};

    _numGroundPoints = 4;

    glGenVertexArrays(1, &_groundVAO);
    glBindVertexArray(_groundVAO);

    GLuint vbods[2]; // 0 - VBO, 1 - IBO
    glGenBuffers(2, vbods);
    glBindBuffer(GL_ARRAY_BUFFER, vbods[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundQuad), groundQuad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(_lightingShaderAttributeLocations.vPos);
    glVertexAttribPointer(_lightingShaderAttributeLocations.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)nullptr);

    glEnableVertexAttribArray(_lightingShaderAttributeLocations.vNormal);
    glVertexAttribPointer(_lightingShaderAttributeLocations.vNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normalX)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbods[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Create Grid Lines
    std::vector<glm::vec3> gridVertices;
    int gridLines = 20;
    float spacing = WORLD_SIZE / gridLines;

    for(int i = -gridLines / 2; i <= gridLines / 2; ++i) {
        // Lines parallel to X-axis
        gridVertices.emplace_back(glm::vec3(-WORLD_SIZE , 0.0f, i * spacing));
        gridVertices.emplace_back(glm::vec3(WORLD_SIZE , 0.0f, i * spacing));

        // Lines parallel to Z-axis
        gridVertices.emplace_back(glm::vec3(i * spacing, 0.0f, -WORLD_SIZE ));
        gridVertices.emplace_back(glm::vec3(i * spacing, 0.0f, WORLD_SIZE ));
    }

    glGenVertexArrays(1, &_gridVAO);
    glBindVertexArray(_gridVAO);

    GLuint gridVBO;
    glGenBuffers(1, &gridVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(glm::vec3), gridVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(_lightingShaderAttributeLocations.vPos);
    glVertexAttribPointer(_lightingShaderAttributeLocations.vPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Disable vNormal for grid lines as they are lines on the ground
    glDisableVertexAttribArray(_lightingShaderAttributeLocations.vNormal);

    _numGridVertices = gridVertices.size();
}

void MPEngine::_generateEnvironment() {
    srand(static_cast<unsigned int>(time(0)));

    // Grid parameters
    const GLfloat GRID_WIDTH = WORLD_SIZE * 1.8f;
    const GLfloat GRID_LENGTH = WORLD_SIZE * 1.8f;
    const GLfloat GRID_SPACING_WIDTH = 1.0f;
    const GLfloat GRID_SPACING_LENGTH = 1.0f;

    // Precomputed parameters based on above
    const GLfloat LEFT_END_POINT = -GRID_WIDTH / 2.0f - 5.0f;
    const GLfloat RIGHT_END_POINT = GRID_WIDTH / 2.0f + 5.0f;
    const GLfloat BOTTOM_END_POINT = -GRID_LENGTH / 2.0f - 5.0f;
    const GLfloat TOP_END_POINT = GRID_LENGTH / 2.0f + 5.0f;

    // Generate Buildings
    for(int i = LEFT_END_POINT; i < RIGHT_END_POINT; i += GRID_SPACING_WIDTH) {
        for(int j = BOTTOM_END_POINT; j < TOP_END_POINT; j += GRID_SPACING_LENGTH) {
            // Don't just draw a building ANYWHERE.
            if( i % 2 && j % 2 && getRand() < 0.02f ) {
                // Translate to spot
                glm::mat4 transToSpotMtx = glm::translate(glm::mat4(1.0f), glm::vec3(i, 0.0f, j));

                // Compute random height
                GLdouble height = powf(getRand(), 2.5)*10 + 1;

                // Scale to building size
                glm::mat4 scaleToHeightMtx = glm::scale(glm::mat4(1.0f), glm::vec3(1, height, 1));

                // Compute full model matrix
                glm::mat4 modelMatrix = transToSpotMtx * scaleToHeightMtx;

                // Compute random color
                glm::vec3 color = glm::vec3(0.5f + 0.5f * getRand(),
                                            0.5f + 0.5f * getRand(),
                                            0.5f + 0.5f * getRand());

                glm::vec3 buildingPosition = glm::vec3(i, height / 2.0f, j);
                float buildingBoundingRadius = glm::sqrt(0.5f * 0.5f + (height / 2.0f) * (height / 2.0f));

                // Store building properties
                BuildingData currentBuilding = {modelMatrix, color, buildingPosition, buildingBoundingRadius};
                _buildings.emplace_back(currentBuilding);
            }
        }
    }
}

void MPEngine::mSetupScene() {
    // Initialize Arcball Camera
    _arcballCam.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    _arcballCam.rotate(0.0f, glm::radians(-30.0f)); // Initial angle

    // Create the Vehicle
    _pVehicle = new Vehicle(_lightingShaderProgram->getShaderProgramHandle(),
                            _lightingShaderUniformLocations.mvpMatrix,
                            _lightingShaderUniformLocations.normalMatrix,
                            _lightingShaderUniformLocations.materialAmbient,
                            _lightingShaderUniformLocations.materialDiffuse,
                            _lightingShaderUniformLocations.materialSpecular,
                            _lightingShaderUniformLocations.materialShininess);

    // Set initial position of Vehicle
    _pVehicle->driveForward(); // Initialize movement if desired

    // Set lighting uniforms
    glm::vec3 lightDirection(-1.0f, -1.0f, -1.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glProgramUniform3fv(_lightingShaderProgram->getShaderProgramHandle(),
                        _lightingShaderProgram->getUniformLocation("lightDirection"),
                        1, glm::value_ptr(lightDirection));
    glProgramUniform3fv(_lightingShaderProgram->getShaderProgramHandle(),
                        _lightingShaderProgram->getUniformLocation("lightColor"),
                        1, glm::value_ptr(lightColor));
}

void MPEngine::_renderScene(glm::mat4 viewMtx, glm::mat4 projMtx) const {
    // Use our lighting shader program
    _lightingShaderProgram->useProgram();

    //// BEGIN DRAWING THE GROUND PLANE ////
    // Draw the ground plane
    glm::mat4 groundModelMtx = glm::scale(glm::mat4(1.0f), glm::vec3(WORLD_SIZE, 1.0f, WORLD_SIZE));
    _computeAndSendMatrixUniforms(groundModelMtx, viewMtx, projMtx);

    // Set material properties for ground (Icy White)
    glm::vec3 groundAmbient(0.8f, 0.8f, 0.9f);    // Icy white ambient
    glm::vec3 groundDiffuse(0.9f, 0.9f, 1.0f);    // Icy white diffuse
    glm::vec3 groundSpecular(0.5f, 0.5f, 0.6f);   // Enhanced specular for icy effect
    float groundShininess = 32.0f;                // Increased shininess for reflectiveness

    glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(groundAmbient));
    glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(groundDiffuse));
    glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(groundSpecular));
    glUniform1f(_lightingShaderUniformLocations.materialShininess, groundShininess);

    glBindVertexArray(_groundVAO);
    glDrawElements(GL_TRIANGLE_STRIP, _numGroundPoints, GL_UNSIGNED_SHORT, (void*)0);
    //// END DRAWING THE GROUND PLANE ////

    //// BEGIN DRAWING THE GRID ////
    // Draw the grid lines
    glm::mat4 gridModelMtx = glm::mat4(1.0f);
    _computeAndSendMatrixUniforms(gridModelMtx, viewMtx, projMtx);

    // Set material properties for grid
    glm::vec3 gridAmbient(0.7f, 0.7f, 0.7f);    // Light gray ambient
    glm::vec3 gridDiffuse(0.8f, 0.8f, 0.8f);    // Light gray diffuse
    glm::vec3 gridSpecular(0.1f, 0.1f, 0.1f);   // Minimal specular
    float gridShininess = 8.0f;

    glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(gridAmbient));
    glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(gridDiffuse));
    glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(gridSpecular));
    glUniform1f(_lightingShaderUniformLocations.materialShininess, gridShininess);

    glBindVertexArray(_gridVAO);
    glDrawArrays(GL_LINES, 0, _numGridVertices);
    //// END DRAWING THE GRID ////

    //// BEGIN DRAWING THE BUILDINGS ////
    for(const BuildingData& building : _buildings) {
        _computeAndSendMatrixUniforms(building.modelMatrix, viewMtx, projMtx);

        // Set material properties for buildings
        glm::vec3 buildingAmbient(0.2f, 0.2f, 0.2f);
        glm::vec3 buildingDiffuse(0.8f, 0.8f, 0.8f); // White buildings
        glm::vec3 buildingSpecular(0.3f, 0.3f, 0.3f);
        float buildingShininess = 32.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(buildingAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(buildingDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(buildingSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, buildingShininess);

        // Draw building as a cone
        CSCI441::drawSolidCone(0.5f, 10.0f, 1, 10);
    }
    //// END DRAWING THE BUILDINGS ////


    //// DRAWING THE VEHICLE ////
    _pVehicle->drawVehicle(viewMtx, projMtx);

}

void MPEngine::_updateScene() {
    bool moved = false;
    glm::vec3 currentPosition = _pVehicle->getPosition();
    glm::vec3 newPosition = currentPosition;

    // Calculate movement direction based on heading
    glm::vec3 forward = glm::vec3(sin(_pVehicle->getHeading()), 0.0f, cos(_pVehicle->getHeading()));
    glm::vec3 backward = -forward;

    // Vehicle Controls - Calculate Proposed New Position
    if(_keys[GLFW_KEY_W]) {
        newPosition += forward * 0.2f;
    }
    if(_keys[GLFW_KEY_S]) {
        newPosition += backward * 0.2f;
    }

    // Backup distance to move back upon collision
    const float BACKUP_DISTANCE = 0.5f;

    // Check collision only if movement keys were pressed
    if(_keys[GLFW_KEY_W] || _keys[GLFW_KEY_S]) {
        if(isMovementValid(newPosition)) {
            // Apply movement
            if(_keys[GLFW_KEY_W]) {
                _pVehicle->driveForward();
            }
            if(_keys[GLFW_KEY_S]) {
                _pVehicle->driveBackward();
            }
            moved = true;
        }
        else {
            // Collision detected
            glm::vec3 backupDirection = (_keys[GLFW_KEY_W]) ? -forward : forward;
            glm::vec3 backupPosition = currentPosition + backupDirection * BACKUP_DISTANCE;

            // Check if backup position is valid
            if(isMovementValid(backupPosition)) {
                _pVehicle->setPosition(backupPosition);
                moved = true;
            }
            else {

            }
        }
    }

    // Handle Turning Independently
    if(_keys[GLFW_KEY_A]) {
        _pVehicle->turnLeft();
        moved = true;
    }
    if(_keys[GLFW_KEY_D]) {
        _pVehicle->turnRight();
        moved = true;
    }

    if(moved) {
        // Update camera target to the vehicle's position
        _arcballCam.setTarget(_pVehicle->getPosition());
    }

    // Bounds Checking to keep the vehicle within the scene
    glm::vec3 pos = _pVehicle->getPosition();
    float halfWorld = WORLD_SIZE;
    pos.x = glm::clamp(pos.x, -halfWorld, halfWorld);
    pos.z = glm::clamp(pos.z, -halfWorld, halfWorld);
    _pVehicle->setPosition(pos);
}

void MPEngine::run() {
    while( !glfwWindowShouldClose(mpWindow) ) {	        // check if the window was instructed to be closed
        glDrawBuffer( GL_BACK );				        // work with our back frame buffer
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );	// clear the current color contents and depth buffer in the window

        // Get the size of our framebuffer.
        GLint framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize( mpWindow, &framebufferWidth, &framebufferHeight );

        glViewport( 0, 0, framebufferWidth, framebufferHeight );

        // Compute the projection matrix
        glm::mat4 projMtx = glm::perspective(glm::radians(45.0f),
                                            static_cast<float>(framebufferWidth) / framebufferHeight,
                                            0.1f, 100.0f);

        // Get the view matrix from the arcball camera
        glm::mat4 viewMtx = _arcballCam.getViewMatrix();

        // Draw the scene
        _renderScene(viewMtx, projMtx);

        // Update the scene based on input
        _updateScene();

        glfwSwapBuffers(mpWindow);
        glfwPollEvents();
    }
}

//*************************************************************************************
//
// Engine Cleanup

void MPEngine::mCleanupShaders() {
    fprintf( stdout, "[INFO]: ...deleting Shaders.\n" );
    delete _lightingShaderProgram;
}

void MPEngine::mCleanupBuffers() {
    fprintf( stdout, "[INFO]: ...deleting VAOs....\n" );
    CSCI441::deleteObjectVAOs();
    glDeleteVertexArrays( 1, &_groundVAO );
    glDeleteVertexArrays( 1, &_gridVAO );

    fprintf( stdout, "[INFO]: ...deleting VBOs....\n" );
    CSCI441::deleteObjectVBOs();

    // Remove Plane deletion as it's no longer needed
    // fprintf(...);
    // delete _pPlane; // Removed

    fprintf( stdout, "[INFO]: ...deleting models..\n" );
    delete _pVehicle;
}

//*************************************************************************************
//
// Private Helper Functions

void MPEngine::_computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    // Compute the Model-View-Projection matrix
    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;

    // Send MVP matrix to shader
    glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(mvpMtx));

    // Compute and send the Normal matrix
    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(modelMtx)));
    glUniformMatrix3fv(_lightingShaderUniformLocations.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalMtx));
}

//*************************************************************************************
//
// Callbacks

void a3_engine_keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods ) {
    auto engine = (MPEngine*) glfwGetWindowUserPointer(window);

    engine->handleKeyEvent(key, action, mods);
}

void a3_engine_cursor_callback(GLFWwindow *window, double x, double y ) {
    auto engine = (MPEngine*) glfwGetWindowUserPointer(window);

    engine->handleCursorPositionEvent(glm::vec2(x, y));
}

void a3_engine_mouse_button_callback(GLFWwindow *window, int button, int action, int mods ) {
    auto engine = (MPEngine*) glfwGetWindowUserPointer(window);

    engine->handleMouseButtonEvent(button, action, mods);
}
