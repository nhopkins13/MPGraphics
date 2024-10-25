#include "Lucid.h"

#include <glm/gtc/matrix_transform.hpp>

#include <CSCI441/objects.hpp>
#include <CSCI441/OpenGLUtils.hpp>

Lucid::Lucid( GLuint shaderProgramHandle, GLint mvpMtxUniformLocation, GLint normalMtxUniformLocation, GLint materialColorUniformLocation ) {
    _shaderProgramHandle                            = shaderProgramHandle;
    _shaderProgramUniformLocations.mvpMtx           = mvpMtxUniformLocation;
    _shaderProgramUniformLocations.normalMtx        = normalMtxUniformLocation;
    _shaderProgramUniformLocations.materialColor    = materialColorUniformLocation;

    _wingAngle = 0.0f;
    _wingAngleRotationSpeed = _PI / 20.0f;

    _rotateHeroAngle = _PI / 2.0f;
}

void Lucid::drawHero( glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) {
    modelMtx = glm::rotate( modelMtx, -_rotateHeroAngle, CSCI441::Y_AXIS );
    modelMtx = glm::rotate( modelMtx, _rotateHeroAngle, CSCI441::Z_AXIS );

    _drawUpperWing(true, modelMtx, viewMtx, projMtx);
    _drawUpperWing(false, modelMtx, viewMtx, projMtx);

    _drawLowerWing(true, modelMtx, viewMtx, projMtx);
    _drawLowerWing(false, modelMtx, viewMtx, projMtx);
}

void Lucid::move() {
    _wingAngle += _wingAngleRotationSpeed;
    if( _wingAngle > _2PI ) _wingAngle -= _2PI;
}

void Lucid::_drawUpperWing(bool isLeftWing, glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const {
    glm::vec3 _colorWing = glm::vec3( 1.0f, 0.8f, 1.0f );
    glm::vec3 _scaleWing = glm::vec3( 0.5f, 1.5f, 1.5f );

    GLfloat _rotateWingAngle = _PI / 2.0f;

    modelMtx = glm::scale( modelMtx, _scaleWing );
    modelMtx = glm::rotate( modelMtx, (isLeftWing ? -1.0f : 1.0f) * _rotateWingAngle, CSCI441::X_AXIS );
    modelMtx = glm::rotate( modelMtx, (0.0f) * _rotateWingAngle, CSCI441::Z_AXIS );

    modelMtx = glm::rotate( modelMtx, (1.0f) * _wingAngle, CSCI441::Z_AXIS );

    _computeAndSendMatrixUniforms(modelMtx, viewMtx, projMtx);

    glProgramUniform3fv(_shaderProgramHandle, _shaderProgramUniformLocations.materialColor, 1, &_colorWing[0]);

    CSCI441::drawSolidCone( 0.05f, 0.2f, 16, 4 );
}

void Lucid::_drawLowerWing(bool isLeftWing, glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const {
    glm::vec3 _colorWing = glm::vec3( 0.8f, 1.0f, 1.0f );
    glm::vec3 _scaleWing = glm::vec3( 0.5f, 1.0f, 0.8f );

    GLfloat _rotateWingAngle = _PI / 2.0f;

    modelMtx = glm::scale( modelMtx, _scaleWing );
    modelMtx = glm::rotate( modelMtx, (isLeftWing ? -1.0f : 1.0f) * _rotateWingAngle, CSCI441::X_AXIS );
    modelMtx = glm::rotate( modelMtx, (0.0f) * _rotateWingAngle, CSCI441::Z_AXIS );

    glm::vec3 wingTranslate = glm::vec3(0.0f,0.0f,0.1f);
    modelMtx = glm::translate( modelMtx, (isLeftWing ? (wingTranslate * -1.0f) : wingTranslate) );

    modelMtx = glm::rotate( modelMtx, (-1.0f) * _wingAngle, CSCI441::Z_AXIS );

    _computeAndSendMatrixUniforms(modelMtx, viewMtx, projMtx);

    glProgramUniform3fv(_shaderProgramHandle, _shaderProgramUniformLocations.materialColor, 1, &_colorWing[0]);

    CSCI441::drawSolidCone( 0.05f, 0.2f, 16, 4 );
}

void Lucid::_computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    // precompute the Model-View-Projection matrix on the CPU
    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;
    // then send it to the shader on the GPU to apply to every vertex
    glProgramUniformMatrix4fv( _shaderProgramHandle, _shaderProgramUniformLocations.mvpMtx, 1, GL_FALSE, &mvpMtx[0][0] );

    glm::mat3 normalMtx = glm::mat3( glm::transpose( glm::inverse( modelMtx )));
    glProgramUniformMatrix3fv( _shaderProgramHandle, _shaderProgramUniformLocations.normalMtx, 1, GL_FALSE, &normalMtx[0][0] );
}