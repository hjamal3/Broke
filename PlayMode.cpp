#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

GLuint phonebank_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > phonebank_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("phone-bank.pnct"));
	phonebank_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > phonebank_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("phone-bank.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = phonebank_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = phonebank_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

WalkMesh const *walkmesh = nullptr;
Load< WalkMeshes > phonebank_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("phone-bank.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
});


void PlayMode::update_camera() {
	player.camera->transform->position.x = std::cos(yaw) * camera_dist + player.transform->position.x;
	player.camera->transform->position.y = std::sin(yaw) * camera_dist + player.transform->position.y;
	player.camera->transform->position.z = std::sin(pitch) * camera_dist + player.transform->position.z + look_offset.z;

	glm::vec3 up = walkmesh->to_world_smooth_normal(player.at);
	glm::mat4x3 frame = player.camera->transform->make_local_to_world();
	glm::vec3 pos = frame[3];

	glm::mat4 view = glm::lookAt(pos, player.transform->position + look_offset, up);
	player.camera->transform->rotation = glm::conjugate(glm::quat_cast(view));
}

PlayMode::PlayMode() : scene(*phonebank_scene) {
	//create a player transform:
	for (auto& transform : scene.transforms) {
		if (transform.name == "Player") player.transform = &transform;
		if (transform.name == "PlayerShadow") shadow = &transform;
	}

	// go through the meshes and find Cube objects. Convert to naming convention later
	std::string str("o_");
	const auto& meshes = phonebank_meshes->meshes;
	for (auto& mesh : meshes) {

		if (mesh.first.find(str) != std::string::npos)
		{
			auto& min = mesh.second.min;
			auto& max = mesh.second.max;
			glm::vec3 center = 0.5f * (min + max);
			glm::vec3 rad = 0.5f * (max - min);
			obstacles.emplace_back(Collision::AABB(center, rad));
		}
	}

	if (player.transform == nullptr) throw std::runtime_error("GameObject not found.");
	if (shadow == nullptr) throw std::runtime_error("GameObject not found.");

	shadow->position = glm::vec3(player.transform->position.x, player.transform->position.y, shadow->position.z);
	shadow_base_height = shadow->position.z;

	// create some message objects. hardcoded for now
	messages.emplace_back(std::make_pair(glm::vec3(player.transform->position.x, player.transform->position.y, player.transform->position.z), "Press WASD to move, press space to jump. Mouse motion to rotate.")); // starting coord of player
	messages.emplace_back(std::make_pair(glm::vec3(-8.5f, -46.0f, 5.0f), "Hold left shift to crawl. Scroll mousewheel to zoom camera."));

	//create a player camera attached to a child of the player transform:
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	player.camera = &scene.cameras.back();
	player.camera->fovy = glm::radians(60.0f);
	player.camera->near = 0.01f;
	//player.camera->transform->parent = player.transform;

	//player's eyes are 1.8 units above the ground:
	//player.camera->transform->position = glm::vec3(0.0f, -5.0f, 5.0f);

	//start player walking at nearest walk point:
	player.at = walkmesh->nearest_walk_point(player.transform->position);

	//rotate camera facing direction (-z) to player facing direction (+y):
	//player.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	update_camera();

	player_height_default = player.transform->scale.z;

}

PlayMode::~PlayMode() {
}

void PlayMode::reset_sliding() {
	slide.pressed = false;
	sliding = false;
	//slide_duration = slide_duration_reset;
	//slide_velocity = 0.0f;
	player.transform->scale.z = player_height_default;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			jump.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_LSHIFT) {
			if (!slide.pressed) {
				slide.pressed = true;
			}
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			jump.pressed = false;
			released = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_LSHIFT) {
			if (slide.pressed) {
				reset_sliding();
			}
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			glm::vec3 up = walkmesh->to_world_smooth_normal(player.at);

			player.transform->rotation = glm::angleAxis(-motion.x * player.camera->fovy, up) * player.transform->rotation;

			//camera_pitch += 1.5f * motion.y * player.camera->fovy;
			yaw += -motion.x * player.camera->fovy;
			if (yaw < -M_PI) yaw += 2.0f * float(M_PI);
			if (yaw >= M_PI) yaw -= 2.0f * float(M_PI);

			//float pitch = glm::pitch(player.camera->transform->rotation);
			pitch -= motion.y * player.camera->fovy;
			if (pitch < -M_PI / 2) pitch = float(-M_PI) / 2;
			if (pitch > M_PI / 2) pitch = float(M_PI) / 2;
			//camera looks down -z (basically at the player's feet) when pitch is at zero.
			//pitch = std::min(pitch, 0.95f * 3.1415926f);
			//pitch = std::max(pitch, 0.05f * 3.1415926f);
			//player.camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
			//glm::mat4 view = glm::lookAt(player.camera->transform->position, player.transform->position, up);
			update_camera();

			return true;
		}
	} else if (evt.type == SDL_MOUSEWHEEL) {
		if (evt.wheel.y > 0 && camera_dist > camera_min_dist) {
			camera_dist -= 1.0f;
		}
		else if (evt.wheel.y < 0 && camera_dist < camera_max_dist) {
			camera_dist += 1.0f;
		}
		update_camera();
		return true;
	}

	return false;
}

void PlayMode::update(float elapsed) {
	//player walking:
	{
		// set player speed 
		if (!(up.pressed || down.pressed || left.pressed || right.pressed || slide.pressed || jump.pressed)  ) // player is still
		{
			speed_multiplier = low_speed; // don't start at 0 speed, this looks better
		}
		else if (!sliding)
		{
			speed_multiplier = std::min(1.0f, speed_multiplier + accel*elapsed);
		}
		PlayerSpeed = PlayerSpeedMax * speed_multiplier; // overwritten later if sliding

		//combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);
		
		if (jump.pressed && released) {
			if (!in_air) {
				released = false;
				in_air = true;
				jump_up_velocity = jump_speed;
			}
		}

		if (slide.pressed && !sliding) {
			sliding = true;
			player.transform->scale.z = 0.6f * player_height_default;
		}

		if (sliding && !in_air) {
			// when sliding, bypass WASD command and only charge in the direction the player faces
			move.y = 1.0f;
			speed_multiplier -= friction * elapsed;
			speed_multiplier = std::max(low_speed, speed_multiplier);
		}
		else if (sliding && in_air) {
			reset_sliding();
		}
		else {
			if (left.pressed && !right.pressed) move.x = -1.0f;
			if (!left.pressed && right.pressed) move.x = 1.0f;
			if (down.pressed && !up.pressed) move.y = -1.0f;
			if (!down.pressed && up.pressed) move.y = 1.0f;
		}


		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		//get move in world coordinate system:
		glm::vec3 remain = player.transform->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);
		glm::vec3 remain_copy = remain;

		// WalkPoint before moving
		WalkPoint before = player.at;

		// step in walkmesh.
		step_in_mesh(remain);

		//update player's 3D position to respect walking
		glm::vec3 temp_pos;
		glm::quat temp_rot;
		step_in_3D(temp_pos, temp_rot);

		// check for jumping
		if (in_air) {
			if (!climbing) {
				if (jump_up_velocity - gravity * elapsed < max_fall_speed) {
					jump_up_velocity = max_fall_speed;
				}
				else {
					jump_up_velocity -= gravity * elapsed;
				}
				z_relative += jump_up_velocity * elapsed;
				if (z_relative <= 0.0f) {
					z_relative = 0.0f;
					jump_up_velocity = 0.0f;
					in_air = false;
					on_platform = false;
				}
			} else {
				assert(obstacle_box != nullptr);
				z_relative += elapsed * climb_speed;
				float platform_height = obstacle_box->c.z + obstacle_box->r.z;
				if (z_relative > platform_height) {
					// climb on top!
					z_relative = platform_height;
					climbing = false;
					on_platform = true;
					in_air = false;
					jump_up_velocity = 0.0f;
				}
			}
		}

		temp_pos.z = temp_pos.z + z_relative;

		// check if the new position leads to a collision
		// create player bounding box
		float player_height = 0.75f;
		if (sliding) player_height = 0.15f;
		Collision::AABB player_box = Collision::AABB(temp_pos, { 0.45f,0.4f,player_height });
		player_box.c.z += player_box.r.z; // hardcode z-offset because in blender frame is at bottom

		bool reset_pos = false;

		// if we are climbing, we are essentially holding onto an obstacle and have no need to detect collision
		if (!climbing) {
			bool collision = false; // was there a collision update?
			for (Collision::AABB& p : obstacles)
			{	
				int collision_x_or_y = Collision::testCollision(p, player_box);
				if (collision_x_or_y && obstacle_box != &p)
				{
					collision = true;
					float obstacle_height = p.c.z + p.r.z;
					if (in_air) {
						if (jump_up_velocity < 0 && std::abs(z_relative - obstacle_height) < 0.4f) {
							z_relative = obstacle_height;
							jump_up_velocity = 0.0f;
							in_air = false;
							on_platform = true;
							obstacle_box = &p;
						} else {
							// Check for two things:
							// - Player is sufficiently close to the edge of the platform vertically
							// - Jump key has been released from the previous press
							if (z_relative < obstacle_height) {
								if (obstacle_height - z_relative < 0.5f || /* on-ground climb */
									(obstacle_height - z_relative < 2.0f && jump.pressed && released) /* in-air climb */) {
									climbing = true;
									obstacle_box = &p;
								}
							}
						}
					}
					player.at = before; // revert position
					// if not on a platform and not in the air, you want to slide along the obstacle in direction of no collision
					// slide x or y 
					if ((collision_x_or_y == 2 && last_collision != 1) || (collision_x_or_y == 1 && last_collision == 2)) // collision in y-axis so move only in x-direction
					{
						last_collision = 2;
						remain = glm::vec4(remain_copy.x, 0.0f, 0.0f, 0.0f); // only move in x-axis
					}
					else// if ((collision_x_or_y == 1 && last_collision != 2) || (collision_x_or_y == 2 && last_collision == 1)) // collision in x-axis so move in y-direction
					{
						last_collision = 1;
						remain = glm::vec4(0.0f, remain_copy.y, 0.0f, 0.0f); // only move in y-axis
					}
					step_in_mesh(remain);
					step_in_3D(temp_pos, temp_rot);
					temp_pos.z = temp_pos.z + z_relative; 

				}
				if (on_platform && obstacle_box == &p && !Collision::testCollision(p, player_box)) {
					in_air = true;
					on_platform = false;
					obstacle_box = nullptr;
				}
			}
			if (!collision)
			{
				last_collision = 0; // if there was no collision, clear
			}
		} else {
			if (Collision::testCollision(*obstacle_box, player_box)) {
				player.at = before;
				reset_pos = true;
			}
		}
		
		// there was no collision, update player's transform
		if (!reset_pos) {
			player.transform->position = temp_pos;
			player.transform->rotation = temp_rot;
		}
		else {
			player.transform->position.z = temp_pos.z;
		}

		// set shadow pos
		shadow->position.x = player.transform->position.x;
		shadow->position.y = player.transform->position.y;
		Collision::AABB *tmp = nullptr;
		for (Collision::AABB& p : obstacles) {
			if (Collision::testCollisionXYStrict(p, player_box) && p.r.z + p.c.z <= player.transform->position.z) {
				tmp = &p;
				shadow->position.z = p.c.z + p.r.z + shadow_base_height;
				break;
			}
		}
		if (tmp == nullptr) {
			shadow->position.z = shadow_base_height;
		}
		
		bool in_range = false;
		// play a message depending on your position
		for (int i = 0; i < (int)messages.size(); i++)
		{
			// check if in range of something
			glm::vec3 diff = player.transform->position - messages[i].first;
			if ((diff.x * diff.x + diff.y * diff.y + diff.z * diff.z < 1.0f))
			{
				in_range = true;
				// if not already there
				if (i != idx_message)
				{
					idx_message = i;
				}
			}
		}
		if (!in_range)
		{
			idx_message = -1;
		}
	}

	update_camera();

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	player.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*player.camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));


		std::string draw_str = "";// "Mouse motion looks; WASD moves; escape ungrabs mouse. "; // MODIFY THIS FOR ANY DEFAULT STRING
		// print message string
		if (idx_message != -1)
		{
			draw_str += messages[idx_message].second;
		}

		constexpr float H = 0.09f;
		lines.draw_text(draw_str,
			glm::vec3(-aspect + 0.1f * H, -0.7 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(draw_str,
			glm::vec3(-aspect + 0.1f * H + ofs, -0.7 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

void PlayMode::step_in_3D(glm::vec3 & pos, glm::quat & rot)
{
	pos = walkmesh->to_world_point(player.at);

	{ //update player's rotation to respect local (smooth) up-vector:

		glm::quat adjust = glm::rotation(
			player.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
			walkmesh->to_world_smooth_normal(player.at) //smoothed up vector at walk location
		);
		rot = glm::normalize(adjust * player.transform->rotation);
	}
}
void PlayMode::step_in_mesh(glm::vec3& remain)
{
	//using a for() instead of a while() here so that if walkpoint gets stuck in
		// some awkward case, code will not infinite loop:
	for (uint32_t iter = 0; iter < 10; ++iter) {
		if (remain == glm::vec3(0.0f)) break;
		WalkPoint end;
		float time;
		walkmesh->walk_in_triangle(player.at, remain, &end, &time);
		player.at = end;
		if (time == 1.0f) {
			//finished within triangle:
			remain = glm::vec3(0.0f);
			break;
		}
		//some step remains:
		remain *= (1.0f - time);
		//try to step over edge:
		glm::quat rotation;
		if (walkmesh->cross_edge(player.at, &end, &rotation)) {
			//stepped to a new triangle:
			player.at = end;
			//rotate step to follow surface:
			remain = rotation * remain;
		}
		else {
			//ran into a wall, bounce / slide along it:
			glm::vec3 const& a = walkmesh->vertices[player.at.indices.x];
			glm::vec3 const& b = walkmesh->vertices[player.at.indices.y];
			glm::vec3 const& c = walkmesh->vertices[player.at.indices.z];
			glm::vec3 along = glm::normalize(b - a);
			glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
			glm::vec3 in = glm::cross(normal, along);

			//check how much 'remain' is pointing out of the triangle:
			float d = glm::dot(remain, in);
			if (d < 0.0f) {
				//bounce off of the wall:
				remain += (-1.25f * d) * in;
			}
			else {
				//if it's just pointing along the edge, bend slightly away from wall:
				remain += 0.01f * d * in;
			}
		}
	}


	if (remain != glm::vec3(0.0f)) {
		std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
	}
}