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
	float z_relative_threshold = 0.0f;
	Collision::AABB *obstacle_box = nullptr;

	// slide control
	bool sliding = false;
	float slide_duration = 1.2f;
	float slide_duration_reset = 1.2f;
	float friction = 17.0f;
	float slide_velocity = 0.0f;


	// climbing control
	bool climbing = false;
	float climb_speed = 10.0f;

	// camera control
	void update_camera();
	float camera_dist = 3.0f;
	float camera_max_dist = 5.0f;
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
	float player_height_default;

	// primitives info
	std::vector<Collision::AABB> obstacles;

	// coordinates of messages. 
	std::vector<std::pair< glm::vec3, std::string>> messages;
	int idx_message = -1; // keep an index of your location, so that you don't keep playing the same message over and over

};
