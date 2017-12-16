#include "../include/engine_types.hpp"
#include <cmath>

namespace ge
{
    matrix3::matrix3()
    {
    }
    matrix3::matrix3(const std::vector<vec3>& init_vec) : data(init_vec)
    {
    }
    matrix3::matrix3(const matrix3& mat) : data(mat.data)
    {
    }

    matrix3 matrix3::indentity()
    {
        std::vector<vec3> init_vec = { vec3(1.f, 0.f, 0.f),
                                       vec3(0.f, 1.f, 0.f),
                                       vec3(0.f, 0.f, 1.f) };
        return matrix3(init_vec);
    }

    matrix3 matrix3::scale(float scale)
    {
        matrix3 scaling;
        scaling.data[0].x = scale;
        scaling.data[1].y = scale;
        return scaling;
    }

    matrix3 matrix3::rotation(float thetha)
    {
        matrix3 rotation   = matrix3::indentity();
        rotation.data[0].x = std::cos(thetha);
        rotation.data[0].y = -std::sin(thetha);
        rotation.data[1].x = std::sin(thetha);
        rotation.data[1].y = std::cos(thetha);
        return rotation;
    }

    matrix3 matrix3::transport(float delta)
    {
        matrix3 transport   = matrix3::indentity();
        transport.data[2].x = delta;
        transport.data[2].y = delta;
        return transport;
    }
}
