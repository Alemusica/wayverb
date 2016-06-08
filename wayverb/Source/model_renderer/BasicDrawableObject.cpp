#include "BasicDrawableObject.hpp"

#include "common/stl_wrappers.h"

Node::Node(Node&&) noexcept = default;
Node& Node::operator=(Node&&) noexcept = default;

glm::vec3 Node::get_position() const {
    return position;
}

void Node::set_position(const glm::vec3& v) {
    position = v;
}

glm::vec3 Node::get_scale() const {
    return scale;
}

void Node::set_scale(float s) {
    scale = glm::vec3{s};
}

void Node::set_scale(const glm::vec3& s) {
    scale = s;
}

glm::mat4 Node::get_matrix() const {
    return glm::translate(get_position()) * Orientable::get_matrix() *
           glm::scale(get_scale());
}

//----------------------------------------------------------------------------//

BasicDrawableObject::BasicDrawableObject(ShaderProgram& shader,
                                         const std::vector<glm::vec3>& g,
                                         const std::vector<glm::vec4>& c,
                                         const std::vector<GLuint>& i,
                                         GLuint mode)
        : shader(&shader)
        , color_vector(c)
        , mode(mode) {
    geometry.data(g);
    set_highlight(0);
    ibo.data(i);

    auto s_vao = vao.get_scoped();

    geometry.bind();
    auto v_pos = shader.get_attrib_location("v_position");
    glEnableVertexAttribArray(v_pos);
    glVertexAttribPointer(v_pos, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    colors.bind();
    auto c_pos = shader.get_attrib_location("v_color");
    glEnableVertexAttribArray(c_pos);
    glVertexAttribPointer(c_pos, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    ibo.bind();
}

BasicDrawableObject::BasicDrawableObject(BasicDrawableObject&&) noexcept =
        default;
BasicDrawableObject& BasicDrawableObject::operator=(
        BasicDrawableObject&&) noexcept = default;

void BasicDrawableObject::set_highlight(float amount) {
    std::vector<glm::vec4> highlighted(color_vector.size());
    proc::transform(color_vector, highlighted.begin(), [amount](const auto& i) {
        return i + amount;
    });
    colors.data(highlighted);
}

void BasicDrawableObject::draw() const {
    auto s_shader = shader->get_scoped();
    shader->set("v_model", get_matrix());

    auto s_vao = vao.get_scoped();
    glDrawElements(mode, ibo.size(), GL_UNSIGNED_INT, nullptr);
}

GLuint BasicDrawableObject::get_mode() const {
    return mode;
}
void BasicDrawableObject::set_mode(GLuint u) {
    mode = u;
}