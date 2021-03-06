#ifndef TEST_BUILD

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cstdint>
#include "shell/command-line-parser.h"
#include "imaging/bitmap.h"
#include "imaging/bmp-format.h"
#include "imaging/bmp-format.h"
#include "midi/midi.h"

using namespace midi;
using namespace std;
using namespace shell;
using namespace imaging;

void draw_rectangle(Bitmap& bitmap, const Position& pos, const uint32_t& width, const uint32_t& height, const Color& color){
	for (uint32_t i = 0; i < width; i++){
		for (uint32_t j = 0; j < height; j++){
			bitmap[Position(pos.x + i, pos.y + j)] = color;
		}
	}
}

int getWidth(vector<NOTE> notes){
	int result = 0;
	for (NOTE n : notes){
		if (value(n.start + n.duration) > result){
			result = value(n.start + n.duration);
		}
	}
	return result;
}

int getLowestNote(vector<NOTE> notes){
	int result = 127;
	for (NOTE n : notes){
		if (value(n.note_number) < result){
			result = value(n.note_number);
		}
	}
	return result;
}

int getHighestNote(vector<NOTE> notes){
	int result = 0;
	for (NOTE n : notes){
		if (value(n.note_number) > result){
			result = value(n.note_number);
		}
	}
	return result;
}

int main(int argn, char* argv[]){
	Bitmap bm(500, 500);
	draw_rectangle(bm, Position(200, 200), 100, 30,Color(0, 1, 1));

	std::string input_file;
	std::string pattern;
	uint32_t height = 16;
	uint32_t scale = 10;
	uint32_t step = 1;
	uint32_t framewidth = 0;

	CommandLineParser parser;
	parser.add_argument(string("-w"), &framewidth);
	parser.add_argument(string("-d"), &step);
	parser.add_argument(string("-s"), &scale);
	parser.add_argument(string("-h"), &height);
	parser.process(argn, argv);
	if (parser.positional_arguments().size() < 2){
		exit(EXIT_FAILURE);
	}

	input_file = parser.positional_arguments()[0];
	pattern = parser.positional_arguments()[1];
	std::ifstream input_file_stream(input_file, std::ios_base::binary);
	vector<NOTE> notes = read_notes(input_file_stream);
	uint32_t mapwidth = getWidth(notes) / scale;
	if (framewidth == 0){
		framewidth = mapwidth;
	}

	int low = getLowestNote(notes);
	int high = getHighestNote(notes);
	Bitmap bitmap(mapwidth, 127 * height);
	for (int i = 0; i <= 127; i++) {
		for (NOTE n : notes) {
			if (n.note_number == NoteNumber(i)) {
				draw_rectangle(bitmap, Position(value(n.start) / scale,
					(127 - i) * height),
					value(n.duration) / scale, height,
					Color(0, 1, 1));
			}
		}
	}

	bitmap = *bitmap.slice(0, height * (127 - high),
		mapwidth, (high - low + 1) * height).get();

	for (int i = 0; i <= mapwidth - framewidth; i += step){
		Bitmap temp = *bitmap.slice(i, 0, framewidth,
			(high - low + 1) * height).get();

		stringstream counter;
		counter << setfill('0') << setw(5) << (i / step);
		string out = pattern;
		imaging::save_as_bmp(out.replace(out.find("%d"), 2, counter.str()), temp);
		cout << "Image: " << (i / step) << " rendered" << endl;
	}
}
#endif