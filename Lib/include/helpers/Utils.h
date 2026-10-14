#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <stdexcept>

#include <fmtlog/fmtlog.h>

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
	static unsigned int MAX_ANISO;
	static int QUALITY;

	static double TARGET_FPS;

	static double calculFPS();

	static double random(double min, double max);
};

class Timer {
public:
	void start();
	double restart();

private:
	std::chrono::system_clock::time_point m_start;
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
V value(std::unordered_map<K, V>& data, K key, V def) {
	auto iter = data.find(key);
	if (iter == data.end()) {
		logw("Failed to find value {}, inserting default {}", key, def);
		data[key] = def;
		return def;
	}
	return iter->second;
}

std::vector<std::string>& operator<<(std::vector<std::string>& stringList,
	const std::string& str);
std::vector<std::string> operator<<(std::vector<std::string> stringList,
	const char* str);