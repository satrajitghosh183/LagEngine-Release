//
// Created by barth on 19/09/2022.
//

#define STB_IMAGE_IMPLEMENTATION

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <glm/vec4.hpp>
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

void loadFileToBuffer(const char *filename, std::string &stringBuffer) {
    stringBuffer = ""; // empty buffer first
    std::ifstream reader(filename);
    if (reader.is_open()) {
        std::string lineBuffer;
        while (std::getline(reader, lineBuffer)) {
            stringBuffer += lineBuffer + "\n";
        }
        reader.close();
    } else std::cerr << "Cloud not open " << filename << std::endl;
}

std::string toString(glm::vec3 vec) {
    return "(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ")";
}
