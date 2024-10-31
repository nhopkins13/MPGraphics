#include "UFO.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

UFO::UFO(GLuint shaderProgramHandle, GLint mvpMatrixLocation, GLint normalMatrixLocation,
         GLint materialAmbientLocation, GLint materialDiffuseLocation,
         GLint materialSpecularLocation, GLint materialShininessLocation)
    : _shaderProgramHandle(shaderProgramHandle),
      _mvpMatrixLocation(mvpMatrixLocation),
      _normalMatrixLocation(normalMatrixLocation),
      _materialAmbientLocation(materialAmbientLocation),
      _materialDiffuseLocation(materialDiffuseLocation),
      _materialSpecularLocation(materialSpecularLocation),
      _materialShininessLocation(materialShininessLocation),
      _position(-10.0f, 0.0f, -10.0f),
      _heading(0.0f)
{}

void UFO::drawUFO(glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 modelMtx = glm::translate(glm::mat4(1.0f), _position + glm::vec3(0.0f, 0.85f, 0.0f));
    glm::mat4 rotatedMtx = glm::rotate(glm::mat4(1.0f), _heading + glm::radians(90.0f), glm::vec3(0, 1, 0));
    glm::mat4 finalModelMtx = modelMtx * rotatedMtx;

    drawCraft(finalModelMtx, viewMtx, projMtx);

    drawLookingPort(finalModelMtx, viewMtx, projMtx);
}

void UFO::flyForward() {
    float speed = 0.2f;
    _position += glm::vec3(sin(_heading), 0.0f, cos(_heading)) * speed;
    float wheelRadius = 0.5f;
    float rotationSpeed = (speed / (2.0f * M_PI * wheelRadius)) * 2.0f * M_PI;
}

void UFO::flyBackward() {
    float speed = 0.2f;
    _position -= glm::vec3(sin(_heading), 0.0f, cos(_heading)) * speed;
    float wheelRadius = 0.5f;
    float rotationSpeed = (speed / (2.0f * M_PI * wheelRadius)) * 2.0f * M_PI;
}

void UFO::turnLeft() {
    float turnSpeed = glm::radians(2.0f);
    _heading += turnSpeed;
    if (_heading >= 2.0f * M_PI) _heading -= 2.0f * M_PI;
}

void UFO::turnRight() {
    float turnSpeed = glm::radians(2.0f);
    _heading -= turnSpeed;
    if (_heading < 0.0f) _heading += 2.0f * M_PI;
}


void UFO::drawCraft(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 bodyMtx = modelMtx * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 0.5f, 1.5f));

    glm::mat4 mvpMtx = projMtx * viewMtx * bodyMtx;
    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(bodyMtx)));

    glUniformMatrix4fv(_mvpMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvpMtx));
    glUniformMatrix3fv(_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMtx));

    glm::vec3 ambient(0.5f, 0.5f, 0.5f);
    glm::vec3 diffuse(0.6f, 0.6f, 0.6f);
    glm::vec3 specular(0.9f, 0.9f, 0.9f);
    float shininess = 64.0f;

    glUniform3fv(_materialAmbientLocation, 1, glm::value_ptr(ambient));
    glUniform3fv(_materialDiffuseLocation, 1, glm::value_ptr(diffuse));
    glUniform3fv(_materialSpecularLocation, 1, glm::value_ptr(specular));
    glUniform1f(_materialShininessLocation, shininess);

    CSCI441::drawSolidCube(1.4f);
}

void UFO::drawLookingPort(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 roofMtx = modelMtx * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
    roofMtx = glm::scale(roofMtx, glm::vec3(1.0f, 0.5f, 1.0f)); // Adjust scale as needed

    glm::mat4 mvpMtx = projMtx * viewMtx * roofMtx;
    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(roofMtx)));

    glUniformMatrix4fv(_mvpMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvpMtx));
    glUniformMatrix3fv(_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMtx));
    glm::vec3 ambient(0.2f, 0.2f, 0.5f);
    glm::vec3 diffuse(0.3f, 0.3f, 1.0f);
    glm::vec3 specular(1.0f, 1.0f, 1.0f);
    float shininess = 16.0f;

    glUniform3fv(_materialAmbientLocation, 1, glm::value_ptr(ambient));
    glUniform3fv(_materialDiffuseLocation, 1, glm::value_ptr(diffuse));
    glUniform3fv(_materialSpecularLocation, 1, glm::value_ptr(specular));
    glUniform1f(_materialShininessLocation, shininess);

    CSCI441::drawSolidDome(0.75f,4.0f,32.0f);
}
void UFO::setPosition(glm::vec3 &vec) {
    _position = vec;
}


