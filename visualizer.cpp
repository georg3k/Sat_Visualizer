#include "visualizer.h"

Visualizer::Visualizer() : QWindow()
{
    m_gl_context = new QOpenGLContext;
    m_gl_format = new QSurfaceFormat;
    setSurfaceType(OpenGLSurface);

    // Для инициализации GL контекста необходимо, чтобы виджет имел заданый размер!
    resize(1080, 720);

    // Инициализация GL контекста
    m_gl_format->setRenderableType(QSurfaceFormat::OpenGL);
    m_gl_format->setVersion(4, 2);
    m_gl_format->setProfile(QSurfaceFormat::CoreProfile);

    // Выделение буферов глубины и прозрачности
    m_gl_format->setDepthBufferSize(1);
    m_gl_format->setAlphaBufferSize(1);

    m_gl_context->setFormat(*m_gl_format);
    m_gl_context->create();
}

Visualizer::~Visualizer()
{
    // Освобождение VBO
    for(auto b : m_buffers)
        glDeleteBuffers(1, &b);

    // Освобождение VAO
    glDeleteVertexArrays(1, &m_sphere_vao_id);
    glDeleteVertexArrays(1, &m_orb_vao_id);

    // Освобождение текстур
    glDeleteTextures(1, &m_day_map_id);
    glDeleteTextures(1, &m_night_map_id);
    glDeleteTextures(1, &m_clouds_map_id);
    glDeleteTextures(1, &m_normal_map_id);
    glDeleteTextures(1, &m_specular_map_id);
    glDeleteTextures(1, &m_space_map_id);
    glDeleteTextures(1, &m_moon_map_id);
    glDeleteTextures(1, &m_sun_map_id);

    // Освобождение шейдеров
    glDeleteProgram(m_earth_program_id);
    glDeleteProgram(m_space_program_id);
    glDeleteProgram(m_moon_program_id);
    glDeleteProgram(m_sun_program_id);
    glDeleteProgram(m_orb_program_id);
    glDeleteProgram(m_mark_program_id);
}

void Visualizer::setSunPosition(QVector3D position)
{
    m_sun_position = position;

    // Пересчет матрицы и бновление юниформ
    updateSunUniforms();
}

void Visualizer::setMoonPosition(QVector3D position)
{
    m_moon_position = position;

    // Пересчет матрицы и обновление юниформ
    updateMoonUniforms();
}

void Visualizer::setMoonRotation(QVector3D rotation)
{
    m_moon_rotation = rotation;

    // Пересчет матрицы и обновление юниформ
    updateMoonUniforms();
}

void Visualizer::setCameraTarget(QVector3D target)
{
    m_camera_target = target;

    // Пересчет матрицы и обновление юниформ
    updateViewUniforms();
}

void Visualizer::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);
    if (isExposed())
        render();
}

void Visualizer::render()
{
    if(!m_is_init)
        init();

    // Очистка FrameBuffer'а
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Отрисовка скайбокса
    glUseProgram(m_space_program_id);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glFrontFace(GL_CW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_space_map_id);

    glBindVertexArray(m_sphere_vao_id);
    glDrawElements(GL_TRIANGLES, m_sphere_indices_count, GL_UNSIGNED_INT, (void*)NULL);
    glBindVertexArray(0);

    // Отрисовка Солнца
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glUseProgram(m_sun_program_id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sun_map_id);

    glBindVertexArray(m_sphere_vao_id);
    glDrawElements(GL_TRIANGLES, m_sphere_indices_count, GL_UNSIGNED_INT, (void*)NULL);
    glBindVertexArray(0);

    // Отрисовка Луны
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glFrontFace(GL_CCW);
    glDepthFunc(GL_LESS);
    glUseProgram(m_moon_program_id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_moon_map_id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_moon_normal_map_id);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_moon_specular_map_id);

    glBindVertexArray(m_sphere_vao_id);
    glDrawElements(GL_TRIANGLES, m_sphere_indices_count, GL_UNSIGNED_INT, (void*)NULL);
    glBindVertexArray(0);

    // Отрисовка Земли
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glFrontFace(GL_CCW);
    glDepthFunc(GL_LESS);
    glUseProgram(m_earth_program_id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_day_map_id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_night_map_id);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_clouds_map_id);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_normal_map_id);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, m_specular_map_id);

    glUniform1f(m_earth_t_uni_id, (MILLS % 1000000) / 1000000.0f);

    glBindVertexArray(m_sphere_vao_id);
    glDrawElements(GL_TRIANGLES, m_sphere_indices_count, GL_UNSIGNED_INT, (void*)NULL);
    glBindVertexArray(0);

    // Отрисовка меток
    glUseProgram(m_mark_program_id);
    glDepthMask(GL_TRUE);

    // Зеленые
    glUniform3f(m_mark_color_uni_id, 0.1f, 1.0f, 0.1f);
    for(auto m : green_marks)
    {
        m_mark_model_mat.setToIdentity();
        m_mark_model_mat.translate(m);
        glUniformMatrix4fv(m_mark_model_uni_id, 1, false, m_mark_model_mat.data());

        glBindVertexArray(m_mark_vao_id);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    // Красные
    glUniform3f(m_mark_color_uni_id, 1.0f, 0.1f, 0.1f);
    for(auto m : red_marks)
    {
        m_mark_model_mat.setToIdentity();
        m_mark_model_mat.translate(m);
        glUniformMatrix4fv(m_mark_model_uni_id, 1, false, m_mark_model_mat.data());

        glBindVertexArray(m_mark_vao_id);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    // Отрисовка орбит
    glUseProgram(m_orb_program_id);

    // Зеленые
    glUniform3f(m_orb_col_uni_id, 0.1f, 1.0f, 0.1f);
    for(unsigned int i = 0; i < green_orbits_tilt.size(); i++)
    {
        m_orb_model_mat.setToIdentity();
        m_orb_model_mat.translate(green_orbits_offset[i]);
        m_orb_model_mat.rotate(QQuaternion::fromEulerAngles(green_orbits_tilt[i]));
        m_orb_model_mat.scale(green_orbits_scale[i]);
        glUniformMatrix4fv(m_orb_model_uni_id, 1, false, m_orb_model_mat.data());
        glUniform3f(m_orb_targ_uni_id, green_marks[i].x(), green_marks[i].y(), green_marks[i].z());

        glUseProgram(m_orb_program_id);
        glBindVertexArray(m_orb_vao_id);
        glDrawArrays(GL_LINE_LOOP, 0, 400);
    }

    // Красные
    glUniform3f(m_orb_col_uni_id, 1.0f, 0.1f, 0.1f);
    for(unsigned int i = 0; i < red_orbits_tilt.size(); i++)
    {
        m_orb_model_mat.setToIdentity();
        m_orb_model_mat.translate(red_orbits_offset[i]);
        m_orb_model_mat.rotate(QQuaternion::fromEulerAngles(red_orbits_tilt[i]));
        m_orb_model_mat.scale(red_orbits_scale[i]);
        glUniformMatrix4fv(m_orb_model_uni_id, 1, false, m_orb_model_mat.data());
        glUniform3f(m_orb_targ_uni_id, red_marks[i].x(), red_marks[i].y(), red_marks[i].z());

        glUseProgram(m_orb_program_id);
        glBindVertexArray(m_orb_vao_id);
        glDrawArrays(GL_LINE_LOOP, 0, 400);
    }

    glBindVertexArray(0);

    m_gl_context->swapBuffers(this);
}

void Visualizer::init()
{
    m_gl_context->makeCurrent(this);
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glViewport(0, 0, width(), height());
    glPointSize(20.0f);
    glLineWidth(5.0f);

    // Шейдер Земли
    GLuint vs_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vs_source = "#version 420 core\n" \
                            "layout(location = 0) in vec3 position;\n" \
                            "layout(location = 1) in vec3 normal;\n" \
                            "layout(location = 2) in vec3 bitangent;\n" \
                            "layout(location = 3) in vec2 uv;\n" \
                            "uniform mat4 model_matrix;\n" \
                            "uniform mat4 view_matrix;\n" \
                            "uniform mat4 proj_matrix;\n" \
                            "out vec3 normal_itp;\n" \
                            "out vec3 bitangent_itp;\n" \
                            "out vec2 uv_itp;\n" \
                            "out vec3 frag_pos;\n" \
                            "void main() {\n" \
                            "   gl_Position = proj_matrix * view_matrix * model_matrix * vec4(position, 1.0);\n" \
                            "   uv_itp = uv;\n" \
                            "   normal_itp = normal;\n" \
                            "   bitangent_itp = -bitangent;\n" \
                            "   frag_pos = (model_matrix * vec4(position, 1.0)).xyz;\n" \
                            "}\n";

    const char *fs_source = "#version 420 core\n" \
                            "in vec3 normal_itp;\n" \
                            "in vec3 bitangent_itp;\n" \
                            "in vec2 uv_itp;\n" \
                            "in vec3 frag_pos;\n" \
                            "layout (binding = 0) uniform sampler2D day_map;\n" \
                            "layout (binding = 1) uniform sampler2D night_map;\n" \
                            "layout (binding = 2) uniform sampler2D clouds_map;\n" \
                            "layout (binding = 3) uniform sampler2D normal_map;\n" \
                            "layout (binding = 4) uniform sampler2D specular_map;\n" \
                            "uniform mat4 model_matrix;\n" \
                            "uniform vec3 camera_pos;\n" \
                            "uniform vec3 sun_pos;\n" \
                            "uniform float t;\n" \
                            "out vec4 color;\n" \
                            "void main() {\n" \
                            "   vec3 T = normalize(vec3(model_matrix * vec4(cross(normal_itp, bitangent_itp), 0.0)));\n" \
                            "   vec3 B = normalize(vec3(model_matrix * vec4(bitangent_itp, 0.0)));\n" \
                            "   vec3 N = normalize(vec3(model_matrix * vec4(normal_itp, 0.0)));\n" \
                            "   mat3 tbn = mat3(T, B, N);\n" \
                            "   vec3 normal_comp = tbn * (texture(normal_map, uv_itp).rgb * 2.0 - 1.0);\n" \
                            "   vec3 view_dir = normalize(camera_pos - frag_pos);\n" \
                            "   vec3 sun_dir = normalize(sun_pos - frag_pos);\n" \
                            "   vec3 sun_ref = reflect(-sun_dir, normal_comp);\n" \
                            "   float spec = max(texture(specular_map, uv_itp).r - texture(clouds_map, uv_itp + vec2(t, 0)).r, 0.0) * pow(clamp(dot(normal_comp, normalize(view_dir + sun_dir)), 0.0, 1.0), 20.0);" \
                            "   float diffuse = max(dot(sun_dir, normal_comp), 0.0);\n" \
                            "   color = mix("
                            "       mix(texture(night_map, uv_itp), texture(clouds_map, uv_itp + vec2(t, 0)) * 0.1,"
                            "       texture(clouds_map, uv_itp + vec2(t, 0)).a),"
                            "       diffuse * mix(texture(day_map, uv_itp), texture(clouds_map, uv_itp + vec2(t, 0)),"
                            "       texture(clouds_map, uv_itp + vec2(t, 0)).a),"
                            "       diffuse) + 0.5 * vec4(spec);\n" \
                            "       color.a = 1.0;\n" \
                            "}\n";

    GLint result;

    glShaderSource(vs_id, 1, &vs_source, NULL);
    glCompileShader(vs_id);
    glGetShaderiv(vs_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(vs_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(vs_id, length, &length, info);
        printf("Earth VS: %s",info);
    }

    glShaderSource(fs_id, 1, &fs_source, NULL);
    glCompileShader(fs_id);
    glGetShaderiv(fs_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(fs_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(fs_id, length, &length, info);
        printf("Earth FS: %s",info);
    }

    m_earth_program_id = glCreateProgram();
    glAttachShader(m_earth_program_id, vs_id);
    glAttachShader(m_earth_program_id, fs_id);
    glLinkProgram(m_earth_program_id);
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);

    m_earth_model_uni_id = glGetUniformLocation(m_earth_program_id, "model_matrix");
    m_earth_view_uni_id = glGetUniformLocation(m_earth_program_id, "view_matrix");
    m_earth_proj_uni_id = glGetUniformLocation(m_earth_program_id, "proj_matrix");
    m_earth_cam_pos_uni_id = glGetUniformLocation(m_earth_program_id, "camera_pos");
    m_earth_sun_pos_uni_id = glGetUniformLocation(m_earth_program_id, "sun_pos");
    m_earth_t_uni_id = glGetUniformLocation(m_earth_program_id, "t");

    // Шейдер космоса
    GLuint vs_space_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs_space_id = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vs_space_source = "#version 420 core\n" \
                                  "layout(location = 0) in vec3 position;\n" \
                                  "layout(location = 3) in vec2 uv;\n" \
                                  "uniform mat4 view_matrix;\n" \
                                  "uniform mat4 proj_matrix;\n" \
                                  "out vec2 uv_itp;\n" \
                                  "void main() {\n" \
                                  "   gl_Position = proj_matrix * vec4((view_matrix * vec4(position, 0.0)).xyz, 1.0);\n" \
                                  "   uv_itp = uv;\n" \
                                  "}\n";

    const char *fs_space_source = "#version 420 core\n" \
                                  "in vec2 uv_itp;\n" \
                                  "out vec4 color;\n" \
                                  "layout (binding = 0) uniform sampler2D space_map;\n" \
                                  "void main() {\n" \
                                  "   color = texture(space_map, uv_itp);\n" \
                                  "   color.a = 1.0;\n" \
                                  "}\n";

    glShaderSource(vs_space_id, 1, &vs_space_source, NULL);
    glCompileShader(vs_space_id);
    glGetShaderiv(vs_space_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(vs_space_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(vs_space_id, length, &length, info);
        printf("Space VS: %s",info);
    }

    glShaderSource(fs_space_id, 1, &fs_space_source, NULL);
    glCompileShader(fs_space_id);
    glGetShaderiv(fs_space_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(fs_space_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(fs_space_id, length, &length, info);
        printf("Space FS: %s",info);
    }

    m_space_program_id = glCreateProgram();
    glAttachShader(m_space_program_id, vs_space_id);
    glAttachShader(m_space_program_id, fs_space_id);
    glLinkProgram(m_space_program_id);
    glDeleteShader(vs_space_id);
    glDeleteShader(fs_space_id);

    m_space_view_uni_id = glGetUniformLocation(m_space_program_id, "view_matrix");
    m_space_proj_uni_id = glGetUniformLocation(m_space_program_id, "proj_matrix");

    // Шейдер Луны
    GLuint vs_moon_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs_moon_id = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vs_moon_source = "#version 420 core\n" \
                               "layout(location = 0) in vec3 position;\n" \
                               "layout(location = 1) in vec3 normal;\n" \
                               "layout(location = 2) in vec3 bitangent;\n" \
                               "layout(location = 3) in vec2 uv;\n" \
                               "uniform mat4 model_matrix;\n" \
                               "uniform mat4 view_matrix;\n" \
                               "uniform mat4 proj_matrix;\n" \
                               "out vec2 uv_itp;\n" \
                               "out vec3 normal_itp;\n" \
                               "out vec3 bitangent_itp;\n" \
                               "out vec3 frag_pos;\n" \
                               "void main() {\n" \
                               "   gl_Position = proj_matrix * view_matrix * model_matrix * vec4(position, 1.0);\n" \
                               "   uv_itp = uv;\n" \
                               "   normal_itp = normal;\n" \
                               "   bitangent_itp = -bitangent;\n" \
                               "   frag_pos = (model_matrix * vec4(position, 1.0)).xyz;\n" \
                               "}\n";

    const char *fs_moon_source = "#version 420 core\n" \
                               "in vec2 uv_itp;\n" \
                               "in vec3 normal_itp;\n" \
                               "in vec3 bitangent_itp;\n" \
                               "in vec3 frag_pos;\n" \
                               "uniform vec3 sun_pos;\n" \
                               "uniform vec3 camera_pos;\n" \
                               "uniform mat4 model_matrix;\n" \
                               "layout (binding = 0) uniform sampler2D color_map;\n" \
                               "layout (binding = 1) uniform sampler2D normal_map;\n" \
                               "layout (binding = 2) uniform sampler2D specular_map;\n" \
                                 "out vec4 color;\n" \
                               "void main() {\n" \
                               "   vec3 T = normalize(vec3(model_matrix * vec4(cross(normal_itp, bitangent_itp), 0.0)));\n" \
                               "   vec3 B = normalize(vec3(model_matrix * vec4(bitangent_itp, 0.0)));\n" \
                               "   vec3 N = normalize(vec3(model_matrix * vec4(normal_itp, 0.0)));\n" \
                               "   mat3 tbn = mat3(T, B, N);\n" \
                               "   vec3 normal_comp = tbn * (texture(normal_map, uv_itp).rgb * 2.0 - 1.0);\n" \
                               "   vec3 sun_dir = normalize(sun_pos - frag_pos);\n" \
                               "   vec3 view_dir = normalize(camera_pos - frag_pos);\n" \
                               "   vec3 sun_ref = reflect(-sun_dir, normal_comp);\n" \
                               "   float spec = texture(specular_map, uv_itp).r * pow(max(dot(normal_comp, normalize(view_dir + sun_dir)), 0.0), 25.0);" \
                               "   float diffuse = max(dot(sun_dir, normal_comp), 0.0);\n" \
                               "   color = diffuse * texture(color_map, uv_itp) + 0.5 * vec4(spec);\n" \
                               "   color.a = 1.0;\n" \
                               "}\n";

    glShaderSource(vs_moon_id, 1, &vs_moon_source, NULL);
    glCompileShader(vs_moon_id);
    glGetShaderiv(vs_moon_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(vs_moon_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(vs_moon_id, length, &length, info);
        printf("Moon VS: %s",info);
    }

    glShaderSource(fs_moon_id, 1, &fs_moon_source, NULL);
    glCompileShader(fs_moon_id);
    glGetShaderiv(fs_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(fs_moon_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(fs_moon_id, length, &length, info);
        printf("Moon FS: %s",info);
    }

    m_moon_program_id = glCreateProgram();
    glAttachShader(m_moon_program_id, vs_moon_id);
    glAttachShader(m_moon_program_id, fs_moon_id);
    glLinkProgram(m_moon_program_id);
    glDeleteShader(vs_moon_id);
    glDeleteShader(fs_moon_id);

    m_moon_model_uni_id = glGetUniformLocation(m_moon_program_id, "model_matrix");
    m_moon_view_uni_id = glGetUniformLocation(m_moon_program_id, "view_matrix");
    m_moon_proj_uni_id = glGetUniformLocation(m_moon_program_id, "proj_matrix");
    m_moon_sun_pos_uni_id = glGetUniformLocation(m_moon_program_id, "sun_pos");
    m_moon_cam_pos_uni_id = glGetUniformLocation(m_moon_program_id, "camera_pos");

    // Шейдер Солнца
    GLuint vs_sun_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs_sun_id = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vs_sun_source = "#version 420 core\n" \
                               "layout(location = 0) in vec3 position;\n" \
                               "layout(location = 3) in vec2 uv;\n" \
                               "uniform mat4 model_matrix;\n" \
                               "uniform mat4 view_matrix;\n" \
                               "uniform mat4 proj_matrix;\n" \
                               "out vec2 uv_itp;\n" \
                               "void main() {\n" \
                               "   gl_Position = proj_matrix * view_matrix * model_matrix * vec4(position, 1.0);\n" \
                               "   uv_itp = uv;\n" \
                               "}\n";

    const char *fs_sun_source = "#version 420 core\n" \
                               "in vec2 uv_itp;\n" \
                               "out vec4 color;\n" \
                               "layout (binding = 0) uniform sampler2D color_map;\n" \
                               "void main() {\n" \
                               "   color = 5.0 * texture(color_map, uv_itp);\n" \
                               "   color.a = 1.0;\n" \
                               "}\n";

    glShaderSource(vs_sun_id, 1, &vs_sun_source, NULL);
    glCompileShader(vs_sun_id);
    glGetShaderiv(vs_sun_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(vs_sun_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(vs_sun_id, length, &length, info);
        printf("Sun VS: %s",info);
    }

    glShaderSource(fs_sun_id, 1, &fs_sun_source, NULL);
    glCompileShader(fs_sun_id);
    glGetShaderiv(fs_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(fs_sun_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(fs_sun_id, length, &length, info);
        printf("Sun FS: %s",info);
    }

    m_sun_program_id = glCreateProgram();
    glAttachShader(m_sun_program_id, vs_sun_id);
    glAttachShader(m_sun_program_id, fs_sun_id);
    glLinkProgram(m_sun_program_id);
    glDeleteShader(vs_sun_id);
    glDeleteShader(fs_sun_id);

    m_sun_model_uni_id = glGetUniformLocation(m_earth_program_id, "model_matrix");
    m_sun_view_uni_id = glGetUniformLocation(m_earth_program_id, "view_matrix");
    m_sun_proj_uni_id = glGetUniformLocation(m_earth_program_id, "proj_matrix");

    // Шейдер орбит
    GLuint vs_orb_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs_orb_id = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vs_orb_source = "#version 420 core\n" \
                               "layout(location = 0) in vec3 position;\n" \
                               "uniform mat4 model_matrix;\n" \
                               "uniform mat4 view_matrix;\n" \
                               "uniform mat4 proj_matrix;\n" \
                               "out vec3 pos_int;\n" \
                               "void main() {\n" \
                               "   pos_int = (model_matrix * vec4(position, 1.0)).xyz;\n" \
                               "   gl_Position = proj_matrix * view_matrix * model_matrix * vec4(position, 1.0);\n" \
                               "}\n";

    const char *fs_orb_source = "#version 420 core\n" \
                               "in vec3 pos_int;\n" \
                               "out vec4 color;\n" \
                               "uniform vec3 target_pos;\n" \
                               "uniform vec3 col;\n" \
                               "void main() {\n" \
                               "   float alpha = min(1.0 / pow(distance(target_pos, pos_int), 5.0), 1.0);\n" \
                               "   color = vec4(col, alpha);\n" \
                               "}\n";

    glShaderSource(vs_orb_id, 1, &vs_orb_source, NULL);
    glCompileShader(vs_orb_id);
    glGetShaderiv(vs_orb_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(vs_orb_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(vs_orb_id, length, &length, info);
        printf("Orbit VS: %s",info);
    }

    glShaderSource(fs_orb_id, 1, &fs_orb_source, NULL);
    glCompileShader(fs_orb_id);
    glGetShaderiv(fs_orb_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(fs_orb_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(fs_orb_id, length, &length, info);
        printf("Orbit FS: %s",info);
    }

    m_orb_program_id = glCreateProgram();
    glAttachShader(m_orb_program_id, vs_orb_id);
    glAttachShader(m_orb_program_id, fs_orb_id);
    glLinkProgram(m_orb_program_id);
    glDeleteShader(vs_orb_id);
    glDeleteShader(fs_orb_id);

    m_orb_model_uni_id = glGetUniformLocation(m_orb_program_id, "model_matrix");
    m_orb_view_uni_id = glGetUniformLocation(m_orb_program_id, "view_matrix");
    m_orb_proj_uni_id = glGetUniformLocation(m_orb_program_id, "proj_matrix");
    m_orb_targ_uni_id = glGetUniformLocation(m_orb_program_id, "target_pos");
    m_orb_col_uni_id = glGetUniformLocation(m_orb_program_id, "col");

    // Шейдер спутников
    GLuint vs_mark_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs_mark_id = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vs_mark_source = "#version 420 core\n" \
                               "layout(location = 0) in vec3 position;\n" \
                               "uniform mat4 model_matrix;\n" \
                               "uniform mat4 view_matrix;\n" \
                               "uniform mat4 proj_matrix;\n" \
                               "out vec2 pos;\n" \
                               "void main() {\n" \
                               "   gl_Position = proj_matrix * view_matrix * model_matrix * vec4(position, 1.0);\n" \
                               "   pos = (proj_matrix * view_matrix * model_matrix * vec4(position, 1.0)).xy;\n" \
                               "}\n";

    const char *fs_mark_source = "#version 420 core\n" \
                               "out vec4 color;\n" \
                               "uniform vec3 col;\n" \
                               "in vec2 pos;\n" \
                               "void main() {\n" \
                               "   vec2 cxy = 2.0 * gl_PointCoord - 1.0;\n" \
                               "   float r = dot(cxy, cxy);\n" \
                               "   if(r > 1.0)\n" \
                               "      discard;\n" \
                               "   color = vec4(r < 0.5 ? col : col * 0.5, 1.0);\n" \
                               "}\n";

    glShaderSource(vs_mark_id, 1, &vs_mark_source, NULL);
    glCompileShader(vs_mark_id);
    glGetShaderiv(vs_mark_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(vs_mark_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(vs_mark_id, length, &length, info);
        printf("mark VS: %s",info);
    }

    glShaderSource(fs_mark_id, 1, &fs_mark_source, NULL);
    glCompileShader(fs_mark_id);
    glGetShaderiv(fs_mark_id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(fs_mark_id, GL_INFO_LOG_LENGTH, &length);
        char info[length];
        glGetShaderInfoLog(fs_mark_id, length, &length, info);
        printf("mark FS: %s",info);
    }

    m_mark_program_id = glCreateProgram();
    glAttachShader(m_mark_program_id, vs_mark_id);
    glAttachShader(m_mark_program_id, fs_mark_id);
    glLinkProgram(m_mark_program_id);
    glDeleteShader(vs_mark_id);
    glDeleteShader(fs_mark_id);

    m_mark_model_uni_id = glGetUniformLocation(m_mark_program_id, "model_matrix");
    m_mark_view_uni_id = glGetUniformLocation(m_mark_program_id, "view_matrix");
    m_mark_proj_uni_id = glGetUniformLocation(m_mark_program_id, "proj_matrix");
    m_mark_color_uni_id = glGetUniformLocation(m_mark_program_id, "col");

    // Загрузка модели сферы
    Assimp::Importer imp;
    const aiScene *scene = imp.ReadFile("sphere.obj", aiProcess_CalcTangentSpace | aiProcess_FlipUVs);

    if(!scene)
        qDebug() << "Error during mesh loading";

    // Загружает только первую сетку, так как загрузка целых сцен не предпологается
    aiMesh *mesh = *(scene->mMeshes);

    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> bitangents;
    std::vector<GLfloat> UVs;
    std::vector<GLuint> indices;

    for(int i = 0; i < mesh->mNumVertices; i++)
    {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);

        if(mesh->HasNormals())
        {
            normals.push_back(mesh->mNormals[i].x);
            normals.push_back(mesh->mNormals[i].y);
            normals.push_back(mesh->mNormals[i].z);
        }

        if(mesh->HasTangentsAndBitangents())
        {
            bitangents.push_back(mesh->mBitangents[i].x);
            bitangents.push_back(mesh->mBitangents[i].y);
            bitangents.push_back(mesh->mBitangents[i].z);
        }

        if(mesh->HasTextureCoords(0))
        {
            UVs.push_back(mesh->mTextureCoords[0][i].x);
            UVs.push_back(mesh->mTextureCoords[0][i].y);
        }
    }

    for(int i = 0; i < mesh->mNumFaces; i++)
    {
        indices.push_back(mesh->mFaces[i].mIndices[0]);
        indices.push_back(mesh->mFaces[i].mIndices[1]);
        indices.push_back(mesh->mFaces[i].mIndices[2]);
    }
    m_sphere_indices_count = indices.size();

    // Загрузка сферы в графическую память
    glGenVertexArrays(1, &m_sphere_vao_id);
    glBindVertexArray(m_sphere_vao_id);

    GLuint vertices_vbo;
    glGenBuffers(1, &vertices_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    m_buffers.push_back(vertices_vbo);

    GLuint normals_vbo;
    if(mesh->HasNormals())
    {
        glGenBuffers(1, &normals_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * normals.size(), normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        m_buffers.push_back(normals_vbo);
    }

    GLuint bitangents_vbo;
    if(mesh->HasTangentsAndBitangents())
    {
        glGenBuffers(1, &bitangents_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, bitangents_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * bitangents.size(), bitangents.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        m_buffers.push_back(bitangents_vbo);
    }

    GLuint uvs_vbo;
    if(mesh->HasTextureCoords(0))
    {
        glGenBuffers(1, &uvs_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, uvs_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * UVs.size(), UVs.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        m_buffers.push_back(uvs_vbo);
    }

    GLuint indices_vbo;
    glGenBuffers(1, &indices_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);
    m_buffers.push_back(indices_vbo);

    glBindVertexArray(0);

    // Генерация орбиты
    glGenVertexArrays(1, &m_orb_vao_id);
    glBindVertexArray(m_orb_vao_id);

    std::vector<float> orb_vertices;

    for(int i = 0; i < 400; i++)
    {
        float t = 2 * M_PI * i / 200;

        orb_vertices.push_back(std::cos(t)); // x
        orb_vertices.push_back(0.0f);        // y
        orb_vertices.push_back(std::sin(t)); // z
    }

    GLuint orb_vertices_vbo;
    glGenBuffers(1, &orb_vertices_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, orb_vertices_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * orb_vertices.size(), orb_vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    m_buffers.push_back(orb_vertices_vbo);

    glBindVertexArray(0);

    // Метка спутника
    glGenVertexArrays(1, &m_mark_vao_id);
    glBindVertexArray(m_mark_vao_id);

    GLfloat mark_data[3] = {0.0f, 0.0f, 0.0f};

    GLuint mark_vertices_vbo;
    glGenBuffers(1, &mark_vertices_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mark_vertices_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3, mark_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    m_buffers.push_back(mark_vertices_vbo);

    imp.FreeScene();

    // Загрузка текстур
    QImage day_texture("earth_day.jpg");
    glGenTextures(1, &m_day_map_id);
    glBindTexture(GL_TEXTURE_2D, m_day_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, day_texture.width(), day_texture.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, day_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    QImage night_texture("earth_night.jpg");
    glGenTextures(1, &m_night_map_id);
    glBindTexture(GL_TEXTURE_2D, m_night_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, night_texture.width(), night_texture.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, night_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    QImage clouds_texture("earth_clouds.png");
    glGenTextures(1, &m_clouds_map_id);
    glBindTexture(GL_TEXTURE_2D, m_clouds_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, clouds_texture.width(), clouds_texture.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, clouds_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    QImage normal_texture("earth_normal.tif");
    glGenTextures(1, &m_normal_map_id);
    glBindTexture(GL_TEXTURE_2D, m_normal_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, normal_texture.width(), normal_texture.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, normal_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    QImage specular_texture("earth_specular.jpg");
    glGenTextures(1, &m_specular_map_id);
    glBindTexture(GL_TEXTURE_2D, m_specular_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, specular_texture.width(), specular_texture.height(), 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, specular_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    QImage space_texture("space.jpg");
    glGenTextures(1, &m_space_map_id);
    glBindTexture(GL_TEXTURE_2D, m_space_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, space_texture.width(), space_texture.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, space_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    QImage moon_texture("moon.jpg");
    glGenTextures(1, &m_moon_map_id);
    glBindTexture(GL_TEXTURE_2D, m_moon_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, moon_texture.width(), moon_texture.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, moon_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    QImage moon_normal_texture("moon_normal.jpg");
    glGenTextures(1, &m_moon_normal_map_id);
    glBindTexture(GL_TEXTURE_2D, m_moon_normal_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, moon_normal_texture.width(), moon_normal_texture.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, moon_normal_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    QImage moon_specular_texture("moon_specular.jpg");
    glGenTextures(1, &m_moon_specular_map_id);
    glBindTexture(GL_TEXTURE_2D, m_moon_specular_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, moon_specular_texture.width(), moon_specular_texture.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, moon_specular_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    QImage sun_texture("sun.jpg");
    glGenTextures(1, &m_sun_map_id);
    glBindTexture(GL_TEXTURE_2D, m_sun_map_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sun_texture.width(), sun_texture.height(), 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, sun_texture.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    updateViewUniforms();
    updateProjUniforms();

    // Завод таймера отрисовки
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, &Visualizer::draw);
    timer->start(1000 / FPS);

    // Начальные значения и просчет матрицы Земли
    m_earth_model_mat.setToIdentity();
    glUseProgram(m_earth_program_id);
    glUniformMatrix4fv(m_earth_model_uni_id, 1, false, m_earth_model_mat.data());

    // Начальные значения и просчет матриц других тел
    setMoonPosition(QVector3D(-30.168f, 0.0f, 0.0f));
    setMoonRotation(QVector3D(0.0f, 0.0f, 0.0f));
    setSunPosition(QVector3D(11740.7f, 0.0f, 0.0f));
    setCameraTarget(QVector3D(0.0f, 0.0f, 0.0f));

    m_is_init = true;
}

void Visualizer::updateSunUniforms()
{
    m_sun_model_mat.setToIdentity();
    m_sun_model_mat.translate(m_sun_position);
    m_sun_model_mat.scale(m_sun_scale);

    glUseProgram(m_sun_program_id);
    glUniformMatrix4fv(m_sun_model_uni_id, 1, false, m_sun_model_mat.data());
    glUseProgram(m_earth_program_id);
    glUniform3f(m_earth_sun_pos_uni_id, m_sun_position.x(), m_sun_position.y(), m_sun_position.z());
    glUseProgram(m_moon_program_id);
    glUniform3f(m_moon_sun_pos_uni_id, m_sun_position.x(), m_sun_position.y(), m_sun_position.z());
}

void Visualizer::updateMoonUniforms()
{
    m_moon_model_mat.setToIdentity();
    m_moon_model_mat.translate(m_moon_position);
    m_moon_model_mat.rotate(QQuaternion::fromEulerAngles(m_moon_rotation));
    m_moon_model_mat.scale(m_moon_scale);

    glUseProgram(m_moon_program_id);
    glUniformMatrix4fv(m_moon_model_uni_id, 1, false, m_moon_model_mat.data());
}

void Visualizer::updateViewUniforms()
{
    m_view_mat.setToIdentity();
    m_view_mat.lookAt(m_camera_target + (m_camera_direction * m_zoom), m_camera_target, QVector3D(0.0f, 1.0f, 0.0f));

    glUseProgram(m_earth_program_id);
    glUniformMatrix4fv(m_earth_view_uni_id, 1, false, m_view_mat.data());
    glUniform3f(m_earth_cam_pos_uni_id, m_camera_target.x() + m_camera_direction.x() * m_zoom,
                                        m_camera_target.y() + m_camera_direction.y() * m_zoom,
                                        m_camera_target.z() + m_camera_direction.z() * m_zoom);
    glUseProgram(m_space_program_id);
    glUniformMatrix4fv(m_space_view_uni_id, 1, false, m_view_mat.data());
    glUseProgram(m_moon_program_id);
    glUniformMatrix4fv(m_moon_view_uni_id, 1, false, m_view_mat.data());
    glUniform3f(m_moon_cam_pos_uni_id, m_camera_target.x() + m_camera_direction.x() * m_zoom,
                                       m_camera_target.y() + m_camera_direction.y() * m_zoom,
                                       m_camera_target.z() + m_camera_direction.z() * m_zoom);
    glUseProgram(m_sun_program_id);
    glUniformMatrix4fv(m_sun_view_uni_id, 1, false, m_view_mat.data());
    glUseProgram(m_orb_program_id);
    glUniformMatrix4fv(m_orb_view_uni_id, 1, false, m_view_mat.data());
    glUseProgram(m_mark_program_id);
    glUniformMatrix4fv(m_mark_view_uni_id, 1, false, m_view_mat.data());
}

void Visualizer::updateProjUniforms()
{
    glViewport(0, 0, width(), height());

    m_proj_mat.setToIdentity();
    m_proj_mat.perspective(45.0f, (float)width() / (float)height(), 0.01f, 15000.0f);

    glUseProgram(m_earth_program_id);
    glUniformMatrix4fv(m_earth_proj_uni_id, 1, false, m_proj_mat.data());
    glUseProgram(m_space_program_id);
    glUniformMatrix4fv(m_space_proj_uni_id, 1, false, m_proj_mat.data());
    glUseProgram(m_moon_program_id);
    glUniformMatrix4fv(m_moon_proj_uni_id, 1, false, m_proj_mat.data());
    glUseProgram(m_sun_program_id);
    glUniformMatrix4fv(m_sun_proj_uni_id, 1, false, m_proj_mat.data());
    glUseProgram(m_orb_program_id);
    glUniformMatrix4fv(m_orb_proj_uni_id, 1, false, m_proj_mat.data());
    glUseProgram(m_mark_program_id);
    glUniformMatrix4fv(m_mark_proj_uni_id, 1, false, m_proj_mat.data());
}

void Visualizer::draw()
{
    // Замер delta
    long int current_time = MILLS;
    m_delta_time = (current_time - m_last_time) / 1000.0;
    m_last_time = current_time;

    render();
}

void Visualizer::mousePressEvent(QMouseEvent *ev)
{
    if(ev->button() == 1)
    {
        m_button_down = true;
        m_drag_begin = QVector2D(ev->x(), ev->y());
    }
}

void Visualizer::mouseReleaseEvent(QMouseEvent *ev)
{
    if(ev->button() == 1)
        m_button_down = false;
}

void Visualizer::mouseMoveEvent(QMouseEvent *ev)
{
    if(!m_button_down)
        return;

    float x_diff = (ev->x() - m_drag_begin.x()) * MOUSE_SENS_X;
    float y_diff = (ev->y() - m_drag_begin.y()) * MOUSE_SENS_Y;

    if(m_last_angle_y + y_diff > 85.0f || m_last_angle_y + y_diff < -85.0f)
        y_diff = 0.0f;

    m_last_angle_y += y_diff;

    QMatrix4x4 camera_transform;
    camera_transform.rotate(y_diff, QVector3D::crossProduct(m_camera_direction, QVector3D(0.0, 1.0, 0.0)));
    camera_transform.rotate(x_diff, QVector3D(0.0, 1.0, 0.0));
    m_camera_direction = camera_transform * m_camera_direction;

    // Пересчет матрицы и обновление юниформ
    updateViewUniforms();

    m_drag_begin = QVector2D(ev->x(), ev->y());
}

void Visualizer::wheelEvent(QWheelEvent *ev)
{
    // Приблежение камеры
    m_zoom -= ev->angleDelta().y() * 0.001f;
    m_zoom = qBound(0.02f, m_zoom, 12.0f);

    // Пересчет матрицы и обновление юниформ
    updateViewUniforms();
}

void Visualizer::resizeEvent(QResizeEvent* ev)
{
    if(!m_is_init)
        return;

    // Пересчет матрицы и обновление юниформ
    updateProjUniforms();
}


