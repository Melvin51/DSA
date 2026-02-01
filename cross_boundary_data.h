typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long  u64;
typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

#define Khz 1000
#define Mhz 1000000
#define Ghz 1000000000

#define milliseconds 1000
#define microseconds 10000

struct coords
{
	float x;
	float y;
};

struct frame_buffer
{
	int width;
	int height;
	void *memory;
};

struct migrating_data
{
	u64 clock_cycles;
	float frame_time;
	struct coords square;
};