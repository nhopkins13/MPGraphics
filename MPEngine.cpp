#include "MPEngine.h"
#include <stb_image.h>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

float getRand() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

MPEngine::MPEngine()
    : CSCI441::OpenGLEngine(4, 1,
                                 1280, 720, // Increased window size for better view
                                 "MP - Over Hill and Under Hill"),
        _pFreeCam(nullptr),
        _pArcballCam(nullptr),
        _pFPCam(nullptr),
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
    delete _textureShaderProgram;
    delete _skyboxShaderProgram;
    delete _pFreeCam;
    delete _pArcballCam;
    delete _pFPCam;
    delete _pVehicle;
    delete _pUFO;
}

void MPEngine::mSetupTextures() {
    glActiveTexture(GL_TEXTURE0);
    _texHandles[TEXTURE_ID::RUG] = _loadAndRegisterTexture("images/groundImage.png");

    // Load and assign the skybox cubemap to texture unit 1
    glActiveTexture(GL_TEXTURE1);
    std::vector<const char*> facesCubemap {
        "images/skyImage.png", // +X
        "images/skyImage.png", // -X
        "images/skyImage.png", // +Y
        "images/skyImage.png", // -Y
        "images/skyImage.png", // +Z
        "images/skyImage.png"  // -Z
    };
    _texHandles[TEXTURE_ID::SKY] = _loadAndRegisterCubemap(facesCubemap);

}

GLuint MPEngine::_loadAndRegisterTexture(const char* FILENAME) {
    // our handle to the GPU
    GLuint textureHandle = 0;

    // enable setting to prevent image from being upside down
    stbi_set_flip_vertically_on_load(false);

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

GLuint MPEngine::_loadAndRegisterCubemap(const std::vector<const char*>& facesCubemap) {
    GLuint cubemapTexture;
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    // Initialize variables to retrieve width, height, and channel info
    int width = 512, height = 512, nrChannels = 3;

    // Load the first face to get dimensions (assuming all faces are the same size)
    unsigned char* data = stbi_load(facesCubemap[0], &width, &height, &nrChannels, 0);
    if (data) {
        // Allocate immutable storage
        glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGB8, width, height);
        stbi_image_free(data); // Free this initial data after getting dimensions
    } else {
        std::cerr << "[ERROR]: Failed to load initial cubemap face for dimensions." << std::endl;
        return 0;
    }

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Load data for each face with glTexSubImage2D
    for (unsigned int i = 0; i < 6; i++) {
        data = stbi_load(facesCubemap[i], &width, &height, &nrChannels, 0);
        if (data) {
            glTexSubImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, 0, 0, width, height,
                GL_RGB, GL_UNSIGNED_BYTE,
                data
            );
            stbi_image_free(data);
        } else {
            std::cerr << "[ERROR]: Failed to load cubemap face: " << facesCubemap[i] << std::endl;
            return 0;
        }
    }

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
    if (key >= 0 && key < NUM_KEYS) {
        _keys[key] = ((action == GLFW_PRESS) || (action == GLFW_REPEAT));
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT) { // Handle key press
        switch (key) {
            // Quit!
            case GLFW_KEY_Q:
            case GLFW_KEY_ESCAPE:
                setWindowShouldClose();
                break;

                // Zoom In/Out with Space
            case GLFW_KEY_SPACE:
                if (currCamera == CameraType::ARCBALL) { // Only zoom if in Arcball mode
                    if (mods & GLFW_MOD_SHIFT) {
                        _pArcballCam->zoomOut();
                    } else {
                        _pArcballCam->zoomIn();
                    }
                }
                break;

            case GLFW_KEY_1:
                currHero = HeroType::VEHICLE;

                break;

            case GLFW_KEY_2:
                currHero = HeroType::UFO;

                break;
            case GLFW_KEY_4:
                currCamera = CameraType::ARCBALL; // Switch to Arcball immediately when UFO is selected
                break;

            case GLFW_KEY_5:
                currCamera = CameraType::FREECAM; // Switch to Freecam immediately
                break;
            case GLFW_KEY_6:
                currCamera = CameraType::FIRSTPERSON; // Switch to First Person view

                if (_pFPCam == nullptr) {
                    _pFPCam = new FPCamera(2.0f);
                }

                if (currHero == HeroType::VEHICLE) {
                    _pFPCam->updatePositionAndOrientation(_pVehicle->getPosition(), _pVehicle->getHeading());
                } else if (currHero == HeroType::UFO) {
                    _pFPCam->updatePositionAndOrientation(_pUFO->getPosition(), _pUFO->getHeading());
                }
                break;

            default:
                break;
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
            _pArcballCam->zoom(deltaZoom);
        }
        else {
            // Perform rotation based on mouse movement
            float deltaTheta = -deltaX * 0.005f;
            float deltaPhi = -deltaY * 0.005f;

            if(currCamera == CameraType::ARCBALL) {
                _pArcballCam->rotate(deltaTheta, deltaPhi);
            }
            else if(currCamera == CameraType::FREECAM) {
                _pFreeCam->rotate(-deltaTheta, deltaPhi);
            }
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
    _lightingShaderProgram = new CSCI441::ShaderProgram("shaders/lighting.vs.glsl", "shaders/lighting.fs.glsl");
    // Retrieve uniform locations
    _lightingShaderUniformLocations.mvpMatrix = _lightingShaderProgram->getUniformLocation("mvpMatrix");
    _lightingShaderUniformLocations.normalMatrix = _lightingShaderProgram->getUniformLocation("normalMatrix");
    _lightingShaderUniformLocations.modelMatrix = _lightingShaderProgram->getUniformLocation("modelMatrix");
    _lightingShaderUniformLocations.viewPos = _lightingShaderProgram->getUniformLocation("viewPos");

    // Material properties
    _lightingShaderUniformLocations.materialAmbient = _lightingShaderProgram->getUniformLocation("material.ambient");
    _lightingShaderUniformLocations.materialDiffuse = _lightingShaderProgram->getUniformLocation("material.diffuse");
    _lightingShaderUniformLocations.materialSpecular = _lightingShaderProgram->getUniformLocation("material.specular");
    _lightingShaderUniformLocations.materialShininess = _lightingShaderProgram->getUniformLocation("material.shininess");

    // Directional Light
    _lightingShaderUniformLocations.dirLightDirection = _lightingShaderProgram->getUniformLocation("dirLight.direction");
    _lightingShaderUniformLocations.dirLightColor = _lightingShaderProgram->getUniformLocation("dirLight.color");

    // Point Lights
    _lightingShaderUniformLocations.numPointLights = _lightingShaderProgram->getUniformLocation("numPointLights");
    _lightingShaderUniformLocations.pointLightPositions = _lightingShaderProgram->getUniformLocation("pointLightPositions");
    _lightingShaderUniformLocations.pointLightColors = _lightingShaderProgram->getUniformLocation("pointLightColors");
    _lightingShaderUniformLocations.pointLightConstants = _lightingShaderProgram->getUniformLocation("pointLightConstants");
    _lightingShaderUniformLocations.pointLightLinears = _lightingShaderProgram->getUniformLocation("pointLightLinears");
    _lightingShaderUniformLocations.pointLightQuadratics = _lightingShaderProgram->getUniformLocation("pointLightQuadratics");

    // Attribute locations
    _lightingShaderAttributeLocations.vPos = _lightingShaderProgram->getAttributeLocation("vPos");
    _lightingShaderAttributeLocations.vNormal = _lightingShaderProgram->getAttributeLocation("vNormal");

    _textureShaderProgram = new CSCI441::ShaderProgram("shaders/texture.vs.glsl", "shaders/texture.fs.glsl");
    _textureShaderUniformLocations.mvpMatrix = _textureShaderProgram->getUniformLocation("mvpMatrix");
    _textureShaderUniformLocations.aTextMap = _textureShaderProgram->getUniformLocation("textureMap");

    _textureShaderAttributeLocations.vPos = _textureShaderProgram->getAttributeLocation("vPos");
    _textureShaderAttributeLocations.aTextCoords = _textureShaderProgram->getAttributeLocation("textureCoords");

    _skyboxShaderProgram = new CSCI441::ShaderProgram("shaders/skybox.vs.glsl", "shaders/skybox.fs.glsl");
    _skyboxShaderUniformLocations.skybox = _skyboxShaderProgram->getUniformLocation("skybox");
    _skyboxShaderUniformLocations.view = _skyboxShaderProgram->getUniformLocation("view");
    _skyboxShaderUniformLocations.projection = _skyboxShaderProgram->getUniformLocation("projection");
}

void MPEngine::mSetupBuffers() {
    //connect our 3D Object Library to our shader
    CSCI441::setVertexAttributeLocations( _lightingShaderAttributeLocations.vPos, _lightingShaderAttributeLocations.vNormal);

    _createGroundBuffers();
    _createSkyBuffers();
    _generateEnvironment();
}

void MPEngine::_createSkyBuffers() {
    // Skybox setup
    float skyboxVertices[] = {
        -1.0f, -1.0f,  1.0f, // 7
         1.0f, -1.0f,  1.0f, // 6
         1.0f, -1.0f, -1.0f, // 5
        -1.0f, -1.0f, -1.0f, // 4
        -1.0f,  1.0f,  1.0f, // 3
         1.0f,  1.0f,  1.0f, // 2
         1.0f,  1.0f, -1.0f, // 1
        -1.0f,  1.0f, -1.0f  // 0
    };

    unsigned int skyboxIndices[] = {
        1, 2, 6, 6, 5, 1,     // Right
        0, 4, 7, 7, 3, 0,     // Left
        4, 5, 6, 6, 7, 4,     // Top
        0, 3, 2, 2, 1, 0,     // Bottom
        0, 1, 5, 5, 4, 0,     // Back
        3, 7, 6, 6, 2, 3      // Front
    };

    // Create VAO for the skybox
    glGenVertexArrays(1, &_skyboxVAO);
    if (_skyboxVAO == 0) {
        std::cerr << "[ERROR]: Skybox VAO not generated correctly." << std::endl;
    }
    glBindVertexArray(_skyboxVAO);

    // Create VBO for skybox vertices
    GLuint skyboxVBO;
    glGenBuffers(1, &skyboxVBO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    // Create EBO for skybox indices
    GLuint skyboxEBO;
    glGenBuffers(1, &skyboxEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind VBO, but keep the EBO bound to the VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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
                    glm::vec3 lampPosition = glm::vec3(transLightMtx[3]);
                    LampData currentLamp = {transToSpotMtx, transLightMtx, lampPosition};
                    _lamps.emplace_back(currentLamp);
                }
            }
        }
    }
}

void MPEngine::mSetupScene() {
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

    // Initialize Arcball Camera
    _pArcballCam = new ArcballCamera();
    _pArcballCam->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    _pArcballCam->rotate(0.0f, glm::radians(-30.0f)); // Initial angle

    //Init free cam
    _pFreeCam = new CSCI441::FreeCam();
    _pFreeCam->setPosition(glm::vec3(60.0f, 40.0f, 30.0f) );
    _pFreeCam->setTheta(-M_PI / 3.0f );
    _pFreeCam->setPhi(M_PI / 2.8f );
    _pFreeCam->recomputeOrientation();
    _cameraSpeed = glm::vec2(0.25f, 0.02f);


    //INIT FPS CAM
    _pFPCam = new FPCamera(2.0f);
    if (currHero == HeroType::VEHICLE) {
        _pFPCam->updatePositionAndOrientation(_pVehicle->getPosition(), _pVehicle->getHeading());
    } else if (currHero == HeroType::UFO) {
        _pFPCam->updatePositionAndOrientation(_pUFO->getPosition(), _pUFO->getHeading());
    }
}

void MPEngine::_renderScene(glm::mat4 viewMtx, glm::mat4 projMtx) const {

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
    glEnable(GL_CULL_FACE);

    _skyboxShaderProgram->useProgram();

    // Set up depth function for skybox rendering
    glDepthFunc(GL_LEQUAL);

    // Bind the skybox cubemap texture to texture unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _texHandles[TEXTURE_ID::SKY]);
    glUniform1i(_skyboxShaderUniformLocations.skybox, 1);


    // Bind VAO and draw the skybox using glDrawElements
    glBindVertexArray(_skyboxVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); // Use glDrawElements to draw with indices
    glBindVertexArray(0);

    // Reset depth function
    glDepthFunc(GL_LESS);

    // Bind the texture shader for ground rendering
    glDisable(GL_CULL_FACE);

    const int MAX_POINT_LIGHTS = 10;

    // Arrays to hold point light data
    glm::vec3 pointLightPositions[MAX_POINT_LIGHTS];
    glm::vec3 pointLightColors[MAX_POINT_LIGHTS];
    float pointLightConstants[MAX_POINT_LIGHTS];
    float pointLightLinears[MAX_POINT_LIGHTS];
    float pointLightQuadratics[MAX_POINT_LIGHTS];

    // Determine the number of point lights
    int numPointLights = std::min(static_cast<int>(_lamps.size()), MAX_POINT_LIGHTS);

    // Populate the point light arrays
    for(int i = 0; i < numPointLights; ++i) {
        pointLightPositions[i] = _lamps[i].position;
        pointLightColors[i] = glm::vec3(0.0f, 0.0f, 1.0f); // Blue color
        pointLightConstants[i] = 1.0f;
        pointLightLinears[i] = 0.09f;
        pointLightQuadratics[i] = 0.032f;
    }

    _lightingShaderProgram->useProgram();
    glm::vec3 cameraPosition;
    if (currCamera == CameraType::ARCBALL) {
        cameraPosition = _pArcballCam->getPosition();
    } else if (currCamera == CameraType::FREECAM) {
        cameraPosition = _pFreeCam->getPosition();
    } else if (currCamera == CameraType::FIRSTPERSON) {
        cameraPosition = _pFPCam->getPosition();
    }

    // Send the camera position to the shader
    glUniform3fv(_lightingShaderUniformLocations.viewPos, 1, glm::value_ptr(cameraPosition));

    // Set directional light uniforms using the correct uniform names and locations
    glm::vec3 lightDirection(-1.0f, -1.0f, -1.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glUniform3fv(_lightingShaderUniformLocations.dirLightDirection, 1, glm::value_ptr(lightDirection));
    glUniform3fv(_lightingShaderUniformLocations.dirLightColor, 1, glm::value_ptr(lightColor));

    glUniform1i(_lightingShaderUniformLocations.numPointLights, numPointLights);
    glUniform3fv(_lightingShaderUniformLocations.pointLightPositions, numPointLights, glm::value_ptr(pointLightPositions[0]));
    glUniform3fv(_lightingShaderUniformLocations.pointLightColors, numPointLights, glm::value_ptr(pointLightColors[0]));
    glUniform1fv(_lightingShaderUniformLocations.pointLightConstants, numPointLights, pointLightConstants);
    glUniform1fv(_lightingShaderUniformLocations.pointLightLinears, numPointLights, pointLightLinears);
    glUniform1fv(_lightingShaderUniformLocations.pointLightQuadratics, numPointLights, pointLightQuadratics);

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

        // Set material properties for lamp posts
        glm::vec3 postAmbient(0.2f, 0.2f, 0.2f);
        glm::vec3 postDiffuse(0.5f, 0.5f, 0.5f);
        glm::vec3 postSpecular(0.3f, 0.3f, 0.3f);
        float postShininess = 32.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(postAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(postDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(postSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, postShininess);

        CSCI441::drawSolidCylinder(0.2, 0.2, 7, 16, 16);
    }

    // Draw lights
    for (const LampData &lamp: _lamps) {
        _computeAndSendMatrixUniforms(lamp.modelMatrixLight, viewMtx, projMtx);

        // Set material properties for lamp lights
        glm::vec3 lightAmbient(0.2f, 0.2f, 0.5f);
        glm::vec3 lightDiffuse(0.0f, 0.0f, 1.0f); // Blue color
        glm::vec3 lightSpecular(0.5f, 0.5f, 0.5f);
        float lightShininess = 64.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(lightAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(lightDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(lightSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, lightShininess);

        CSCI441::drawSolidSphere(0.5, 16, 16);
    }
    //// END DRAWING THE LAMPS ////

    _pVehicle->drawVehicle(viewMtx, projMtx);
    _pUFO->drawUFO(viewMtx, projMtx);
}

void MPEngine::_updateScene() {
    // Backup distance to move back upon collision
    const float BACKUP_DISTANCE = 0.5f;

    // Check current hero type and handle movement for vehicles
    if (currHero == HeroType::VEHICLE) {
        bool moved = false;
        glm::vec3 currentPosition = _pVehicle->getPosition();
        glm::vec3 newPosition = currentPosition;

        // Calculate movement direction based on heading
        glm::vec3 forward = glm::vec3(sin(_pVehicle->getHeading()), 0.0f, cos(_pVehicle->getHeading()));
        glm::vec3 backward = -forward;

        // Vehicle Controls - Calculate Proposed New Position
        if (_keys[GLFW_KEY_W]) {
            newPosition += forward * 0.2f;
        }
        if (_keys[GLFW_KEY_S]) {
            newPosition += backward * 0.2f;
        }
        if ((_keys[GLFW_KEY_W] || _keys[GLFW_KEY_S])) {
            if (isMovementValid(newPosition)) {
                // Apply movement
                if (_keys[GLFW_KEY_W]) {
                    _pVehicle->driveForward();
                }
                if (_keys[GLFW_KEY_S]) {
                    _pVehicle->driveBackward();
                }
                moved = true;
            } else {
                // Collision detected
                glm::vec3 backupDirection = (_keys[GLFW_KEY_W]) ? -forward : forward;
                glm::vec3 backupPosition = currentPosition + backupDirection * BACKUP_DISTANCE;

                // Check if backup position is valid
                if (isMovementValid(backupPosition)) {
                    _pVehicle->setPosition(backupPosition);
                    moved = true;
                }
            }
        }

        // Handle Turning Independently
        if (_keys[GLFW_KEY_A]) {
            _pVehicle->turnLeft();
            moved = true;
        }
        if (_keys[GLFW_KEY_D]) {
            _pVehicle->turnRight();
            moved = true;
        }

        if (moved) {
            // Update camera target to the vehicle's position
            _pArcballCam->setTarget(_pVehicle->getPosition());
        }

        // Bounds Checking to keep the vehicle within the scene
        glm::vec3 pos = _pVehicle->getPosition();
        float halfWorld = WORLD_SIZE;
        pos.x = glm::clamp(pos.x, -halfWorld, halfWorld);
        pos.z = glm::clamp(pos.z, -halfWorld, halfWorld);
        _pVehicle->setPosition(pos);
    }
    else if (currHero == HeroType::UFO) {
        bool moved = false;
        glm::vec3 currentPosition = _pUFO->getPosition();
        glm::vec3 newPosition = currentPosition;

        // Calculate movement direction based on heading
        glm::vec3 forward = glm::vec3(sin(_pUFO->getHeading()), 0.0f, cos(_pUFO->getHeading()));
        glm::vec3 backward = -forward;

        // UFO Controls - Calculate Proposed New Position
        if (_keys[GLFW_KEY_W]) {
            newPosition += forward * 0.2f; // Move forward
        }
        if (_keys[GLFW_KEY_S]) {
            newPosition += backward * 0.2f; // Move backward
        }
        if ((_keys[GLFW_KEY_W] || _keys[GLFW_KEY_S])) {
            if (isMovementValid(newPosition)) {
                // Apply movement
                if (_keys[GLFW_KEY_W]) {
                    _pUFO->flyForward();
                }
                if (_keys[GLFW_KEY_S]) {
                    _pUFO->flyBackward();
                }
                moved = true;
            } else {
                // Collision detected
                glm::vec3 backupDirection = (_keys[GLFW_KEY_W]) ? -forward : forward;
                glm::vec3 backupPosition = currentPosition + backupDirection * BACKUP_DISTANCE;

                // Check if backup position is valid
                if (isMovementValid(backupPosition)) {
                    _pUFO->setPosition(backupPosition);
                    moved = true;
                }
                // If backup position is not valid, we could log a message or handle differently
            }
        }

        // Handle Turning Independently
        if (_keys[GLFW_KEY_A]) {
            _pUFO->turnLeft();
            moved = true;
        }
        if (_keys[GLFW_KEY_D]) {
            _pUFO->turnRight();
            moved = true;
        }

        if (moved) {
            // Update camera target to the UFO's position
            _pArcballCam->setTarget(_pUFO->getPosition());
        }

        // Bounds Checking to keep the UFO within the scene
        glm::vec3 pos = _pUFO->getPosition();
        float halfWorld = WORLD_SIZE;
        pos.x = glm::clamp(pos.x, -halfWorld, halfWorld);
        pos.z = glm::clamp(pos.z, -halfWorld, halfWorld);
        _pUFO->setPosition(pos);
    }

    // Handle Free Camera Movement
    if (currCamera == CameraType::FREECAM) {
        bool moved = false;

        // Free Camera Controls
        if (_keys[GLFW_KEY_SPACE]) {
            if (_keys[GLFW_KEY_LEFT_SHIFT] || _keys[GLFW_KEY_RIGHT_SHIFT]) {
                _pFreeCam->moveBackward(_cameraSpeed.x);
            } else {
                _pFreeCam->moveForward(_cameraSpeed.x);
            }
            moved = true;
        }

        // Turning Controls
        if (_keys[GLFW_KEY_RIGHT]) {
            _pFreeCam->rotate(_cameraSpeed.y, 0.0f);
            moved = true;
        }
        if ( _keys[GLFW_KEY_LEFT]) {
            _pFreeCam->rotate(-_cameraSpeed.y, 0.0f);
            moved = true;
        }

        // Pitch Controls
        if (_keys[GLFW_KEY_UP]) {
            _pFreeCam->rotate(0.0f, _cameraSpeed.y);
            moved = true;
        }
        if (_keys[GLFW_KEY_DOWN]) {
            _pFreeCam->rotate(0.0f, -_cameraSpeed.y);
            moved = true;
        }

        // Update camera target to follow the free cam position
        if (moved) {
            _pArcballCam->setTarget(_pFreeCam->getPosition());
        }
    }

    if (currCamera == CameraType::FIRSTPERSON) {
        if (currHero == HeroType::VEHICLE) {
            _pFPCam->updatePositionAndOrientation(_pVehicle->getPosition(), _pVehicle->getHeading());
        } else if (currHero == HeroType::UFO) {
            _pFPCam->updatePositionAndOrientation(_pUFO->getPosition(), _pUFO->getHeading());
        }
    }
}

void MPEngine::run() {

    while (!glfwWindowShouldClose(mpWindow)) { // Check if the window was instructed to be closed
        glDrawBuffer(GL_BACK); // Work with our back frame buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the current color contents and depth buffer in the window

        // Get the size of our framebuffer.
        GLint framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(mpWindow, &framebufferWidth, &framebufferHeight);

        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glm::mat4 projMtx;
        glm::mat4 viewMtx;

        if (currCamera == CameraType::ARCBALL) {
            projMtx = glm::perspective(glm::radians(45.0f),
                                       static_cast<float>(framebufferWidth) / framebufferHeight,
                                       0.1f, 100.0f);
            viewMtx = _pArcballCam->getViewMatrix();
        }
        else if (currCamera == CameraType::FREECAM) {
            projMtx = _pFreeCam->getProjectionMatrix();
            viewMtx = _pFreeCam->getViewMatrix();
        }

        if (currCamera == CameraType::FIRSTPERSON) {
            //set the view mtx and proj mtx to the first person cameras
            projMtx = glm::perspective(glm::radians(45.0f),
                                       static_cast<float>(framebufferWidth) / framebufferHeight,
                                       0.1f, 100.0f);
            viewMtx = _pFPCam->getViewMatrix();
        }

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

    fprintf( stdout, "[INFO]: ...deleting models..\n" );
    delete _pVehicle;
    delete _pUFO;
}

void MPEngine::mCleanupTextures() {
    fprintf( stdout, "[INFO]: ...deleting textures\n" );
    // TODO #23 - delete textures
    glDeleteTextures(1, &_texHandles[RUG]);
    glDeleteTextures(1, &_texHandles[SKY]);

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

    // Send model matrix to shader
    glUniformMatrix4fv(_lightingShaderUniformLocations.modelMatrix, 1, GL_FALSE, glm::value_ptr(modelMtx));
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