#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"
#include "Collision.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	void step_in_3D(glm::vec3& pos, glm::quat& rot);
	void step_in_mesh(glm::vec3& remain);
	void draw_textbox(float aspect, glm::vec2& center, glm::vec2& radius);

	void reset_sliding();

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
		// uint8_t clean = 0;
	} left, right, down, up, jump, slide;

	// jumping control
	bool in_air = false;
	bool on_platform = false;
	bool released = true;
	float gravity = 30.0f;
	float jump_up_velocity = 0.0f;
	float max_fall_speed = -12.0f;
	float jump_speed = 13.0f;
	float z_relative = 0.0f;
	Collision::AABB *obstacle_box = nullptr;
	Collision::AABB *platform_box = nullptr;

	// slide control
	bool sliding = false;
	//float slide_duration = 1.2f;
	//float slide_duration_reset = 1.2f;
	float friction = 1.5f;
	//float slide_velocity = 0.0f;

	// player motion
	const float PlayerSpeedMax = 35.0f;
	float PlayerSpeed = 0;
	float speed_multiplier = 0.0f; // 0 to 1
	const float accel = 3.0f;
	const float low_speed = 0.2f;
	int last_collision = 0;

	// climbing control
	bool climbing = false;
	float climb_speed = 10.0f;

	// camera control
	void update_camera();
	float camera_dist = 3.0f;
	float camera_max_dist = 4.0f;
	float camera_min_dist = 1.0f;
	float yaw = -float(M_PI)/2.0f;
	float pitch = 0.25f;
	glm::vec3 look_offset = glm::vec3(0.0f, 0.0f, 0.8f);

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//player info:
	struct Player {
		WalkPoint at;
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;

		// Player contains a bounding box

	} player;

	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};

	Scene::Transform *shadow = nullptr;
	float shadow_base_height;
	float player_height_default;

	// primitives info
	std::vector<Collision::AABB> obstacles;

	// coordinates of messages. 
	std::vector<std::pair< glm::vec3, std::string>> messages;
	int idx_message = -1; // keep an index of your location, so that you don't keep playing the same message over and over

	bool prologue = true;
	int prologue_message = 0;
	std::vector<std::string> prologue_messages;

};
