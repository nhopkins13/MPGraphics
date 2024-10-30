#include "MPEngine.h"

#include <CSCI441/objects.hpp>
#include <ctime>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

GLfloat getRand() {
    return static_cast<GLfloat>(rand()) / static_cast<GLfloat>(RAND_MAX);
}

MPEngine::MPEngine()
         : CSCI441::OpenGLEngine(4, 1,
                                 1280, 720, // Increased window size for better view
                                 "MP - Over Hill and Under Hill"),
           _pVehicle(nullptr),
            _pUFO(nullptr),
           _animationTime(0.0f),
           _groundVAO(0),
           _numGroundPoints(0)
{
    for(auto& key : _keys) key = GL_FALSE;

    _mousePosition = glm::vec2(MOUSE_UNINITIALIZED, MOUSE_UNINITIALIZED );
    _leftMouseButtonState = GLFW_RELEASE;
}

MPEngine::~MPEngine() {
    delete _lightingShaderProgram;
    delete _pVehicle;
    delete _pUFO;
}

void MPEngine::mSetupTextures() {
    glActiveTexture(GL_TEXTURE0);
    _texHandles[TEXTURE_ID::RUG] = _loadAndRegisterTexture("images/groundImage.png");

    // Load and assign the skybox cubemap to texture unit 1
    glActiveTexture(GL_TEXTURE1);
    std::vector<const char*> skyboxFaces = {
        "images/skyImage.png", // +X
        "images/skyImage.png", // -X
        "images/skyImage.png", // +Y
        "images/skyImage.png", // -Y
        "images/skyImage.png", // +Z
        "images/skyImage.png"  // -Z
    };
    _texHandles[TEXTURE_ID::SKY] = _loadAndRegisterCubemap(skyboxFaces);
}

GLuint MPEngine::_loadAndRegisterTexture(const char* FILENAME) {
    // our handle to the GPU
    GLuint textureHandle = 0;

    // enable setting to prevent image from being upside down
    stbi_set_flip_vertically_on_load(true);

    // will hold image parameters after load
    GLint imageWidth, imageHeight, imageChannels;
    // load image from file
    GLubyte* data = stbi_load( FILENAME, &imageWidth, &imageHeight, &imageChannels, 0);

    // if data was read from file
    if( data ) {
        const GLint STORAGE_TYPE = (imageChannels == 4 ? GL_RGBA : GL_RGB);

        // TODO #01 - generate a texture handle
        glGenTextures(1, &textureHandle);
        // TODO #02 - bind it to be active
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        // set texture parameters
        // TODO #03 - mag filter
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // TODO #04 - min filter
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // TODO #05 - wrap s
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        // TODO #06 - wrap t
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // TODO #07 - transfer image data to the GPU
        glTexImage2D(GL_TEXTURE_2D, 0, STORAGE_TYPE, imageWidth, imageHeight, 0, STORAGE_TYPE, GL_UNSIGNED_BYTE, data);

        fprintf( stdout, "[INFO]: %s texture map read in with handle %d\n", FILENAME, textureHandle);

        // release image memory from CPU - it now lives on the GPU
        stbi_image_free(data);
    } else {
        // load failed
        fprintf( stderr, "[ERROR]: Could not load texture map \"%s\"\n", FILENAME );
    }

    return textureHandle;
}

GLuint MPEngine::_loadAndRegisterCubemap(const std::vector<const char*>& faces) {
    GLuint cubemapTexture;
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    stbi_set_flip_vertically_on_load(false);  // Cubemap faces should not be flipped

    // Load each face of the cubemap
    for (GLuint i = 0; i < faces.size(); i++) {
        int width, height, nrChannels;
        GLubyte* data = stbi_load(faces[i], &width, &height, &nrChannels, 0);

        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            fprintf(stderr, "[ERROR]: Could not load cubemap face \"%s\"\n", faces[i]);
            stbi_image_free(data);
            return 0; // Exit early if a face fails to load
        }
    }

    // Set cubemap-specific texture parameters after loading
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return cubemapTexture;
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
            case GLFW_KEY_1:
                currHero = HeroType::VEHICLE;
            break;

            case GLFW_KEY_2:
                currHero = HeroType::UFO;
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
    glfwSetKeyCallback(mpWindow, mp_engine_keyboard_callback);
    glfwSetMouseButtonCallback(mpWindow, mp_engine_mouse_button_callback);
    glfwSetCursorPosCallback(mpWindow, mp_engine_cursor_callback);

    // Set the user pointer to this instance
    glfwSetWindowUserPointer(mpWindow, this);
}

void MPEngine::mSetupOpenGL() {
    glEnable( GL_DEPTH_TEST );                        // enable depth testing
    glDepthFunc( GL_LESS );                           // use less than depth test

    glEnable(GL_BLEND);                                // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// use one minus blending equation

//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void MPEngine::mSetupShaders() {
    _lightingShaderProgram = new CSCI441::ShaderProgram("shaders/lighting.vs.glsl", "shaders/lighting.fs.glsl" );
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

    _textureShaderProgram = new CSCI441::ShaderProgram("shaders/texture.vs.glsl", "shaders/texture.fs.glsl");
    _textureShaderUniformLocations.mvpMatrix = _textureShaderProgram->getUniformLocation("mvpMatrix");
    _textureShaderUniformLocations.aTextMap = _textureShaderProgram->getUniformLocation("textureMap");

    _textureShaderAttributeLocations.vPos = _textureShaderProgram->getAttributeLocation("vPos");
    _textureShaderAttributeLocations.aTextCoords = _textureShaderProgram->getAttributeLocation("textureCoords");

    _skyboxShaderProgram = new CSCI441::ShaderProgram("shaders/skybox.vs.glsl", "shaders/skybox.fs.glsl");
    _skyboxShaderUniformLocations.view = _skyboxShaderProgram->getUniformLocation("view");
    _skyboxShaderUniformLocations.projection = _skyboxShaderProgram->getUniformLocation("projection");
    _skyboxShaderAttributeLocations.vPos = _skyboxShaderProgram->getAttributeLocation("vPos");

}

void MPEngine::mSetupBuffers() {
    //connect our 3D Object Library to our shader
    CSCI441::setVertexAttributeLocations( _lightingShaderAttributeLocations.vPos, _lightingShaderAttributeLocations.vNormal);

    _createGroundBuffers();
    _generateEnvironment();
    _createSkyBuffers();
}

void MPEngine::_createSkyBuffers() {
    // Skybox setup
    float skyboxVertices[] = {
        // Define vertices for each face of a cube
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
    };

    GLuint skyboxVBO, skyboxVAO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    // Position attribute for skybox vertices
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Unbind VAO and VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Store skybox VAO handle for rendering
    _skyboxVAO = skyboxVAO;
}

void MPEngine::_createGroundBuffers() {
    struct Vertex {
        glm::vec3 position;
        float normalX;
        float normalY;
        float normalZ;
        glm::vec2 texCoords;
    };

    Vertex groundQuad[4] = {
        {{-1.0f, 0.0f, -1.0f}, 0.0f, 1.0f, 0.0f, {0.0f, 0.0f}},
        {{ 1.0f, 0.0f, -1.0f}, 0.0f, 1.0f, 0.0f, {1.0f, 0.0f}},
        {{-1.0f, 0.0f,  1.0f}, 0.0f, 1.0f, 0.0f, {0.0f, 1.0f}},
        {{ 1.0f, 0.0f,  1.0f}, 0.0f, 1.0f, 0.0f, {1.0f, 1.0f}}
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

    glEnableVertexAttribArray(_textureShaderAttributeLocations.vPos);
    glVertexAttribPointer(_textureShaderAttributeLocations.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(_textureShaderAttributeLocations.aTextCoords);
    glVertexAttribPointer(_textureShaderAttributeLocations.aTextCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texCoords)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbods[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
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

    // Generate Objects
    for(int i = LEFT_END_POINT; i < RIGHT_END_POINT; i += GRID_SPACING_WIDTH) {
        for(int j = BOTTOM_END_POINT; j < TOP_END_POINT; j += GRID_SPACING_LENGTH) {
            // Don't just draw an object ANYWHERE.
            if( i % 2 && j % 2 && getRand() < 0.02f ) {
                // Translate to spot
                glm::mat4 transToSpotMtx = glm::translate(glm::mat4(1.0f), glm::vec3(i, 0.0f, j));

                // Choose whether to make object a tree/lamppost
                if (getRand() > 0.2) {
                    glm::mat4 transLeavesMtx = glm::translate(transToSpotMtx, glm::vec3(0, 5, 0));
                    TreeData currentTree = {transToSpotMtx, transLeavesMtx};
                    _trees.emplace_back(currentTree);
                } else {
                    glm::mat4 transLightMtx = glm::translate(transToSpotMtx, glm::vec3(0, 7, 0));

                    // Store lamp properties
                    LampData currentLamp = {transToSpotMtx, transLightMtx};
                    _lamps.emplace_back(currentLamp);
                }
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

    _pUFO = new UFO(_lightingShaderProgram->getShaderProgramHandle(), _lightingShaderUniformLocations.mvpMatrix,
                            _lightingShaderUniformLocations.normalMatrix,
                            _lightingShaderUniformLocations.materialAmbient,
                            _lightingShaderUniformLocations.materialDiffuse,
                            _lightingShaderUniformLocations.materialSpecular,
                            _lightingShaderUniformLocations.materialShininess);


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
    glDepthFunc(GL_LEQUAL);  // Render the skybox at the furthest depth

    // Use skybox shader program
    _skyboxShaderProgram->useProgram();

    // Pass view and projection matrices without translation for skybox
    glm::mat4 view = glm::mat4(glm::mat3(_arcballCam.getViewMatrix()));
    _skyboxShaderProgram->setProgramUniform(_skyboxShaderUniformLocations.view, view);
    _skyboxShaderProgram->setProgramUniform(_skyboxShaderUniformLocations.projection, projMtx);

    // Bind the skybox cubemap texture to texture unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _texHandles[TEXTURE_ID::SKY]);
    glUniform1i(_skyboxShaderProgram->getUniformLocation("skybox"), 1);  // Assign unit 1 to skybox sampler

    // Render the skybox
    glBindVertexArray(_skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);  // Reset depth function for normal scene rendering

    // Bind the texture shader for ground rendering
    _textureShaderProgram->useProgram();

    // Setup the model matrix for the ground
    glm::mat4 groundModelMtx = glm::scale(glm::mat4(1.0f), glm::vec3(WORLD_SIZE, 1.0f, WORLD_SIZE));
    glm::mat4 mvpMtx = projMtx * viewMtx * groundModelMtx;

    // Set uniform for the texture shader's MVP matrix
    _textureShaderProgram->setProgramUniform(_textureShaderUniformLocations.mvpMatrix, mvpMtx);

    // Bind the ground texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texHandles[TEXTURE_ID::RUG]);
    glUniform1i(_textureShaderUniformLocations.aTextMap, 0);

    // Bind and draw the ground VAO
    glBindVertexArray(_groundVAO);
    glDrawElements(GL_TRIANGLE_STRIP, _numGroundPoints, GL_UNSIGNED_SHORT, nullptr);

    _lightingShaderProgram->useProgram();

    //// BEGIN DRAWING THE TREES ////
    // Draw trunks
    for(const TreeData& tree : _trees){
        _computeAndSendMatrixUniforms(tree.modelMatrixTrunk, viewMtx, projMtx);

        // Set material properties for tree trunks
        glm::vec3 trunkAmbient(0.2f, 0.2f, 0.2f);
        glm::vec3 trunkDiffuse(99 / 255.f, 39 / 255.f, 9 / 255.f);
        glm::vec3 trunkSpecular(0.3f, 0.3f, 0.3f);
        float trunkShininess = 32.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(trunkAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(trunkDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(trunkSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, trunkShininess);
        _computeAndSendMatrixUniforms(tree.modelMatrixTrunk, viewMtx, projMtx);
        CSCI441::drawSolidCylinder(1, 1, 5, 16, 16);
    }

    // Draw leaves
    for(const TreeData& tree : _trees){
        _computeAndSendMatrixUniforms(tree.modelMatrixLeaves, viewMtx, projMtx);

        // Set material properties for tree leaves
        glm::vec3 leavesAmbient(0.2f, 0.2f, 0.2f);
        glm::vec3 leavesDiffuse(46 / 255.f, 143 / 255.f, 41 / 255.f);
        glm::vec3 leavesSpecular(0.3f, 0.3f, 0.3f);
        float leavesShininess = 32.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(leavesAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(leavesDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(leavesSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, leavesShininess);
        _computeAndSendMatrixUniforms(tree.modelMatrixLeaves, viewMtx, projMtx);
        CSCI441::drawSolidCone(3, 8, 16, 16);
    }
    //// END DRAWING THE TREES ////
    /////// BEGIN DRAWING THE LAMPS ////
    // Draw posts
    for (const LampData &lamp: _lamps) {
        _computeAndSendMatrixUniforms(lamp.modelMatrixPost, viewMtx, projMtx);

        // Set material properties for buildings
        glm::vec3 buildingAmbient(0.2f, 0.2f, 0.2f);
        glm::vec3 buildingDiffuse(0, 0, 0);
        glm::vec3 buildingSpecular(0.3f, 0.3f, 0.3f);
        float buildingShininess = 32.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(buildingAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(buildingDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(buildingSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, buildingShininess);

        CSCI441::drawSolidCylinder(0.2, 0.2, 7, 16, 16);
    }

    // Draw lights
    for (const LampData &lamp: _lamps) {
        _computeAndSendMatrixUniforms(lamp.modelMatrixLight, viewMtx, projMtx);

        // Set material properties for buildings
        glm::vec3 buildingAmbient(0.2f, 0.2f, 0.2f);
        glm::vec3 buildingDiffuse(0.8f, 0.8f, 0.8f);
        glm::vec3 buildingSpecular(0.3f, 0.3f, 0.3f);
        float buildingShininess = 32.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(buildingAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(buildingDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(buildingSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, buildingShininess);

        CSCI441::drawSolidSphere(0.5, 16, 16);
    }
    //// END DRAWING THE LAMPS ////

    _pVehicle->drawVehicle(viewMtx, projMtx);
    _pUFO->drawUFO(viewMtx, projMtx);
}

void MPEngine::_updateScene() {
    // Backup distance to move back upon collision
    const float BACKUP_DISTANCE = 0.5f;

    if(currHero == HeroType::VEHICLE) {
        bool moved = false;
        glm::vec3 currentPosition= _pVehicle->getPosition();
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
        if((_keys[GLFW_KEY_W] || _keys[GLFW_KEY_S])) {
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
    else if(currHero == HeroType::UFO) {
        bool moved = false;
        glm::vec3 currentPosition= _pUFO->getPosition();
        glm::vec3 newPosition = currentPosition;

        // Calculate movement direction based on heading
        glm::vec3 forward = glm::vec3(sin(_pUFO->getHeading()), 0.0f, cos(_pUFO->getHeading()));
        glm::vec3 backward = -forward;

        // Vehicle Controls - Calculate Proposed New Position
        if(_keys[GLFW_KEY_W]) {
            newPosition += forward * 0.2f;
        }
        if(_keys[GLFW_KEY_S]) {
            newPosition += backward * 0.2f;
        }
        if((_keys[GLFW_KEY_W] || _keys[GLFW_KEY_S])) {
            if(isMovementValid(newPosition)) {
                // Apply movement
                if(_keys[GLFW_KEY_W]) {
                    _pUFO->flyForward();
                }
                if(_keys[GLFW_KEY_S]) {
                    _pUFO->flyBackward();
                }
                moved = true;
            }
            else {
                // Collision detected
                glm::vec3 backupDirection = (_keys[GLFW_KEY_W]) ? -forward : forward;
                glm::vec3 backupPosition = currentPosition + backupDirection * BACKUP_DISTANCE;

                // Check if backup position is valid
                if(isMovementValid(backupPosition)) {
                    _pUFO->setPosition(backupPosition);
                    moved = true;
                }
                else {

                }
            }
        }

        if(_keys[GLFW_KEY_A]) {
            _pUFO->turnLeft();
            moved = true;
        }
        if(_keys[GLFW_KEY_D]) {
            _pUFO->turnRight();
            moved = true;
        }

        if(moved) {
            // Update camera target to the vehicle's position
            _arcballCam.setTarget(_pUFO->getPosition());
        }

        // Bounds Checking to keep the vehicle within the scene
        glm::vec3 pos = _pUFO->getPosition();
        float halfWorld = WORLD_SIZE;
        pos.x = glm::clamp(pos.x, -halfWorld, halfWorld);
        pos.z = glm::clamp(pos.z, -halfWorld, halfWorld);
        _pUFO->setPosition(pos);
    }
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
    delete _textureShaderProgram;
}

void MPEngine::mCleanupBuffers() {
    fprintf( stdout, "[INFO]: ...deleting VAOs....\n" );
    CSCI441::deleteObjectVAOs();
    glDeleteVertexArrays( 1, &_groundVAO );

    fprintf( stdout, "[INFO]: ...deleting VBOs....\n" );
    CSCI441::deleteObjectVBOs();

    // Remove Plane deletion as it's no longer needed
    // fprintf(...);
    // delete _pPlane; // Removed

    fprintf( stdout, "[INFO]: ...deleting models..\n" );
    delete _pVehicle;
}

void MPEngine::mCleanupTextures() {
    fprintf( stdout, "[INFO]: ...deleting textures\n" );
    // TODO #23 - delete textures
    glDeleteTextures(1, &_texHandles[RUG]);


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

void mp_engine_keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods ) {
    auto engine = (MPEngine*) glfwGetWindowUserPointer(window);

    engine->handleKeyEvent(key, action, mods);
}

void mp_engine_cursor_callback(GLFWwindow *window, double x, double y ) {
    auto engine = (MPEngine*) glfwGetWindowUserPointer(window);

    engine->handleCursorPositionEvent(glm::vec2(x, y));
}

void mp_engine_mouse_button_callback(GLFWwindow *window, int button, int action, int mods ) {
    auto engine = (MPEngine*) glfwGetWindowUserPointer(window);

    engine->handleMouseButtonEvent(button, action, mods);
}