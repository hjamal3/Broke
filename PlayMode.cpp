#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

GLuint phonebank_meshes_for_lit_color_texture_program = 0;
GLuint vertex_buffer_for_color_texture_program = 0;
GLuint vertex_buffer = 0;
GLuint white_tex = 0;
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
	//----- allocate OpenGL resources -----
	{ 
		glGenBuffers(1, &vertex_buffer);
		
		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{
		//vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program->Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program->Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program->Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program->Color_vec4);

		glVertexAttribPointer(
			color_texture_program->TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ 
		//solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}




	//create a player transform:
	for (auto& transform : scene.transforms) {
		if (transform.name == "Player") player.transform = &transform;
		if (transform.name == "PlayerShadow") shadow = &transform;
	}

	// go through the meshes and find Cube objects. Convert to naming convention later
	std::string str_obstacle("o_");
	std::string str_barrier("c_");
	const auto& meshes = phonebank_meshes->meshes;
	for (auto& mesh : meshes) {

		if (mesh.first.find(str_obstacle) != std::string::npos)
		{
			auto& min = mesh.second.min;
			auto& max = mesh.second.max;
			glm::vec3 center = 0.5f * (min + max);
			glm::vec3 rad = 0.5f * (max - min);
			obstacles.emplace_back(Collision::AABB(center, rad));
		}
		else if (mesh.first.find(str_barrier) != std::string::npos)
		{
			auto& min = mesh.second.min;
			auto& max = mesh.second.max;
			glm::vec3 center = 0.5f * (min + max);
			glm::vec3 rad = 0.5f * (max - min);
			Collision::AABB box = Collision::AABB(center, rad);
			box.r.z = 100.0f; // set a big vertical barrier so you can never climb the obstacle
			obstacles.emplace_back(box);
		}
	}

	if (player.transform == nullptr) throw std::runtime_error("GameObject not found.");
	if (shadow == nullptr) throw std::runtime_error("GameObject not found.");

	shadow->position = glm::vec3(player.transform->position.x, player.transform->position.y, shadow->position.z);
	shadow_base_height = shadow->position.z;

	// create some message objects. hardcoded for now
	messages.emplace_back(std::make_pair(glm::vec3(player.transform->position.x, player.transform->position.y, player.transform->position.z), "Press WASD to move, press space to jump. Mouse motion to rotate.")); // starting coord of player
	messages.emplace_back(std::make_pair(glm::vec3(-8.5f, -46.0f, 5.0f), "Hold left shift to crawl. Scroll mousewheel to zoom camera."));

	// intialize the prologue introductory texts
	prologue_messages.push_back("I'm a broke octopus./Press Space to Continue");
	prologue_messages.push_back("I used to live in the depth of the ocean with my girlfriend...");
	prologue_messages.push_back("Until I proposed to her...");
	prologue_messages.push_back("I thought that was it. But one day, she was gone.");
	prologue_messages.push_back("Left me a note that goes as below:");
	prologue_messages.push_back("\"I need you to collect something for me.");
	prologue_messages.push_back("\"I'm not an ordinary octopus.");
	prologue_messages.push_back("\"OCEAN STARDUST, CONSCIENCE OF COAL, SEALED NARCISSUS, and HYDROSOI");
	prologue_messages.push_back("\"These I will need to maintain the normal look of an octopus.");
	prologue_messages.push_back("\"Sorry I've been lying to you all this time.");
	prologue_messages.push_back("\"Will you do this for me?\"");
	prologue_messages.push_back("I'm a BROKE octopus with none of the treasures she mentioned.");
	prologue_messages.push_back("I paid visits to the sages. I inquired the sperm whale clairvoyant.");
	prologue_messages.push_back("They all point me towards one answer...");
	prologue_messages.push_back("Humans.");

	objectives.emplace_back(std::make_pair(glm::vec3(player.transform->position.x, player.transform->position.y, player.transform->position.z), "Explore around!"));
	objectives.emplace_back(std::make_pair(glm::vec3(-8.52156f, -46.2738f, 4.80875f), "Find a way into the restaurant."));
	objectives.emplace_back(std::make_pair(glm::vec3(20.8374f, -40.9954f, 4.83061f), "Collect the treasures mentioned in Fiance's note."));
	objectives.emplace_back(std::make_pair(glm::vec3(-14.7522f, -6.78037f, 0.0f), "Get out of the restaurant through the door!"));

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
			if (prologue) {
				prologue_message += 1;
			} else {
				jump.pressed = true;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_LSHIFT) {
			if (!slide.pressed) {
				slide.pressed = true;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_TAB) {
			std::cout << player.transform->position.x << " " << player.transform->position.y << " " << player.transform->position.z << "\n";
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
	if (prologue) {
		if (((uint32_t) prologue_message) < prologue_messages.size()) return;
		prologue = false;
	}

	//player walking:
	{

		//combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);

		// set player speeds
		// if player is still reset to low speed multiplier
		if (!(up.pressed || down.pressed || left.pressed || right.pressed || slide.pressed || jump.pressed)  )
		{
			speed_multiplier = low_speed; // don't start at 0 speed, this looks better
		}
		// use acceleration to set speed multiplier
		else if (!sliding)
		{
			speed_multiplier = std::min(1.0f, speed_multiplier + accel*elapsed);
		}
		// set the speed itself
		PlayerSpeed = PlayerSpeedMax * speed_multiplier;
		
		// start jump
		if (jump.pressed && released) {
			if (!in_air) {
				released = false;
				in_air = true;
				jump_up_velocity = jump_speed;
			}
		}

		// start slide, squish player dimension in z
		if (slide.pressed && !sliding) {
			sliding = true;
			player.transform->scale.z = 0.6f * player_height_default;
		}

		// when sliding, bypass WASD command and only charge in the direction the player faces
		if (sliding && !in_air) {
			move.y = 1.0f;
			speed_multiplier -= friction * elapsed;
			speed_multiplier = std::max(low_speed, speed_multiplier);
		}
		// jumping ends sliding
		else if (sliding && in_air) {
			reset_sliding();
		}
		// regular WASD commands
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

		// WalkPoint before moving, used to reset if collisions
		WalkPoint before = player.at;

		// step in walkmesh.
		step_in_mesh(remain);

		//update player's 3D position to respect walking
		glm::vec3 temp_pos;
		glm::quat temp_rot;
		step_in_3D(temp_pos, temp_rot);

		// jumping: 
		if (in_air) {
			if (!climbing) {
				// cap fall speed
				if (jump_up_velocity - gravity * elapsed < max_fall_speed) {
					jump_up_velocity = max_fall_speed;
				}
				else {
					jump_up_velocity -= gravity * elapsed;
				}
				z_relative += jump_up_velocity * elapsed;
				// landing on the ground
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
		
		// update position in z due to jumping (or not)
		temp_pos.z = temp_pos.z + z_relative;

		// check if the new position leads to a collision
		// create player bounding box
		float player_height = 0.75f/2.0f;
		if (sliding) player_height = 0.15f;
		Collision::AABB player_box = Collision::AABB(temp_pos, { 0.45f,0.4f,player_height });
		player_box.c.z += player_box.r.z; // hardcode z-offset because in blender frame is at bottom
		bool reset_pos = false;

		// collision checking
		if (!climbing) { // if we are climbing, we are essentially holding onto an obstacle and have no need to detect collision
			bool collision = false;
			for (Collision::AABB& p : obstacles)
			{	
				int collision_x_or_y = Collision::testCollision(p, player_box);
				if (collision_x_or_y && obstacle_box != &p)
				{
					collision = true;
					float obstacle_height = p.c.z + p.r.z;
					if (in_air) {
						// if the top of player hits bottom of obstacle, set velocity to 0.0f
						if (jump_up_velocity > 0 && p.c.z - p.r.z <= player_box.c.z + player_box.r.z &&
							p.c.z - p.r.z >= player_box.c.z + player_box.r.z - jump_up_velocity * elapsed)
						{
							z_relative -= jump_up_velocity * elapsed;
							jump_up_velocity = 0.0f;
						}

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

				// on platform but no collision with the obstacle, enable falling
				if (on_platform && obstacle_box == &p && !Collision::testCollision(p, player_box) && (!sliding || !Collision::testCollisionXY(p, player_box)) ) {
						in_air = true;
						on_platform = false;
						obstacle_box = nullptr;
				}
			}
			if (!collision)
			{
				last_collision = 0; // if there was no collision, clear variable (used for sliding motion)
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
				if (tmp == nullptr) {
					tmp = &p;
				} else {
					if (p.r.z + p.c.z >= tmp->c.z + tmp->r.z) {
						tmp = &p;
					}
				}
			}
		}
		if (tmp != nullptr) {
			shadow->position.z = tmp->c.z + tmp->r.z + shadow_base_height;
		} else {
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

		// check if in range of something
		if (cur_objective + 1 < (int) objectives.size() - 1) {
			glm::vec3 diff = player.transform->position - objectives[cur_objective+1].first;
			if ((diff.x * diff.x + diff.y * diff.y + diff.z * diff.z < 2.0f))
			{
				cur_objective++;
			}
		}
		// else if (cur_objective + 1 == (int) objectives.size() - 1) {
			// need to check whether all necessary treasures have been collected
		// }
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
		if (prologue) {
			draw_str += prologue_messages[prologue_message];
		} else if (idx_message != -1)
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
		add_to_textbox(
			glm::vec2(-aspect + 0.1f * H, -0.7 + 0.05f * H),
			glm::vec2(draw_str.size() * H / 2.0f, 2.0f * H));

		if (!prologue) {
			// draw objectives
			lines.draw_text("Objective:",
				glm::vec3(-aspect + 0.1f * H, -0.7 + 17.0f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text("Objective:",
				glm::vec3(-aspect + 0.1f * H + ofs, -0.7 + 17.0f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			lines.draw_text(objectives[cur_objective].second,
				glm::vec3(-aspect + 0.1f * H, -0.7 + 16.0f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text(objectives[cur_objective].second,
				glm::vec3(-aspect + 0.1f * H + ofs, -0.7 + 16.0f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			uint32_t max_len = objectives[cur_objective].second.size() > 9 ? (uint32_t)objectives[cur_objective].second.size() : 9;
			add_to_textbox(
				glm::vec2(-aspect + 0.1f * H, -0.7 + 16.95f * H),
				glm::vec2(max_len * H / 2.0f, 2.0f * H));
		}

		draw_textbox(aspect);
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

void PlayMode::add_to_textbox(glm::vec2 center, glm::vec2 radius)
{

	auto color = glm::u8vec4(0,128,128,255);

	textbox.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	textbox.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	textbox.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

	textbox.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	textbox.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	textbox.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
}

void PlayMode::draw_textbox(float aspect)
{

	glm::mat4 court_to_clip = glm::mat4(
		1.0f / aspect, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, textbox.size() * sizeof(textbox[0]), textbox.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program->program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(textbox.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

	uint32_t size = (uint32_t)textbox.size();
	while (size > 0) {
		textbox.pop_back();
		size--;
	}
}