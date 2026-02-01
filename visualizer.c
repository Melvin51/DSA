// NOTE: origin point is bottom-left of elements drawn

#include "cross_boundary_data.h"
#include "visualizer.h"

#define BASE_10 10
#define MAX_DIGITS 20

static struct dimension value_bitmap_size = {4,4};

enum colors
{
	RED = 0x00FF0000,
	GREEN = 0x0000FF00,
	BLUE = 0x000000FF,
	BLACK = 0x00000000,
		
	COLORS_COUNT = 4
};

static void DrawRectangle(struct frame_buffer *game_buffer, struct coords pos, 
						  struct dimension dim, int color)
{
	u8 cancel_draw = 0;
	
	//Clamp dimensions to fit within the frame buffer
	{
		if(pos.x + dim.w > game_buffer->width)
		{
			pos.x = game_buffer->width - dim.w;
		}
		
		if(pos.y + dim.h > game_buffer->height)
		{
			cancel_draw = 1;
			pos.y = game_buffer->height - dim.h;
		}
		
		if(pos.y < 0)
		{
			cancel_draw = 1;
			pos.y = 0 + dim.h;
		}		
	}
	
	if(cancel_draw == 0)
	{
		int *row_pos = game_buffer->memory;
		row_pos = row_pos + game_buffer->width * (int)pos.y;
		
		for(int y = pos.y; y < (pos.y + dim.h); y++)
		{
			int *col_pos = row_pos + (int)pos.x;
			for(int x = pos.x; x < (pos.x + dim.w); x++)
			{
				*col_pos++ = color; // A.R.G.B
			}
			
			row_pos = row_pos + game_buffer->width;
		}		
	}
}

static void DrawTile(struct frame_buffer *game_buffer, int tile_number,
					 struct coords pos, struct dimension dim)
{
	if(tile_number < 0 || tile_number > 9)
	{
		tile_number = 0;
	}
	
	if(tile_number > 9)
	{
		tile_number = 9;
	}
	
	struct coords tmp = pos;
	char *tile_map = digits[tile_number * 5];
	
	for(int row = 0; row < 5; row++)
	{
		for(int col = 0; col < 3; col++)
		{
			if(*tile_map++)
			{
				DrawRectangle(game_buffer, tmp, dim, 0xAAFF00FF);
			}
			
			tmp.x += dim.w;
		}
		
		tmp.y -= dim.h;
		tmp.x = pos.x;
	}
}

static void DrawU64(struct frame_buffer *game_buffer, struct coords pos, struct dimension size, u64 value)
{
	struct dimension dims = size;
	u8 digits[MAX_DIGITS] = {0};
	u8 order_magnitude = 1;
	

	// Clamp the y coord to not draw past frame buffer.
	// coords of a rectangle element in a tile are bottom-left based
	if((pos.y - dims.h * 4) < 0)
	{
		pos.y = dims.h * 4;
	}
	if(pos.y >= game_buffer->height)
	{
		pos.y = game_buffer->height - dims.h;
	}

	
	DETERMINE_DECIMAL_MAGNITUDE(value, order_magnitude);
	
	u8 idx = MAX_DIGITS - 1;
	
	for(int i = 0; i < MAX_DIGITS; i++)
	{
		digits[idx--] = (value % BASE_10);
		value = value / BASE_10;
	}
	
	for(int i = order_magnitude; i > 0; i--)
	{
		DrawTile(game_buffer, digits[MAX_DIGITS - i], pos, dims);
		pos.x += dims.w * 4;
	}
}

static void DrawS32(struct frame_buffer *game_buffer, struct coords pos, struct dimension size, int value)
{
	struct dimension dims = size;
	u8 digits[BASE_10] = {0};
	u8 idx = BASE_10 - 1;
	u8 order_magnitude = 1;
	
	// Clamp the y coord to not draw past frame buffer.
	// coords of a rectangle element in a tile are bottom-left based
#if 0
	if((pos.y - dims.h * 4) < 0)
	{
		pos.y = dims.h * 4;
	}
	if((pos.y + dims.h * 4) >= game_buffer->height)
	{
		pos.y = game_buffer->height - dims.h;
	}
#endif
	
	if(value & (1 << 31)) // check sign of value
	{
		struct coords sign_pos = pos;
		sign_pos.x -= dims.w * 2 + 5;
		sign_pos.y -= (dims.h * 4) / 2;
		DrawRectangle(game_buffer, sign_pos, (struct dimension){20.f, 5.f}, 0xAAFF00FF); // Draw sign
		
		value = ~value + 1; // Get the value ignoring sign (2's comeplement = 1's comeplement representation + 1)
	}
	
	DETERMINE_DECIMAL_MAGNITUDE(value, order_magnitude);
	
	for(int i = 0; i < MAX_DIGITS; i++)
	{
		digits[idx--] = (value % BASE_10);
		value = value / BASE_10;
	}
	
	for(int i = order_magnitude; i > 0; i--)
	{
		DrawTile(game_buffer, digits[BASE_10 - i], pos, dims);
		pos.x += dims.w * 4;
	}
}

union r32
{
	float flt;
	u32 bit_field;
};

// TODO: Fix Drawing floats
static void DrawF32(struct frame_buffer *game_buffer, struct coords pos, float flt_val)
{
	union r32 value = {flt_val};
	
	struct dimension dims = {6, 6};
	
	if((pos.y - dims.h * 4) < 0)
	{
		pos.y = dims.h * 4; // 4 = number of tiles
	}
	if(pos.y >= game_buffer->height)
	{
		pos.y = game_buffer->height - dims.h;
	}
	
	u8 digits[MAX_DIGITS] = {0};
	u8 idx = MAX_DIGITS - 1;
	
	if(value.bit_field & (1 << 31)) // check sign of value
	{
		struct coords sign_pos = pos;
		sign_pos.x -= dims.w * 2 + dims.w * 2;
		sign_pos.y -= (dims.h * 4) / 2;
		DrawRectangle(game_buffer, sign_pos, (struct dimension){20.f, 5.f}, 0xAAFF00FF);
	}
	
	u32 exp = ((value.bit_field & (255 << 23)) >> 23);
	u32 frac = (value.bit_field & (0xFFFFFFFF >> 9)); // base 2
	char biased_exp = exp - 127;
		
	struct coords decimal_point = pos;
	decimal_point.x += dims.w * 4;
	decimal_point.y -= dims.h * 4;
	
	if(biased_exp < 0) 
	{
		DrawS32(game_buffer, pos, value_bitmap_size, 0);
		DrawRectangle(game_buffer, decimal_point, (struct dimension){5.f, 5.f}, 0xAAFF00FF);
		
		struct coords fraction_position = decimal_point;
		fraction_position.x += dims.w * 2;
		fraction_position.y = pos.y;
		
		u32 base10_signifcand = 22;
		
		DrawS32(game_buffer, fraction_position, value_bitmap_size, base10_signifcand);
	}
}


static void DrawTargetData(struct frame_buffer *game_buffer, struct migrating_data game_state, float input_size)
{
	float input_size_in_pixels = game_buffer->width / input_size;
	float empty_space = input_size_in_pixels - 1; // empty space must be <= size in pixels otherwise the width dimension ends up 0 or negative, so nothing will draw

	if(input_size < 0)
	{
		input_size = -input_size;
	}
	
	struct coords input_pos = {0, game_buffer->height / 2};
	struct dimension input_dim = {input_size_in_pixels - empty_space, 0};
	int starting_color = 0x00FF0000;
	int color = starting_color;
	
	while(input_size--)
	{
		input_dim.h = game_state.clock_cycles * 50;
		
		if((input_pos.x + input_dim.w) > game_buffer->width || 
		   (input_pos.y + input_dim.h) > game_buffer->height)
		{
			break;
		}
		   
		DrawRectangle(game_buffer, input_pos , input_dim, color);
		
		input_pos.x += input_dim.w + empty_space;
		
		if(color)
		{
			color = (color >> 8);
		}
		else
		{
			color = starting_color;
		}
	}
}

DrawGrid(struct frame_buffer *game_buffer, u32 rows, u32 cols, int color, u8 draw_line_values)
{
#define GRID_LINE_SIZE 1
	
	struct dimension grid_row_dims = {game_buffer->width, GRID_LINE_SIZE};
	struct dimension grid_cols_dims = {GRID_LINE_SIZE, game_buffer->height};
	
	struct coords row_pos = {0, game_buffer->height - grid_row_dims.h};
	struct coords col_pos = {0};
	
	u32 row_dt = game_buffer->height / rows;
	u32 col_dt = game_buffer->width / cols;
	
	while(rows--)
	{
		DrawRectangle(game_buffer, row_pos, grid_row_dims, color);
		
		if(draw_line_values)
		{
			struct coords line_value_pos = row_pos;
			line_value_pos.y -= 10;
			DrawS32(game_buffer, line_value_pos, value_bitmap_size, row_pos.y);			
		}
		
		row_pos.y -= row_dt;
		if(row_pos.y < 0)
		{
			row_pos.y = grid_row_dims.h;
			DrawRectangle(game_buffer, row_pos, grid_row_dims, color);
		}
	}
	
	while(cols--)
	{
		DrawRectangle(game_buffer, col_pos, grid_cols_dims, color);
		
		if(draw_line_values)
		{
			struct coords line_value_pos = col_pos;
			line_value_pos.y = 30;
			line_value_pos.x += 5;			
			DrawS32(game_buffer, line_value_pos, value_bitmap_size, col_pos.x);
		}
		
		col_pos.x += col_dt;		
		if(col_pos.x > game_buffer->width - grid_cols_dims.w)
		{
			col_pos.x = game_buffer->width - grid_cols_dims.w;
			DrawRectangle(game_buffer, col_pos, grid_cols_dims, color);
		}
	}
	
	//Draw Grid Boundaries
	row_pos.y = grid_row_dims.h;
	DrawRectangle(game_buffer, row_pos, grid_row_dims, color);
	
	col_pos.x = game_buffer->width - grid_cols_dims.w;
	DrawRectangle(game_buffer, col_pos, grid_cols_dims, color);
}

void VisualizerMain(struct frame_buffer *game_buffer, struct migrating_data *game_state)
{
	struct dimension dims2 = {30.f, 30.f};
	int *paint_pointer = (int *)game_buffer->memory;
	
	for(int y = 0; y < game_buffer->height; y++)
	{
		for(int x = 0; x < game_buffer->width; x++)
		{
			*paint_pointer++ = 0x002F2F2F; //A.R.G.B
		}
	}
	
	game_state->square.x += 15 * game_state->frame_time;
	game_state->square.y += -50 * game_state->frame_time;
	
	if((game_state->square.x + dims2.w) > game_buffer->width || 
	   game_state->square.x < 0)
	{
		game_state->square.x = 0;
	}
	
	if(game_state->square.y < 0)
	{
		game_state->square.y = game_buffer->height / 2 + 600;
	}
	
	// Draw Calls

	DrawGrid(game_buffer, 12, 12, RED, 0);
	
	DrawRectangle(game_buffer, game_state->square, dims2, GREEN);
	DrawS32(game_buffer, (struct coords){10, 20}, value_bitmap_size, game_state->square.x);
	DrawS32(game_buffer, (struct coords){100, 20}, value_bitmap_size, game_state->square.y);
	
	//DrawS32(game_buffer, (struct coords){140, 50}, value_bitmap_size, game_buffer->width);
	//DrawS32(game_buffer, (struct coords){240, 50}, value_bitmap_size, game_buffer->height);
	
	DrawS32(game_buffer, (struct coords){game_buffer->width - 50, game_buffer->height - 10}, value_bitmap_size, game_state->frame_time * milliseconds);
	DrawU64(game_buffer, (struct coords){game_buffer->width - 50, game_buffer->height - 40}, value_bitmap_size, game_state->clock_cycles);
	//DrawTargetData(game_buffer, game_state, 400);
}

int _DllMainCRTStartup(){return 1;} // _DllMainCRTStartup must return true
int _fltused;