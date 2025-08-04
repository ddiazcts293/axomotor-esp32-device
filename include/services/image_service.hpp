#pragma once

#include <service_base.hpp>
#include <esp_camera.h>

namespace axomotor::services {

class ImageService : public threading::ServiceBase
{
public:
    ImageService();

private:
    bool m_is_active;
    std::string m_current_dir;

    esp_err_t setup() override;
    void loop() override;

    bool ensure_dir_exists();
    bool save_image(camera_fb_t *fb);
};

} // namespace axomotor::services
