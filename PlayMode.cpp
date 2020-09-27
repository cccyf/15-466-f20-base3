#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include <time.h>

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint bird_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > bird_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("bird.pnct"));
	bird_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > bird_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("bird.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = bird_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = bird_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});

Load< Sound::Sample > robin_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("robin.wav"));
});

Load< Sound::Sample > bg_birds_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("bg.wav"));
});

Load< Sound::Sample > game_over_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("game_over.wav"));
});

PlayMode::PlayMode() : scene(*bird_scene) {
	srand (time(NULL));
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "body") body = &transform;
		else if (transform.name == "head") head = &transform;
		else if (transform.name == "mouth") mouth = &transform;
		else if (transform.name == "left leg") left_leg = &transform;
		else if (transform.name == "right leg") right_leg = &transform;
		else if (transform.name == "tail0") tail_0 = &transform;
		else if (transform.name == "tail1") tail_1 = &transform;
		else if (transform.name == "egg") egg = &transform;
	}
	if (body == nullptr) throw std::runtime_error("Body not found.");
	if (head == nullptr) throw std::runtime_error("Head not found.");
	if (mouth == nullptr) throw std::runtime_error("mouth not found.");
	if (left_leg == nullptr) throw std::runtime_error("left_leg not found.");
	if (right_leg == nullptr) throw std::runtime_error("right_leg not found.");
	if (tail_0 == nullptr) throw std::runtime_error("tail_0 not found.");
	if (tail_1 == nullptr) throw std::runtime_error("tail_1 not found.");
	if (egg == nullptr) throw std::runtime_error("egg not found.");

	head_rotation = head->rotation;
	left_leg_rotation = left_leg->rotation;
	right_leg_rotation = right_leg->rotation;

	bird_rotation = body->rotation;

	// head->rotation = head_rotation * glm::angleAxis(
	// 	glm::radians(1.0f * std::sin(2.f * float(M_PI))),
	// 	glm::vec3(0.0f, 1.0f, 0.0f)
	// );

	// hip_base_rotation = hip->rotation;
	// upper_leg_base_rotation = upper_leg->rotation;
	// lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	egg_position = egg->position;
	std::cout << egg_position.x << " " << egg_position.y  << " " << egg_position.z << "\n";

	glm::mat4x3 frame = camera->transform->make_local_to_parent();
	glm::vec3 right = frame[0];

	leftmost_pos = egg_position - 2.f * right;
	rightmost_pos = egg_position + 2.f * right;

	valid_left_pos = egg_position - 0.2f * right;
	valid_right_pos = egg_position + 0.2f * right;
	
	egg->position = leftmost_pos;


	// body->rotation = bird_rotation * glm::angleAxis(
	// 	glm::radians(std::min(-10.f, 720.0f )),
	// 	glm::vec3(1.0f, 0.0f, 0.0f)
	// );
	
	// Sound::Sample trance =*dusty_floor_sample;
	Sound::loop(*bg_birds_sample, 1.0, 0.0);

	//start music loop playing:
	// (note: position will be over-ridden in update())
	// leg_tip_loop = Sound::loop_3D(*blippy_trance_sample, 1.0f, get_leg_tip_position(), 10.0f);
}

PlayMode::~PlayMode() {
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
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		// if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
		// 	SDL_SetRelativeMouseMode(SDL_TRUE);
		// 	return true;
		// }
	} else if (evt.type == SDL_MOUSEMOTION) {
		// if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			// glm::vec2 motion = glm::vec2(
			// 	evt.motion.xrel / float(window_size.y),
			// 	-evt.motion.yrel / float(window_size.y)
			// );
			// camera->transform->rotation = glm::normalize(
			// 	camera->transform->rotation
			// 	* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
			// 	* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			// );
			// egg->position.y = evt.motion.yrel;
			return true;
		// }
	}

	return false;
}

void PlayMode::sing(float elapsed){
	sing_duration += elapsed;

	body->rotation = bird_rotation * glm::angleAxis(
		glm::radians(std::min(10.f, 180.0f * sing_duration / sing_limit)),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	if (sing_duration >= sing_limit)
	{
		robin->stop(0.f);
		sing_duration = -1.f;
		body->rotation = bird_rotation;
	}
	return;
}

void PlayMode::rotate(float elapsed){
	rotation_duration += elapsed;

	body->rotation = bird_rotation * glm::angleAxis(
		glm::radians(std::min(180.f, 720.0f * rotation_duration / rotation_limit)),
		glm::vec3(.0f, .0f, 1.0f)
	);

	if (rotation_duration >= rotation_limit)
	{
		rotation_duration = -1.f;
		body->rotation = bird_rotation;
	}
	
}

void PlayMode::move(float elapsed, int direction){

}

void PlayMode::eat(float elapsed){
	eat_duration += elapsed;

	body->rotation = bird_rotation * glm::angleAxis(
		glm::radians(std::max(-10.f, -180.0f * eat_duration / eat_limit)),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	if (eat_duration >= eat_limit)
	{
		eat_duration = -1.f;
		body->rotation = bird_rotation;

		game_duration = .0f;
		egg->position = leftmost_pos;
	}
	
}

void PlayMode::think(float elapsed){
	thinking += elapsed;
	if (thinking >= thinking_limit)
	{	
		thinking = -1.f;
	}
}

void PlayMode::update(float elapsed) {

	if(eat_duration >= 0.f){
		eat(elapsed);
		return;
	}

	if(egg->position.x >= valid_left_pos.x && egg->position.x <= valid_right_pos.x){
		game_duration += elapsed;
	}


	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 3.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;


		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right_move = frame[0];
		// glm::vec3 up = frame[1];
		// glm::vec3 forward = -frame[2];

		egg->position += move.x * right_move;

		if (egg->position.x < leftmost_pos.x)
		{
			egg->position = leftmost_pos;
		}else if (egg->position.x > rightmost_pos.x){
			egg->position = rightmost_pos;
		}

		// camera->transform->position += move.x * right + move.y * forward;

		//reset button press counters:
		left.downs = 0;
		right.downs = 0;
		up.downs = 0;
		down.downs = 0;
	}

	//slowly rotates through [0,1):
	wobble += elapsed / 3.0f;
	wobble -= std::floor(wobble);


	head->rotation = head_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 3.0f * float(M_PI))),
		glm::vec3(.2f, 1.0f, 0.0f)
	);
	left_leg->rotation = left_leg_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);
	right_leg->rotation = right_leg_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	if (thinking >= 0.f)
	{
		think(elapsed);
		return;	
	}
	

	if(sing_duration >= 0.f){
		sing(elapsed);
		return;
	}

	if(rotation_duration >= 0.f){
		rotate(elapsed);
		return;
	}

	last_duration += elapsed;

	if (egg->position.x >= valid_left_pos.x && egg->position.x <= valid_right_pos.x)
	{
	// std::cout << distance << "\n";
		// eat
		Sound::play(*game_over_sample, 2.0f);
		eat_duration = 0.f;
		last_duration = 0.f;
		return;
	}

	if(last_duration >= change_limit)
	// make choice
	{
		int choice = rand() % 100;
		if (choice < 30)
		{
			// sing
			std::cout << choice << " sing\n";
			sing_duration = 0.f;
			last_duration = 0.f;
			robin = Sound::loop(*robin_sample, 1.f, 1.f);
			return;
		}
		if (choice < 50)
		{
			// circle back
			std::cout << choice << " rotation\n";
			rotation_duration = 0.f;
			last_duration = 0.f;
			return;
		}
		
		thinking = 0.f;
		std::cout << choice << " thinking\n";
		// if (choice < 30 && cur_pos_index > -1)
		// {
		// 	// move left
		// 	move_left_duration = 0.f;
		// 	return;
		// }
		// if (choice < 40 && cur_pos_index < 1)
		// {
		// 	// move right
		// 	move_right_duration = 0.f;
		// 	return;
		// }
		// glm::vec3 relative = head->position - egg->position;
		// float distance = relative.x * relative.x + relative.y * relative.y + relative.z * relative.z;
		// std::cout << choice <<" " <<distance << " eat\n";
		// std::cout << "bird x "<< head->position.x << "egg x " << egg->position.x << "\n";
		// std::cout << "bird y "<< head->position.y << "egg y " << egg->position.y << "\n";
		// std::cout << "bird z "<< head->position.z << "egg z " << egg->position.z << "\n";
	}



	//move sound to follow leg tip position:
	// leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);



	{ //update listener to camera position:
		// glm::mat4x3 frame = camera->transform->make_local_to_parent();
		// glm::vec3 right = frame[0];
		// glm::vec3 at = frame[3];
		// Sound::listener.set_position_right(at, right, 1.0f / 60.0f);
	}


}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

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
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Duration: "+ std::to_string(game_duration),
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Duration: "+ std::to_string(game_duration),
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
