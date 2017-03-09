//
//  radeonEventHandler.hpp
//  App
//
//  Created by Josh McNamee on 11/10/2016.
//
//

#ifndef radeonEventHandler_h
#define radeonEventHandler_h

#include <cstdlib>
#include <azure/Eventable.hpp>
#include <azure/Event.hpp>
#include <azure/Eventkey.hpp>
#include <azure/glm/glm.hpp>

#include <Mush Core/camera_path_io.hpp>

#include "../Scene/scene.h"

namespace Baikal {
    class Scene;
}

extern std::unique_ptr<Baikal::Scene> g_scene;

class radeonEventHandler : public azure::Eventable {
    public:
    radeonEventHandler(bool disable_mouse) : azure::Eventable(), _disable_mouse(disable_mouse) {}
        ~radeonEventHandler() {}
        
        bool event(std::shared_ptr<azure::Event> event) {
            if (event->isType("keyDown")) {
                switch (event->getAttribute<azure::Key>("key")) {
                    case azure::Key::Up:
                    case azure::Key::w:
                        _up_pressed = true;
                        break;
                    case azure::Key::Down:
                    case azure::Key::s:
                        _down_pressed = true;
                        break;
                    case azure::Key::Left:
                    case azure::Key::a:
                        _left_pressed = true;
                        break;
                    case azure::Key::Right:
                    case azure::Key::d:
                        _right_pressed = true;
                        break;
					case azure::Key::e:
                    case azure::Key::Home:
                        _home_pressed = true;
                        break;
					case azure::Key::x:
                    case azure::Key::End:
                        _end_pressed = true;
                        break;
                    case azure::Key::LShift:
                    case azure::Key::RShift:
                        _shift_pressed = true;
                        break;
					case azure::Key::Space:
						save_position();
						break;
                }
            } else if (event->isType("keyUp")) {
                switch (event->getAttribute<azure::Key>("key")) {
                    case azure::Key::Up:
                    case azure::Key::w:
                        _up_pressed = false;
                        break;
                    case azure::Key::Down:
                    case azure::Key::s:
                        _down_pressed = false;
                        break;
                    case azure::Key::Left:
                    case azure::Key::a:
                        _left_pressed = false;
                        break;
                    case azure::Key::Right:
                    case azure::Key::d:
                        _right_pressed = false;
                        break;
					case azure::Key::e:
                    case azure::Key::Home:
                        _home_pressed = false;
                        break;
					case azure::Key::x:
                    case azure::Key::End:
                        _end_pressed = false;
                        break;
                    case azure::Key::LShift:
                    case azure::Key::RShift:
                        _shift_pressed = false;
                        break;
                }
            } else if (event->isType("mouseMove")) {
                if (!_disable_mouse) {
                    int m_x = event->getAttribute<int>("x");
                    int m_y = event->getAttribute<int>("y");
                    if (_tracking_mouse == true) {
                        int diff_x = m_x - _m_x;
                        int diff_y = m_y - _m_y;
                        /*
                         float f_diff_x = diff_x * 0.1125f;
                         float f_diff_y = diff_y * 0.1125f;
                         */
                        _mouse_delta_x += diff_x;
                        _mouse_delta_y += diff_y;
                    }
                    _m_x = m_x;
                    _m_y = m_y;
                }
                
            }
            else if (event->isType("mouseDown")) {
                if (event->getAttribute<azure::MouseButton>("button") == azure::MouseButton::Left) {
                    _tracking_mouse = true;
                }
            }
            else if (event->isType("mouseUp")) {
                if (event->getAttribute<azure::MouseButton>("button") == azure::MouseButton::Left) {
                    _tracking_mouse = false;
                }
            }
			else if (event->isType("camera_additional_shift_matrix")) {

				glm::mat4 mat = event->getAttribute<glm::mat4>("matrix");
				//float fov = event->getAttribute<float>("fov");
				glm::vec3 loc = event->getAttribute<glm::vec3>("location");

				memcpy(_oculus_matrix.m, &mat[0][0], sizeof(float) * 4 * 4);

				_oculus_position.x = loc.x;
				_oculus_position.y = loc.y;
				_oculus_position.z = loc.z;

				//_camera->set_additional_displacement(loc);
				//_camera->set_fov(fov);
				//_camera->set_additional_shift_matrix(mat);

				_oculus_changed = true;
			}
            return false;
        }
        
        bool update() {
			_tick++;
			g_scene->camera_->RemoveOculusTransform();
            static auto prevtime = std::chrono::high_resolution_clock::now();
            
            auto time = std::chrono::high_resolution_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(time - prevtime);
            prevtime = time;
            
            bool update = false;
            float kMouseSensitivity = 0.001125f;
            
            float kMovementSpeed = 50.25f;
            
            if (_shift_pressed) {
                kMovementSpeed = kMovementSpeed / 20.0f;
                //kMouseSensitivity = kMouseSensitivity / 20.0f;
            }
            
            float delta_x = _mouse_delta_x * kMouseSensitivity;
            float delta_y = _mouse_delta_y * kMouseSensitivity;
            
            _mouse_delta_x = 0.0f;
            _mouse_delta_y = 0.0f;
            
            float camrotx = delta_x;
            float camroty = -delta_y;
            
            if (std::abs(camroty) > 0.001f)
            {
                g_scene->camera_->Tilt(camroty);
				phi += camroty;
                //gg_scene->camera_->ArcballRotateVertically(float3(0, 0, 0), camroty);
                update = true;
            }
            
            if (std::abs(camrotx) > 0.001f)
            {
                g_scene->camera_->Rotate(camrotx);
				theta += camrotx;
                //gg_scene->camera_->ArcballRotateHorizontally(float3(0, 0, 0), camrotx);
                update = true;
            }
            
            //float g_cspeed = 100.25f;
            
            
            if (_up_pressed)
            {
                g_scene->camera_->MoveForward((float)dt.count() * kMovementSpeed);
                update = true;
            }
            
            if (_down_pressed)
            {
                g_scene->camera_->MoveForward(-(float)dt.count() * kMovementSpeed);
                update = true;
            }
            
            if (_right_pressed)
            {
                g_scene->camera_->MoveRight((float)dt.count() * kMovementSpeed);
                update = true;
            }
            
            if (_left_pressed)
            {
                g_scene->camera_->MoveRight(-(float)dt.count() * kMovementSpeed);
                update = true;
            }
            
            if (_home_pressed)
            {
                g_scene->camera_->MoveUp((float)dt.count() * kMovementSpeed);
                update = true;
            }
            
            if (_end_pressed)
            {
                g_scene->camera_->MoveUp(-(float)dt.count() * kMovementSpeed);
                update = true;
            }

			if (_oculus_changed.exchange(false)) {
				update = true;
			}

			g_scene->camera_->ApplyOculusTransform(_oculus_matrix, _oculus_position);
            return update;

        }

		void write_camera_path(const std::string camera_path) const {

			if (_saved_camera_positions.size() > 1) {
				write_camera_path_json(camera_path.c_str(), _saved_camera_positions);
			}

		}

		void save_position() {
			auto pos = g_scene->camera_->GetPosition();
			_saved_camera_positions.push_back({ _tick, {{pos.x, pos.y, pos.z} , theta, phi, 0.0f, 0.0f } });
		}
        
    private:
        bool _up_pressed = false, _down_pressed = false, _left_pressed = false, _right_pressed = false, _home_pressed = false, _end_pressed = false;
    
		bool _shift_pressed = false;
    
		float _mouse_delta_x = 0.0f, _mouse_delta_y = 0.0f;
        
        bool _tracking_mouse = false;
        
        int _m_x, _m_y;

		std::atomic_bool _oculus_changed;
		RadeonRays::matrix _oculus_matrix;
		RadeonRays::float3 _oculus_position;
    
		bool _disable_mouse;
		std::vector<mush::camera::camera_path_node> _saved_camera_positions;

		float theta = 0.0f;
		float phi = 0.0f;

		int _tick = 0;
    };

#endif /* radeonEventHandler_h */
