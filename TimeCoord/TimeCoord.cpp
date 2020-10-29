// TimeCoord.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

void xchgpix(char *buf, int w, int h, int x1, int y1, int fi1,
	int x2, int y2, int fi2) {

	char tmp = buf[w*h * 3 * fi1 + x1 + y1 * w];
	buf[w*h * 3 * fi1 + x1 + y1 * w] = buf[w*h * 3 * fi2 + x2 + y2 * w];
	buf[w*h * 3 * fi2 + x2 + y2 * w] = tmp;

	tmp = buf[w*h * 3 * fi1 + w * h + x1 + y1 * w];
	buf[w*h * 3 * fi1 + w * h + x1 + y1 * w] = buf[w*h * 3 * fi2 + w * h + x2 + y2 * w];
	buf[w*h * 3 * fi2 + w * h + x2 + y2 * w] = tmp;

	tmp = buf[w*h * 3 * fi1 + 2 * w*h + x1 + y1 * w];
	buf[w*h * 3 * fi1 + 2 * w*h + x1 + y1 * w] = buf[w*h * 3 * fi2 + 2 * w*h + x2 + y2 * w];
	buf[w*h * 3 * fi2 + 2 * w*h + x2 + y2 * w] = tmp;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	std::array<char, 128> buffer;
	std::string tmp, s_width, s_height, s_framerate, s_framenum;
	int w, h, framenum;
	std::wstring fn1, fn2, w_params;

	if (argc < 3) {
		int rc = std::fputs("Command syntax:\nTimeCoord invideo outvideo [-p params]\nwhere:\n  invideo  - filename of input video\n  outvideo - filename of output video\n  params   - optional parameters to give to ffmpeg for encoding output video\n", stderr);
		if (rc == EOF)
			std::perror("fputs() failed");
		exit(__LINE__);
	}
	fn1 = argv[1];
	fn2 = argv[2];
	if (argc > 3) {
		std::wstring tmp2 = argv[3];
		if (tmp2 == L"-p") {
			for (int i = 4; i < argc; i++) {
				w_params = w_params + L"\"" + argv[i] + L"\" ";
			}
		}
		else {
			printf("Incorrect parameter\n");
			exit(__LINE__);
		}
	}
	printf("Counting frames...\n");
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_wpopen((L"ffprobe -v error -count_frames -select_streams v:0 -show_entries stream=width,height,avg_frame_rate,nb_read_frames -of default=nokey=1:noprint_wrappers=1 \"" + fn1 + L"\"").c_str(), L"r"), _pclose);
	if (!pipe) {
		std::perror("_wpopen() failed");
		exit(__LINE__);
	}
	if (!std::fgets(buffer.data(), buffer.size(), pipe.get())) {
		std::perror("fgets() failed");
		exit(__LINE__);
	}
	s_width = buffer.data();
	s_width.erase(s_width.end() - 1);
	std::wstring w_width(s_width.begin(), s_width.end());
	if (!std::fgets(buffer.data(), buffer.size(), pipe.get())) {
		std::perror("fgets() failed");
		exit(__LINE__);
	}
	s_height = buffer.data();
	s_height.erase(s_height.end() - 1);
	std::wstring w_height(s_height.begin(), s_height.end());
	if (!std::fgets(buffer.data(), buffer.size(), pipe.get())) {
		std::perror("fgets() failed");
		exit(__LINE__);
	}
	s_framerate = buffer.data();
	s_framerate.erase(s_framerate.end() - 1);
	std::wstring w_framerate(s_framerate.begin(), s_framerate.end());
	if (!std::fgets(buffer.data(), buffer.size(), pipe.get())) {
		std::perror("fgets() failed");
		exit(__LINE__);
	}
	s_framenum = buffer.data();
	s_framenum.erase(s_framenum.end() - 1);
	w = std::atoi(s_width.c_str());
	h = std::atoi(s_height.c_str());
	framenum = std::atoi(s_framenum.c_str());
	printf("%d frames [%dx%d] at %s\n", framenum, w, h, s_framerate.c_str());
	size_t bufsize;
	bufsize = (w >= h) ? 3*w*w*h : 3*w*h*h;
	std::unique_ptr<char[]> buf(new char[bufsize]);
	if (!buf) {
		std::perror("Memory allocation failed");
		exit(__LINE__);
	}
	size_t readsize;
	std::unique_ptr<FILE, decltype(&_pclose)> pipe2(_wpopen((L"ffmpeg -hide_banner -v error -i \"" + fn1 + L"\" -map 0:v:0 -f rawvideo -vcodec rawvideo -pix_fmt yuv444p -").c_str(), L"rb"), _pclose);
	if (!pipe2) {
		std::perror("_wpopen() failed");
		exit(__LINE__);
	}
	std::unique_ptr<FILE, decltype(&_pclose)> pipe3(_wpopen((L"ffmpeg -hide_banner -v error -f rawvideo -pix_fmt yuv444p -video_size " + w_width + L"x" + w_height + L" -framerate " + w_framerate + L" -i - " + w_params + L"\"" + fn2 + L"\"").c_str(), L"wb"), _pclose);
	if (!pipe3) {
		std::perror("_wpopen() failed");
		exit(__LINE__);
	}
	printf("Encoding started:\n0%%");
	size_t block_number = 0;
	size_t block_count = (w >= h) ? framenum/w+1 : framenum/h+1;
	while (0 != (readsize = std::fread(buf.get(), 1, bufsize, pipe2.get()))) {
		if (readsize < bufsize) {
			memset(buf.get() + readsize, 0, bufsize - readsize);
		}
		if (w >= h) {
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w - 1; x++) {
					for (int fi = x + 1; fi < w; fi++) {
						xchgpix(buf.get(), w, h, x, y, fi, fi, y, x);
					}
				}
			}
		}
		else {
			for (int x = 0; x < w; x++) {
				for (int y = 0; y < h - 1; y++) {
					for (int fi = y + 1; fi < h; fi++) {
						xchgpix(buf.get(), w, h, x, y, fi, x, fi, y);
					}
				}
			}
		}
		std::fwrite(buf.get(), 1, bufsize, pipe3.get());
		block_number++;
		printf("\r%.2f%%", 100.0*block_number/block_count);
	}
	printf("\r100.00%%\n");
}
