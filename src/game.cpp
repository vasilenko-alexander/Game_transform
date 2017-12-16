#include "../include/engine.hpp"
#include "../include/engine_constants.hpp"
#include "../include/engine_types.hpp"
#include <fstream>
#include <iostream>
#include <cmath>

int main(int /*argn*/, char* /*args*/ [])
{
    ge::IEngine* gameEngine = ge::getInstance();
    std::string errMsg      = gameEngine->init_engine(ge::everything);

    if (!errMsg.empty())
    {
        std::cerr << errMsg << std::endl;
        return EXIT_FAILURE;
    }

    const std::string text_path = "./textures/texture.png";
    gameEngine->draw_texture(text_path);

    bool run_loop = true;
    ge::event event;
    while (run_loop)
    {
        while (gameEngine->read_event(event))
        {
            if (!event.msg.empty())
            {
                std::cout << event.msg << std::endl;
            }
            if (event.type == ge::events_t::shutdown)
            {
                run_loop = false;
                break;
            }
        }
        const std::string vertexes_path = "./config/texture_vertexes.txt";

        std::ifstream file(vertexes_path);
        if (!file.is_open())
        {
            std::cerr << "Can't open file with vertexes values" << std::endl;
            continue;
        }

        ge::texture txLeft, txRight;

        float time    = gameEngine->get_time();
        float sin_val = std::sin(time);

        ge::matrix3 move   = ge::matrix3::transport(sin_val / 2);
        ge::matrix3 rotate = ge::matrix3::rotation(time);
        ge::matrix3 scale  = ge::matrix3::scale(sin_val);

        file >> txRight >> txLeft;

        for (size_t v_i = 0, max_i = txRight.coords.size(); v_i < max_i; ++v_i)
        {
            txRight.coords[v_i] = txRight.coords[v_i] * rotate * scale * move;
            txLeft.coords[v_i]  = txLeft.coords[v_i] * rotate * scale * move;
        }

        gameEngine->render(txLeft);
        gameEngine->render(txRight);

        gameEngine->swap_buffers();
    }
    gameEngine->uninit_engine();
    return EXIT_SUCCESS;
}
