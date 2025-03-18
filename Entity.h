enum AnimationDirection { LEFT, RIGHT, UP, DOWN };
enum EntityType { PLAYER, PLATFORM, };

class Entity
{
private:

    // ————— PHYSICS ————— //
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::vec3 m_acceleration;

    // ————— TRANSFORMATIONS ————— //
    glm::vec3 m_movement;
	glm::vec3 m_scale;
    float     m_speed;

    glm::mat4 m_model_matrix;

    // ————— TEXTURES ————— //
    GLuint    m_texture_id;
    EntityType m_entity_type;

    // ————— ANIMATION ————— //
    int m_animation_cols;
    int m_animation_frames,
        m_animation_index,
        m_animation_rows;

    int* m_animation_indices = nullptr;
    float m_animation_time = 0.0f;

    float m_width = 1.0f;
    float m_height = 1.0f;

public:
    // ————— STATIC VARIABLES ————— //
    static constexpr int    SECONDS_PER_FRAME = 4;
    static constexpr int    LEFT = 0,
        RIGHT = 1,
        UP = 2,
        DOWN = 3;


    // ————— PHYSICS ————— //
    bool m_is_active = true;
    bool m_is_flying = true;
    bool m_collided_top = false;
    bool m_collided_bottom = false;
    bool m_collided_left = false;
    bool m_collided_right = false;

    // ————— METHODS ————— //
    Entity();
    ~Entity();

    void draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index);
    void update(float delta_time, Entity* collidable_entities, int collidable_entity_count);
    void render(ShaderProgram* program);

    bool const check_collision(Entity* other) const;
    void const check_collision_y(Entity* collidable_entities, int collidable_entity_count);
    void const check_collision_x(Entity* collidable_entities, int collidable_entity_count);

    void move_left()    { m_movement.x = -1.0f; };
    void move_right()   { m_movement.x = 1.0f; };
    void move_up()      { m_movement.y = 1.0f; };
    void move_down()    { m_movement.y = -1.0f; };

    // ————— GETTERS ————— //
	glm::vec3 const get_scale()         const { return m_scale; }
    glm::vec3 const get_position()      const { return m_position; }
    glm::vec3 const get_movement()      const { return m_movement; }
    glm::vec3 const get_velocity()      const { return m_velocity; }
    glm::vec3 const get_acceleration()  const { return m_acceleration; }
    float   const get_speed()           const { return m_speed; }
    float   const get_width()           const { return m_width; }
    float   const get_height()          const { return m_height; }
    GLuint  const get_texture_id()      const { return m_texture_id; }
    EntityType const get_entity_type()  const { return m_entity_type; }

    // ————— SETTERS ————— //
	void const set_scale(glm::vec3 new_scale)                   { m_scale = new_scale; }
    void const set_position(glm::vec3 new_position)             { m_position = new_position; }
    void const set_movement(glm::vec3 new_movement)             { m_movement = new_movement; }
    void const set_velocity(glm::vec3 new_velocity)             { m_velocity = new_velocity; }
    void const set_acceleration(glm::vec3 new_acceleration)     { m_acceleration = new_acceleration; }
    void const set_speed(float new_speed)                       { m_speed = new_speed; }
    void const set_width(float new_width)                       { m_width = new_width; }
    void const set_height(float new_height)                     { m_height = new_height; }
    void const set_texture_id(GLuint new_texture_id)            { m_texture_id = new_texture_id; }
    EntityType const set_entity_type(EntityType new_entity_type){ m_entity_type = new_entity_type; }
};
