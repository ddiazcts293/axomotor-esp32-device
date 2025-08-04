#include "services/axomotor_service.hpp"

using namespace axomotor::services;

extern "C" void app_main() 
{
    AxoMotorService::init();
}
