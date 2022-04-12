#pragma once

#include "Program.h"
#include <memory>
#include <vector>
#include <string>

class Revo
{
	public:
		Revo();
		virtual ~Revo();
		void init();
		void draw(const std::shared_ptr<Program> prog) const;
		float lowest_y = 0.0;
	private:
		std::vector<float> posBuf;
		std::vector<float> norBuf;
		std::vector<float> texBuf;
		std::vector<unsigned int> indBuf;
		unsigned posBufID;
		unsigned norBufID;
		unsigned texBufID;
		unsigned indBufID;
};
