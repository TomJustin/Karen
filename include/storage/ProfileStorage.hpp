#pragma once

#include <fstream>
#include <iostream>
#include "user/Profile.hpp"

struct ProfileStorage{

    bool loadProfile(Profile& profile);

    bool setupProfile(Profile& profile);
};