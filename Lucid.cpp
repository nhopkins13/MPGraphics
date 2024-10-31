#include "Lucid.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <CSCI441/OpenGLUtils.hpp>

Lucid::Lucid(GLuint shaderProgramHandle, GLint mvpMatrixLocation, GLint normalMatrixLocation,
             GLint materialAmbientLocation, GLint materialDiffuseLocation,
             GLint materialSpecularLocation, GLint materialShininessLocation)
    : _shaderProgramHandle(shaderProgramHandle),
      _mvpMatrixLocation(mvpMatrixLocation),
      _normalMatrixLocation(normalMatrixLocation),
      _materialAmbientLocation(materialAmbientLocation),
      _materialDiffuseLocation(materialDiffuseLocation),
      _materialSpecularLocation(materialSpecularLocation),
      _materialShininessLocation(materialShininessLocation),
      _position(0.0f, 0.0f, 0.0f),
      _heading(0.0f),
      _wingAngle(0.0f)
{}

void Lucid::drawLucid(glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 modelMtx = glm::translate(glm::mat4(1.0f), _position + glm::vec3(0.0f, 0.85f, 0.0f));
    modelMtx = glm::rotate( modelMtx, _rotateHeroAngle, CSCI441::Z_AXIS );

    glm::mat4 rotatedMtx = glm::rotate(glm::mat4(1.0f), _heading + glm::radians(90.0f), CSCI441::X_AXIS);
    glm::mat4 finalModelMtx = modelMtx * rotatedMtx;

    _drawUpperWing(true, finalModelMtx, viewMtx, projMtx);
    _drawUpperWing(false, finalModelMtx, viewMtx, projMtx);

    _drawLowerWing(true, finalModelMtx, viewMtx, projMtx);
    _drawLowerWing(false, finalModelMtx, viewMtx, projMtx);
}

void Lucid::move() {
    _wingAngle += _wingAngleRotationSpeed;
    if( _wingAngle > _2PI ) _wingAngle -= _2PI;
}

void Lucid::moveForward() {
    float speed = 0.2f;
    // Updated movement vector to align with +Z-axis front
    _position += glm::vec3(sin(_heading), 0.0f, cos(_heading)) * speed;
    move();
}

void Lucid::moveBackward() {
    float speed = 0.2f;
    // Updated movement vector to align with +Z-axis front
    _position -= glm::vec3(sin(_heading), 0.0f, cos(_heading)) * speed;
    move();
}

void Lucid::turnLeft() {
    float turnSpeed = glm::radians(2.0f);
    _heading += turnSpeed;

    if (_heading >= 2.0f * M_PI) _heading -= 2.0f * M_PI;
}

void Lucid::turnRight() {
    float turnSpeed = glm::radians(2.0f);
    _heading -= turnSpeed;

    if (_heading < 0.0f) _heading += 2.0f * M_PI;
}

void Lucid::_drawUpperWing(bool isLeftWing, glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const {
    glm::vec3 _colorWing = glm::vec3( 1.0f, 0.8f, 1.0f );
    glm::vec3 _scaleWing = glm::vec3( 0.5f, 1.5f, 1.5f );

    GLfloat _rotateWingAngle = _PI / 2.0f;

    modelMtx = glm::scale( modelMtx, _scaleWing );
    modelMtx = glm::rotate( modelMtx, (isLeftWing ? -1.0f : 1.0f) * _rotateWingAngle, CSCI441::X_AXIS );
    modelMtx = glm::rotate( modelMtx, (0.0f) * _rotateWingAngle, CSCI441::Z_AXIS );

    modelMtx = glm::rotate( modelMtx, (1.0f) * _wingAngle, CSCI441::Z_AXIS );

    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;
    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(modelMtx)));

    glUniformMatrix4fv(_mvpMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvpMtx));
    glUniformMatrix3fv(_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMtx));

    glm::vec3 ambient(0.1f, 0.1f, 0.1f);
    glm::vec3 diffuse(_colorWing);
    glm::vec3 specular(0.3f, 0.3f, 0.3f);
    float shininess = 32.0f;

    glUniform3fv(_materialAmbientLocation, 1, glm::value_ptr(ambient));
    glUniform3fv(_materialDiffuseLocation, 1, glm::value_ptr(diffuse));
    glUniform3fv(_materialSpecularLocation, 1, glm::value_ptr(specular));
    glUniform1f(_materialShininessLocation, shininess);

    CSCI441::drawSolidCone( 0.05f, 0.2f, 16, 4 );
}

void Lucid::_drawLowerWing(bool isLeftWing, glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx ) const {
    glm::vec3 _colorWing = glm::vec3( 1.0f, 0.5f, 1.0f );
    glm::vec3 _scaleWing = glm::vec3( 0.5f, 1.0f, 0.8f );

    GLfloat _rotateWingAngle = _PI / 2.0f;

    modelMtx = glm::scale( modelMtx, _scaleWing );
    modelMtx = glm::rotate( modelMtx, (isLeftWing ? -1.0f : 1.0f) * _rotateWingAngle, CSCI441::X_AXIS );
    modelMtx = glm::rotate( modelMtx, (0.0f) * _rotateWingAngle, CSCI441::Z_AXIS );

    glm::vec3 wingTranslate = glm::vec3(0.0f,0.0f,0.1f);
    modelMtx = glm::translate( modelMtx, (isLeftWing ? (wingTranslate * -1.0f) : wingTranslate) );

    modelMtx = glm::rotate( modelMtx, (-1.0f) * _wingAngle, CSCI441::Z_AXIS );

    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;
    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(modelMtx)));

    glUniformMatrix4fv(_mvpMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvpMtx));
    glUniformMatrix3fv(_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMtx));

    glm::vec3 ambient(0.1f, 0.1f, 0.1f);
    glm::vec3 diffuse(_colorWing);
    glm::vec3 specular(0.3f, 0.3f, 0.3f);
    float shininess = 32.0f;

    glUniform3fv(_materialAmbientLocation, 1, glm::value_ptr(ambient));
    glUniform3fv(_materialDiffuseLocation, 1, glm::value_ptr(diffuse));
    glUniform3fv(_materialSpecularLocation, 1, glm::value_ptr(specular));
    glUniform1f(_materialShininessLocation, shininess);

    CSCI441::drawSolidCone( 0.05f, 0.2f, 16, 4 );
}