#ifndef RENDERERS2D_H_INCLUDED
#define RENDERERS2D_H_INCLUDED

#include <utility>

#include "graphics.h"
#include "mat.h"
#include "reflection.h"
#include "template_utils.h"

namespace Renderers
{
    ReflectStruct(Attributes, (
        (fvec2)(pos),
        (fvec4)(color),
        (fvec2)(texture_pos),
        (fvec3)(factors),
    ))

    ReflectStruct(Uniforms, (
        (Graphics::Shader::VertexUniform<fmat4>)(matrix),
        (Graphics::Shader::VertexUniform<fvec2>)(texture_size),
        (Graphics::Shader::FragmentUniform<Graphics::Texture>)(texture),
        (Graphics::Shader::FragmentUniform<fmat4>)(color_matrix),
    ))

    class Poly2D
    {
        Graphics::Shader shader;
        Graphics::RenderQueue<Attributes, Graphics::triangles> queue;
        Uniforms uni;

        class Quad_t : TemplateUtils::MoveFunc<Quad_t>
        {
            using ref = Quad_t &&;

            // The constructor sets those:
            decltype(Poly2D::queue) *queue;
            fvec2 m_pos, m_size;

            bool has_texture = 0;
            fvec2 m_tex_pos = fvec2(0), m_tex_size = fvec2(0);

            bool has_center = 0;
            fvec2 m_center = fvec2(0);
            bool m_center_pos_tex = 0;

            bool has_matrix = 0;
            fmat3 m_matrix = fmat3::identity();

            bool has_color = 0;
            fvec3 m_colors[4] {};

            bool has_tex_color_fac = 0;
            float m_tex_color_factors[4] = {1,1,1,1};

            float m_alpha[4] = {1,1,1,1};
            float m_beta[4] = {1,1,1,1};

            bool m_abs_pos = 0;
            bool m_abs_tex_pos = 0;

            bool m_flip_x = 0, m_flip_y = 0;

            static void OnMove(Quad_t &&from, Quad_t &/*to*/)
            {
                from.queue = 0;
            }
          public:
            Quad_t(decltype(Poly2D::queue) *queue, fvec2 pos, fvec2 size) : queue(queue), m_pos(pos), m_size(size) {}

            Quad_t(const Quad_t &) = delete;
            Quad_t &operator=(const Quad_t &) = delete;

            Quad_t(Quad_t &&) = default;
            Quad_t &operator=(Quad_t &&) = default;

            ~Quad_t()
            {
                if (!queue)
                    return;

                DebugAssert("2D poly renderer: Quad with no texture nor color specified.", has_texture || has_color);
                DebugAssert("2D poly renderer: Quad with absolute corner coodinates with a center specified.", m_abs_pos + has_center < 2);
                DebugAssert("2D poly renderer: Quad with absolute texture coordinates mode but no texture coordinates specified.", m_abs_tex_pos <= has_texture);
                DebugAssert("2D poly renderer: Quad with texture and color, but without a mixing factor.", (has_texture && has_color) == has_tex_color_fac);
                DebugAssert("2D poly renderer: Quad with a matrix but without a center specified.", has_matrix <= has_center);
                DebugAssert("2D poly renderer: Quad with a center position specified in texels but without a texture.", m_center_pos_tex <= has_texture);

                if (m_abs_pos)
                    m_size -= m_pos;
                if (m_abs_tex_pos)
                    m_tex_size -= m_tex_pos;

                Attributes out[4];

                if (has_texture)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        out[i].color = m_colors[i].to_vec4(0);
                        out[i].factors.x = m_tex_color_factors[i];
                        out[i].factors.y = m_alpha[i];
                    }

                    if (m_center_pos_tex)
                    {
                        if (m_tex_size.x)
                            m_center.x *= m_size.x / m_tex_size.x;
                        if (m_tex_size.y)
                            m_center.y *= m_size.y / m_tex_size.y;
                    }
                }
                else
                {
                    for (int i = 0; i < 4; i++)
                    {
                        out[i].color = m_colors[i].to_vec4(m_alpha[i]);
                        out[i].factors.x = out[i].factors.y = 0;
                    }
                }

                for (int i = 0; i < 4; i++)
                    out[i].factors.z = m_beta[i];

                if (m_flip_x)
                {
                    m_tex_pos.x += m_tex_size.x;
                    m_tex_size.x = -m_tex_size.x;
                    if (has_center)
                        m_center.x = m_size.x - m_center.x;
                }
                if (m_flip_y)
                {
                    m_tex_pos.y += m_tex_size.y;
                    m_tex_size.y = -m_tex_size.y;
                    if (has_center)
                        m_center.y = m_size.y - m_center.y;
                }

                out[0].pos = -m_center;
                out[2].pos = m_size - m_center;
                out[1].pos = {out[0].pos.x, out[2].pos.y};
                out[3].pos = {out[2].pos.x, out[0].pos.y};

                if (has_matrix)
                {
                    for (auto &it : out)
                        it.pos = m_pos + (m_matrix /mul/ it.pos.to_vec3(1)).to_vec2();
                }
                else
                {
                    for (auto &it : out)
                        it.pos += m_pos;
                }

                out[0].texture_pos = m_tex_pos;
                out[2].texture_pos = m_tex_pos + m_tex_size;
                out[1].texture_pos = {out[0].texture_pos.x, out[2].texture_pos.y};
                out[3].texture_pos = {out[2].texture_pos.x, out[0].texture_pos.y};

                queue->Quad(out[0], out[1], out[2], out[3]);
            }

            ref tex(ivec2 pos, ivec2 size)
            {
                tex_f(pos, size);
                return (ref)*this;
            }
            ref tex(ivec2 pos)
            {
                tex_f(pos);
                return (ref)*this;
            }
            ref tex_f(fvec2 pos, fvec2 size)
            {
                DebugAssert("2D poly renderer: Quad_t texture specified twice.", !has_texture);
                has_texture = 1;

                m_tex_pos = pos;
                m_tex_size = size;
                return (ref)*this;
            }
            ref tex_f(fvec2 pos)
            {
                tex_f(pos, m_size);
                return (ref)*this;
            }
            ref center(fvec2 c)
            {
                DebugAssert("2D poly renderer: Quad_t center specified twice.", !has_center);
                has_center = 1;

                m_center = c;
                m_center_pos_tex = 1;
                return (ref)*this;
            }
            ref pixel_center(fvec2 c) // Same as `center()`, but the coordinates are always measured in pixels instead of texels even if a texture is specified.
            {
                DebugAssert("2D poly renderer: Quad_t center specified twice.", !has_center);
                has_center = 1;

                m_center = c;
                m_center_pos_tex = 0;
                return (ref)*this;
            }
            ref center()
            {
                pixel_center(m_size / 2);
                return (ref)*this;
            }
            ref matrix(fmat3 m) // This can be called multiple times, resulting in multiplying matrices in the order they were passed.
            {
                if (has_matrix)
                    m_matrix = m_matrix /mul/ m;
                else
                {
                    has_matrix = 1;
                    m_matrix = m;
                }
                return (ref)*this;
            }
            ref rotate(float a) // Uses `matrix()`.
            {
                matrix(fmat3::rotate2D(a));
                return (ref)*this;
            }
            ref translate(ivec2 v) // Uses a matrix.
            {
                translate_f(v);
                return (ref)*this;
            }
            ref translate_f(fvec2 v) // Uses a matrix.
            {
                matrix(fmat3::translate2D(v));
                return (ref)*this;
            }
            ref scale(ivec2 s) // Uses a matrix.
            {
                scale_f(s);
                return (ref)*this;
            }
            ref scale(int s) // Uses a matrix.
            {
                scale_f(s);
                return (ref)*this;
            }
            ref scale_f(fvec2 s) // Uses a matrix.
            {
                matrix(fmat3::scale(s));
                return (ref)*this;
            }
            ref scale_f(float s) // Uses a matrix.
            {
                scale_f(fvec2(s));
                return (ref)*this;
            }
            ref color(fvec3 c)
            {
                DebugAssert("2D poly renderer: Quad_t color specified twice.", !has_color);
                has_color = 1;

                for (auto &it : m_colors)
                    it = c;
                return (ref)*this;
            }
            ref color(fvec3 a, fvec3 b, fvec3 c, fvec3 d)
            {
                DebugAssert("2D poly renderer: Quad_t color specified twice.", !has_color);
                has_color = 1;

                m_colors[0] = a;
                m_colors[1] = b;
                m_colors[2] = c;
                m_colors[3] = d;
                return (ref)*this;
            }
            ref mix(float x) // 0 - fill with color, 1 - use texture
            {
                DebugAssert("2D poly renderer: Quad_t texture/color factor specified twice.", !has_tex_color_fac);
                has_tex_color_fac = 1;

                for (auto &it : m_tex_color_factors)
                    it = x;
                return (ref)*this;
            }
            ref mix(float a, float b, float c, float d)
            {
                DebugAssert("2D poly renderer: Quad_t texture/color factor specified twice.", !has_tex_color_fac);
                has_tex_color_fac = 1;

                m_tex_color_factors[0] = a;
                m_tex_color_factors[1] = b;
                m_tex_color_factors[2] = c;
                m_tex_color_factors[3] = d;
                return (ref)*this;
            }
            ref alpha(float a)
            {
                for (auto &it : m_alpha)
                    it = a;
                return (ref)*this;
            }
            ref alpha(float a, float b, float c, float d)
            {
                m_alpha[0] = a;
                m_alpha[1] = b;
                m_alpha[2] = c;
                m_alpha[3] = d;
                return (ref)*this;
            }
            ref beta(float a) // 1 - normal blending, 0 - additive blending
            {
                for (auto &it : m_beta)
                    it = a;
                return (ref)*this;
            }
            ref beta(float a, float b, float c, float d)
            {
                m_beta[0] = a;
                m_beta[1] = b;
                m_beta[2] = c;
                m_beta[3] = d;
                return (ref)*this;
            }
            ref absolute(bool x = 1) // Interpret size as a position of the second corner
            {
                m_abs_pos = x;
                return (ref)*this;
            }
            ref absolute_tex(bool x = 1) // Interpret texture size as a position of the second corner
            {
                m_abs_tex_pos = x;
                return (ref)*this;
            }
            ref flip_x(bool f = 1) // Flips texture horizontally if it was specified. Updates the center accordingly if it was specified.
            {
                m_flip_x = f;
                return (ref)*this;
            }
            ref flip_y(bool f = 1) // Flips texture vertically if it was specified. Updates the center accordingly if it was specified.
            {
                m_flip_y = f;
                return (ref)*this;
            }
        };
      public:
        Poly2D() {}
        Poly2D(int size, const Graphics::Shader::Config &cfg = {})
        {
            Create(size, cfg);
        }
        Poly2D(int size, const std::string &v_src, const std::string &f_src, const Graphics::Shader::Config &cfg = {})
        {
            Create(size, v_src, f_src, cfg);
        }
        void Create(int size, const Graphics::Shader::Config &cfg = {})
        {
            constexpr const char *v = R"(
VARYING( vec4 , color       )
VARYING( vec2 , texture_pos )
VARYING( vec3 , factors     )
void main()
{
    gl_Position = u_matrix * vec4(a_pos, 0, 1);
    v_color       = a_color;
    v_texture_pos = a_texture_pos / u_texture_size;
    v_factors     = a_factors;
})";
            constexpr const char *f = R"(
VARYING( vec4 , color       )
VARYING( vec2 , texture_pos )
VARYING( vec3 , factors     )
void main()
{
    vec4 tex_color = texture2D(u_texture, v_texture_pos);
    gl_FragColor = vec4(v_color.rgb * (1. - v_factors.x) + tex_color.rgb * v_factors.x,
                        v_color.a   * (1. - v_factors.y) + tex_color.a   * v_factors.y);
    vec4 result = u_color_matrix * vec4(gl_FragColor.rgb, 1);
    gl_FragColor.a *= result.a;
    gl_FragColor.rgb = result.rgb * gl_FragColor.a;
    gl_FragColor.a *= v_factors.z;
})";
            Create(size, v, f, cfg);
        }
        void Create(int size, const std::string &v_src, const std::string &f_src, const Graphics::Shader::Config &cfg = {}) // With custom shader.
        {
            decltype(shader) new_shader;
            new_shader.Create<Attributes>(v_src, f_src, &uni, cfg);
            decltype(queue) new_queue(size);
            shader = std::move(new_shader);
            queue  = std::move(new_queue);

            SetMatrix(fmat4::identity());
            ResetColorMatrix();
        }
        void Destroy()
        {
            shader.Destroy();
            queue.Destroy();
        }

        void Finish() // Binds the shader.
        {
            shader.Bind();
            queue.Flush();
        }

        void SetMatrix(fmat4 m) // Binds the shader, flushes the queue.
        {
            Finish();
            uni.matrix = m;
        }

        // final_color = (color_matrix * vec4(color.rgb,1)) * vec4(1,1,1,color.a)
        void SetColorMatrix(fmat4 m) // Binds the shader, flushes the queue.
        {
            Finish();
            uni.color_matrix = m;
        }
        void ResetColorMatrix() // Binds the shader, flushes the queue.
        {
            Finish();
            uni.color_matrix = fmat4::identity();
        }

        void SetTexture(const Graphics::Texture &texture) // Binds the shader, flushes the queue.
        {
            Finish();
            uni.texture = texture;
            uni.texture_size = texture.Size();
        }
        void SetTexture(const Graphics::Texture &&) = delete;

        Quad_t Quad(ivec2 pos, ivec2 size)
        {
            return {&queue, pos, size};
        }
        Quad_t Quad_f(fvec2 pos, fvec2 size)
        {
            return {&queue, pos, size};
        }
    };
}

#endif
