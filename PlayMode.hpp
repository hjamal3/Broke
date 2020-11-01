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

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, jump, slide;

	// jumping control
	bool in_air = false;
	bool on_platform = false;
	float gravity = 30.0f;
	float jump_up_velocity = 0.0f;
	float z_relative = 0.0f;
	float z_relative_threshold = 0.0f;
	Collision::AABB *obstacle_box = nullptr;

	// slide control
	bool sliding = false;
	float slide_duration = 1.5f;
	float slide_acc = 0.2f;
	float slide_velocity = 0.0f;

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

	// primitives info
	std::vector<Collision::AABB> obstacles;

};
