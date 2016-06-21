#pragma once

#include <string>
#include <vector>

class Image
{
public:
	Image();
	Image(const std::string& file);
	bool load(const std::string& path);

	const unsigned char* getData() const;

	unsigned int getWidth() const;
	unsigned int getHeight() const;

private:
	unsigned int m_width = 0;
	unsigned int m_height = 0;
	std::vector<unsigned char> m_data;
};