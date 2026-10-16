#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#include "helpers/Logger.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

/*!
 * Utils
 * Miscellaneous utility functions
 * @static
 */
class Utils {
public:
	static char* getFileContent(std::string path);

	static GLFWwindow* window;
	static bool USE_ANISO;
	static float MAX_ANISO;
	static int QUALITY;

	static double TARGET_FPS;

	static double calculFPS();

	static double random(double min, double max);

	// Movement integration scale: Renderer::delta is elapsed wall seconds
	// divided by TARGET_FPS, co-tuned with Camera::moveSpeed (200). Do not
	// change this scale without retuning every movement speed, and keep
	// TARGET_FPS as the single source of truth (no parallel constant).
	static double movementDelta(double elapsedSeconds);
};

template <typename T> size_t indexOf(const std::vector<T>& values, T value) {
	for (size_t i = 0; i < values.size(); ++i) {
		if (values.at(i) == value) {
			return i;
		}
	}
	throw std::runtime_error("Index not found");
	return 0;
}

template <typename K, typename V>
V value(std::unordered_map<K, V>& data, const K& key) {
	auto iter = data.find(key);
	if (iter == data.end()) {
		throw std::runtime_error("Value not found");
	}
	return iter->second;
}

std::vector<std::string>& operator<<(std::vector<std::string>& stringList,
	const std::string& str);
std::vector<std::string> operator<<(std::vector<std::string> stringList,
	const char* str);