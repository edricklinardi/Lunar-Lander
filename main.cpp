/**
* Author: Edrick Linardi
* Assignment: Lunar Lander
* Date due: 2025-3-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "Entity.h"
#include <vector>
#include <cstdlib>
#include <ctime>

// ————— STRUCTS AND ENUMS ————— //
struct GameState
{
    Entity* lander;
    Entity* platform;
    Entity* lava;
    Entity* background;
};

enum AppStatus { RUNNING, TERMINATED };

// ————— CONSTANTS ————— //

constexpr int WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
LEVEL_OF_DETAIL = 0, // mipmap reduction image level
TEXTURE_BORDER = 0; // this value MUST be zero

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr int FONTBANK_SIZE = 16;

constexpr int PLATFORM_COUNT = 5;
constexpr int LAVA_COUNT = 3;

GameState g_game_state;
SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
constexpr float GRAVITY = -1.0f;
constexpr float MILLISECONDS_IN_SECOND = 1000.0f;

// ————— TEXTURES ————— //
GLuint background_texture_id;
GLuint font_texture_id;
constexpr char  background_sprite_filepath[] = "assets/background.png",
                font_sprite_filepath[] = "assets/font_sprite.png",
                lander_sprite_filepath[] = "assets/steve.png",
                platform_sprite_filepath[] = "assets/grass.png",
                lava_sprite_filepath[] = "assets/lava.png";

// ————— TIMING ————— //
float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;

// ————— MATRICES & VECTORS ————— //
glm::mat4 g_view_matrix, g_projection_matrix;
glm::mat4 g_background_matrix;
glm::vec3 g_initial_lander_position = glm::vec3(0.0f, 3.0f, 0.0f);

// ————— OTHER VARIABLES ————— //
float fuel = 150.0f;
bool isRunning = true;
bool gameOver = false;
bool gameWin = false;
bool gameLose = false;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
        texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void initialise()
{
    // Initialise video
    SDL_Init(SDL_INIT_VIDEO);
    srand(time(0));

    g_display_window = SDL_CreateWindow("Lunar Lander",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	font_texture_id = load_texture(font_sprite_filepath);

	// ————— BACKGROUND ————— //
	g_game_state.background = new Entity();
	g_game_state.background->set_texture_id(load_texture(background_sprite_filepath));
	g_game_state.background->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
	g_game_state.background->set_scale(glm::vec3(10.0f, 7.5f, 1.0f));

    // ————— PLATFORM ————— //
    g_game_state.platform = new Entity[PLATFORM_COUNT];
	GLuint platform_texture = load_texture(platform_sprite_filepath);
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        float x = ((rand() % 10) - 5.0f);  // Random x between -5 and 5
        float y = -3.0f + (rand() % 3) * 0.5f; // Random y, near the ground

        g_game_state.platform[i].set_texture_id(platform_texture);
        g_game_state.platform[i].set_position(glm::vec3(x, y, 0.0f));
        g_game_state.platform[i].set_width(0.5f);
        g_game_state.platform[i].set_height(0.5f);
		g_game_state.platform[i].set_scale(glm::vec3(0.5f, 0.5f, 1.0f));
    }

	// ————— LAVA ————— //
	g_game_state.lava = new Entity[LAVA_COUNT];
    GLuint lava_texture = load_texture(lava_sprite_filepath);
    for (int i = 0; i < LAVA_COUNT; i++) {
        float x = ((rand() % 10) - 5.0f);  // Random x between -5 and 5
        float y = -3.5f + (rand() % 3) * 0.5f; // Random y, near the bottom

        g_game_state.lava[i].set_texture_id(lava_texture);
        g_game_state.lava[i].set_position(glm::vec3(x, y, 0.0f));
        g_game_state.lava[i].set_width(0.5f);
        g_game_state.lava[i].set_height(0.5f);
		g_game_state.lava[i].set_scale(glm::vec3(0.5f, 0.5f, 1.0f));
    }

	// ————— PLAYER ————— //
	g_game_state.lander = new Entity();
	g_game_state.lander->set_texture_id(load_texture(lander_sprite_filepath));
	g_game_state.lander->set_position(g_initial_lander_position);
	g_game_state.lander->set_movement(glm::vec3(0.0f, 0.0f, 0.0f));
	g_game_state.lander->set_speed(1.0f);
	g_game_state.lander->set_acceleration(glm::vec3(0.0f, GRAVITY, 0.0f));
	g_game_state.lander->set_width(0.5f);
	g_game_state.lander->set_height(1.0f);
	g_game_state.lander->set_scale(glm::vec3(0.5f, 1.0f, 1.0f));
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_q:
				g_app_status = TERMINATED;
                break;
            case SDLK_r:
                if (gameOver)
                {
                    fuel = 150.0f;
                    gameOver = false;
                    gameWin = false;
                    gameLose = false;
                    g_game_state.lander->set_position(g_initial_lander_position);
                    g_game_state.lander->set_velocity(glm::vec3(0.0f, 0.0f, 0.0f));
                    g_game_state.lander->set_acceleration(glm::vec3(0.0f, GRAVITY, 0.0f));
                }
                break;
            default: break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    // Reset acceleration
    g_game_state.lander->set_acceleration(glm::vec3(0.0f, GRAVITY, 0.0f));

    if (fuel > 0)  // Movement only allowed if fuel is available
    {
        if (key_state[SDL_SCANCODE_LEFT]) 
        {
            g_game_state.lander->set_acceleration(glm::vec3(-2.0f, g_game_state.lander->get_acceleration().y, 0.0f));
            fuel -= 0.1f;
        }
        if (key_state[SDL_SCANCODE_RIGHT]) 
        {
            g_game_state.lander->set_acceleration(glm::vec3(2.0f, g_game_state.lander->get_acceleration().y, 0.0f));
            fuel -= 0.1f;
        }
        if (key_state[SDL_SCANCODE_UP]) 
        {
            g_game_state.lander->set_acceleration(glm::vec3(g_game_state.lander->get_acceleration().x, 2.0f, 0.0f));
            fuel -= 0.1f;
            g_game_state.lander->m_landed = false; // Allow gravity again
        }
    }

    // If fuel runs out, stop movement
    if (fuel <= 0) 
    {
        g_game_state.lander->set_acceleration(glm::vec3(0.0f, GRAVITY, 0.0f));
    }
}

void update()
{
    // ————— DELTA TIME ————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    // ————— FIXED TIMESTEP ————— //
    delta_time += g_time_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        g_game_state.lander->update(FIXED_TIMESTEP, g_game_state.platform, PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }

    g_time_accumulator = delta_time;

    // Could not do check collision because of clipping prevention, checked bool directly instead
    for (int i = 0; i < PLATFORM_COUNT; i++) { // Landed on platform, game win
        if (g_game_state.lander->m_collided_bottom) 
        {
            gameOver = true;
			gameWin = true;
            break;
        }
    }

	for (int i = 0; i < LAVA_COUNT; i++) { // Landed on lava, game lose
        if (g_game_state.lander->check_collision(&g_game_state.lava[i])) 
        {
            gameOver = true;
			gameLose = true;
            break;
        }
    }

    // Update platforms' matrices
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        g_game_state.platform[i].update(delta_time, nullptr, 0);
    }

    // Update lava's matrices
    for (int i = 0; i < LAVA_COUNT; i++) {
        g_game_state.lava[i].update(delta_time, nullptr, 0);
    }

}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

	g_game_state.background->render(&g_shader_program);

    // Draw lava
    for (int i = 0; i < LAVA_COUNT; i++) {
        g_game_state.lava[i].render(&g_shader_program);
    }

    // Draw platforms
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        g_game_state.platform[i].render(&g_shader_program);
    }

	std::string fuel_message = "Fuel: " + std::to_string(int(fuel));
	draw_text(&g_shader_program, font_texture_id, fuel_message, 0.3f, 0.01f, glm::vec3(-4.75f, 3.5f, 0.0f));

	if (gameOver)
	{
		std::string message;
		if (gameWin)
		{
			message = "Mission Accomplished!";
		}
		else if (gameLose)
		{
			message = "Mission Failed!";
		}
		draw_text(&g_shader_program, font_texture_id, message, 0.3f, 0.01f, glm::vec3(-2.5f, 1.0f, 0.0f));
		draw_text(&g_shader_program, font_texture_id, "Press R to restart", 0.3f, 0.01f, glm::vec3(-2.5f, 0.0f, 0.0f));
	}

    // Draw lander
    g_game_state.lander->render(&g_shader_program);

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        if (!gameOver)
        {
			update();
        }
        render();
    }

    shutdown();
    return 0;
}