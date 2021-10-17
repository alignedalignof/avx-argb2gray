#include <vector>
#include <cmath>
#include <cstdarg>
#include <chrono>
#include <cstdio>

using namespace std;
using namespace std::chrono;
using ticker = high_resolution_clock;
static ticker::time_point start = ticker::now();

static void msprintf(const char* fmt, ...)
{
	auto dt = duration<float, milli>(ticker::now() - start);
	printf("[%lf ms] ", dt.count());
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\r\n");
}

__attribute__((optimize("O0"))) static void avx_argb2gray(const uint32_t* argb, uint8_t* gray, int w, int h, int s)
{
	__asm__ __volatile__ (
		"vzeroall;"
		"mov %[ofs], 0x002b4015;"
		"movd xmm7, %[ofs];"
		"vpbroadcastd ymm7, xmm7;"
		"sub %[argb], %[w];"
		"sub %[argb], %[w];"
		"sub %[argb], %[w];"
		"sub %[argb], %[w];"
		"sub %[gray], %[w];"
	"height:"
		"cmp %[h], 0;"
		"jle end;"
		"dec %[h];"
		"xor %[ofs], %[ofs];"
		"add %[argb], %[w];"
		"add %[argb], %[w];"
		"add %[argb], %[w];"
		"add %[argb], %[w];"
		"add %[gray], %[w];"
	"width:"
		"cmp %[ofs], %[w];"
		"jge height;"
		"vmovdqu ymm0, [%[argb] + 4*%[ofs]];"
		"vmovdqu ymm1, [%[argb] + 4*%[ofs] + 32];"
		"vpmaddubsw ymm0, ymm0, ymm7;"
		"vpmaddubsw ymm1, ymm1, ymm7;"
		"vphaddw ymm2, ymm0, ymm1;"
		"vmovdqu ymm0, [%[argb] + 4*%[ofs] + 64];"
		"vmovdqu ymm1, [%[argb] + 4*%[ofs] + 96];"
		"vpsrlw ymm2, ymm2, 7;"
		"vpermq ymm2, ymm2, 0xd8;"
		"vpmaddubsw ymm0, ymm0, ymm7;"
		"vpmaddubsw ymm1, ymm1, ymm7;"
		"vphaddw ymm3, ymm0, ymm1;"
		"vpsrlw ymm3, ymm3, 7;"
		"vpermq ymm3, ymm3, 0xd8;"
		"vpackuswb ymm6, ymm2, ymm3;"
		"vpermq ymm6, ymm6, 0xd8;"
		"vmovdqu [%[gray] + %[ofs]], ymm6;"
		"add %[ofs], 32;"
		"jmp width;"
	"end:"
		"vzeroupper;"
		:
		: [ofs] "r" (0), [argb] "r" (argb), [gray] "r" (gray), [w] "r" (w), [h] "r" (h), [s] "r" (s)
		: "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "memory"
	);
}

static void ez_argb2gray(const uint32_t* argb, uint8_t* gray, int w, int h, int s)
{
	for (int y = 0; y < h; y++)
	for (int x = 0; x < w; x++)
	{
		uint32_t px = argb[y*s + x];
		uint8_t r = px >> 16, g = px >> 8, b = px;
		gray[y*w + x] = (21*b + (g << 6) + 43*r) >> 7;
	}
}

static void test_argb2gray(const char* name, void (*argb2gray)(const uint32_t* argb, uint8_t* gray, int w, int h, int s))
{
	const int W = 4096;
	const int H = 4096;

	vector<uint32_t> argb(W*H + 32); // >_>
	vector<uint8_t> gray(W*H + 32); // <_<
	for (int i = 0; i < argb.size(); i++)
	{
		argb[i] = i | 0xff000000;
	}

	auto expected = [](int x, int y)
	{
		uint32_t px = y*W + x;
		uint8_t r = px >> 16, g = px >> 8, b = px;
		return (21*b + (g << 6) + 43*r) >> 7;
	};

	int times = 1000;
	auto s = high_resolution_clock::now();
	for (int i = times; i --> 0;)
	{
		argb2gray(argb.data(), gray.data(), W, H, H);
	}
	auto dt = duration<double, micro>(high_resolution_clock::now() - s);

	uint32_t errors = 0;
	for (int y = 0; y < H; y++)
	for (int x = 0; x < W; x++)
	if (gray[W*y + x] != expected(x, y))
	{
		errors++;
	}
	msprintf("'%s' x%i: %0.1lf us, %u errors", name, times, dt.count(), errors);
}

int main(int argn, char* argv[])
{
	test_argb2gray("ez c", ez_argb2gray);
	test_argb2gray("sus avx", avx_argb2gray);
	return 0;
}
