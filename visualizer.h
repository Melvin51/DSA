// NOTE: This macro should be changed in the future for something better 
#define DETERMINE_DECIMAL_MAGNITUDE(value, order) \
if(value > 10)\
{\
order = 2;\
}\
if(value > 100)\
{\
order = 3;\
}\
if(value > 1000)\
{\
order = 4;\
}\
if(value > 10000)\
{\
order = 5;\
}\
if(value > 100000)\
{\
order = 6;\
}\
if(value > 1000000)\
{\
order = 7;\
}\
if(value > 10000000)\
{\
order = 8;\
}\
if(value > 100000000)\
{\
order = 9;\
}\
if(value > 1000000000)\
{\
order = 10;\
}\
if(value > 10000000000)\
{\
	order = 11;\
}\
if(value > 100000000000)\
{\
	order = 12;\
}\
if(value > 1000000000000)\
{\
	order = 13;\
}\
if(value > 10000000000000)\
{\
	order = 14;\
}\
if(value > 100000000000000)\
{\
	order = 15;\
}\
if(value > 1000000000000000)\
{\
	order = 16;\
}\
if(value > 10000000000000000)\
{\
	order = 17;\
}\
if(value > 100000000000000000)\
{\
	order = 18;\
}\
if(value > 1000000000000000000)\
{\
	order = 19;\
}\
if(value > 10000000000000000000)\
{\
	order = 20;\
}\

#define DECIMAL 10
// Digits 0-9 represented as tiles
char digits[5 * DECIMAL][3] = 
{
	{1, 1, 1},
	{1, 0, 1},
	{1, 0, 1},
	{1, 0, 1},
	{1, 1, 1},
	
	{0, 1, 0},
	{0, 1, 0},
	{0, 1, 0},
	{0, 1, 0},
	{0, 1, 0},
	
	{1, 1, 1},
	{0, 0, 1},
	{1, 1, 1},
	{1, 0, 0},
	{1, 1, 1},
	
	{1, 1, 1},
	{0, 0, 1},
	{1, 1, 1},
	{0, 0, 1},
	{1, 1, 1},
	
	{1, 0, 1},
	{1, 0, 1},
	{1, 1, 1},
	{0, 0, 1},
	{0, 0, 1},
	
	{1, 1, 1},
	{1, 0, 0},
	{1, 1, 1},
	{0, 0, 1},
	{1, 1, 1},
	
	{1, 1, 1},
	{1, 0, 0},
	{1, 1, 1},
	{1, 0, 1},
	{1, 1, 1},
	
	{1, 1, 1},
	{0, 0, 1},
	{0, 1, 0},
	{0, 1, 0},
	{0, 1, 0},
	
	{1, 1, 1},
	{1, 0, 1},
	{1, 1, 1},
	{1, 0, 1},
	{1, 1, 1},
	
	{1, 1, 1},
	{1, 0, 1},
	{1, 1, 1},
	{0, 0, 1},
	{1, 1, 1},
};

struct dimension
{
	int w;
	int h;
};
