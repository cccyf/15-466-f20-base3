#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

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

	void sing(float elapsed);
	void rotate(float elapsed);
	void move(float elapsed, int direction);
	void eat(float elapsed);
	void think(float elapsed);

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform *body = nullptr;
	Scene::Transform *head = nullptr;
	Scene::Transform *mouth = nullptr;
	Scene::Transform *left_leg = nullptr;
	Scene::Transform *right_leg = nullptr;
	Scene::Transform *tail_0 = nullptr;
	Scene::Transform *tail_1 = nullptr;

	glm::quat head_rotation;
	glm::quat left_leg_rotation;
	glm::quat right_leg_rotation;

	Scene::Transform *egg = nullptr;
	glm::vec3 egg_position;

	glm::vec3 leftmost_pos;
	glm::vec3 rightmost_pos;

	glm::vec3 valid_left_pos;
	glm::vec3 valid_right_pos;

	float sing_duration = -1.f;
	float sing_limit = 5.f;

	glm::quat bird_rotation;
	float rotation_duration = -1.f;
	float rotation_limit = 5.f;

	glm::vec3 bird_position;
	float move_left_duration = -1.f;
	float move_right_duration = -1.f;
	float move_limit = 2.f;
	int cur_pos_index = 0;

	float eat_duration = -1.f;
	float eat_limit = 2.f;

	int direction = 0;

	float last_duration = 5.f;
	float change_limit = 5.f;

	float thinking = -1.f;
	float thinking_limit = 2.f;

	float game_duration = 0.0f;

	std::shared_ptr< Sound::PlayingSample > robin;
	//hexapod leg to wobble:
	Scene::Transform *hip = nullptr;
	Scene::Transform *upper_leg = nullptr;
	Scene::Transform *lower_leg = nullptr;
	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;

	glm::vec3 get_leg_tip_position();

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > leg_tip_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
