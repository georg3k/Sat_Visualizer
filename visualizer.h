#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <QWindow>
#include <chrono>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVector3D>
#include <QMatrix4x4>
#include <QImage>
#include <QOpenGLFunctions_3_3_Core>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>

#include <cmath>

class Visualizer : public QWindow, protected QOpenGLFunctions_3_3_Core
{
    public:
        Visualizer();
        ~Visualizer();
        virtual void render();
        void exposeEvent(QExposeEvent *event);

    protected:
        void mousePressEvent(QMouseEvent *ev);
        void mouseReleaseEvent(QMouseEvent *ev);
        void mouseMoveEvent(QMouseEvent *ev);
        void wheelEvent(QWheelEvent* ev);
        void resizeEvent(QResizeEvent* ev);

    private:
        void init();

        // Общие параметры GL и виджета
        QOpenGLContext *m_gl_context;
        QSurfaceFormat *m_gl_format;
        bool m_is_init = false;
        std::vector<GLuint> m_buffers;

        // Данные сферы
        GLuint m_sphere_vao_id;
        GLint m_sphere_indices_count;

        // Данные эллипса орбиты
        GLuint m_orb_vao_id;
        GLint m_orb_indices_count;

        // Данные метки
        GLuint m_mark_vao_id;

        // Данные шейдера Земли
        GLuint m_earth_program_id;
        GLint m_earth_model_uni_id;
        GLint m_earth_view_uni_id;
        GLint m_earth_proj_uni_id;
        GLint m_earth_cam_pos_uni_id;

        // Данные шейдера фона
        GLuint m_space_program_id;
        GLint m_space_view_uni_id;
        GLint m_space_proj_uni_id;

        // Данные шейдера Луны
        GLuint m_moon_program_id;
        GLint m_moon_model_uni_id;
        GLint m_moon_view_uni_id;
        GLint m_moon_proj_uni_id;

        // Данные шейдера Солнца
        GLuint m_sun_program_id;
        GLint m_sun_model_uni_id;
        GLint m_sun_view_uni_id;
        GLint m_sun_proj_uni_id;

        // Данные шейдера орбит
        GLuint m_orb_program_id;
        GLint m_orb_model_uni_id;
        GLint m_orb_view_uni_id;
        GLint m_orb_proj_uni_id;
        GLint m_orb_targ_uni_id;
        GLint m_orb_col_uni_id;

        // Данные шейдера меток
        GLuint m_mark_program_id;
        GLint m_mark_model_uni_id;
        GLint m_mark_view_uni_id;
        GLint m_mark_proj_uni_id;
        GLint m_mark_color_uni_id;

        // Текстуры
        GLuint m_day_map_id;
        GLuint m_night_map_id;
        GLuint m_normal_map_id;
        GLuint m_space_map_id;
        GLuint m_moon_map_id;
        GLuint m_sun_map_id;

        // Цикл обновления
        long int last_time = clock();
        double delta_time;

        // Матрицы отрисовки
        QMatrix4x4 m_sphere_model_mat;
        QMatrix4x4 m_sun_model_mat;
        QMatrix4x4 m_moon_model_mat;
        QMatrix4x4 m_orb_model_mat;
        QMatrix4x4 m_mark_model_mat;
        QMatrix4x4 m_view_mat;
        QMatrix4x4 m_proj_mat;

        // Данные камеры
        QVector3D m_camera_target;
        QVector3D m_camera_direction;

        // Управление камерой
        bool m_button_down = false;
        QVector2D m_drag_begin;
        float m_zoom = 3.0f;
        float m_last_angle_y = 0.0f;

    public slots:
        virtual void draw();
};

#endif
