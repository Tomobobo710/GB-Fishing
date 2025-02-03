#include <gb/gb.h>
#include <rand.h>
#include <string.h>
#include <gbdk/font.h>
#include <stdio.h>
#include <gbdk/console.h>

// Game states
#define STATE_TITLE 0
#define STATE_PLAYING 1
#define STATE_PAUSE 2
#define STATE_CATCH 3

// Screen positions
#define WATER_LINE 85
#define PIER_HEIGHT 75

// Movement limits
#define MIN_PLAYER_X 16
#define MAX_PLAYER_X 144

// Casting constants
#define MAX_CAST_POWER 7
#define MIN_CAST_POWER 1
#define MAX_LINE_SEGMENTS 8
#define GRAVITY 1
#define SCREEN_WIDTH 20
#define SCREEN_HEIGHT 18
#define MAX_REGIONS 8
#define REGION_LAYER_BKG     0x01
#define REGION_LAYER_WINDOW  0x02
#define REGION_LAYER_SPRITE  0x04
#define REGION_PROP_ANIMATED 0x10
#define REGION_PROP_PERSIST  0x20
#define REGION_PROP_PRIORITY 0x40
#define REGION_PROP_TEXT 0x80
#define PIER_END_X 48
#define FISH_INTEREST_RADIUS 16

// Character sprite data (16x16 - made of 4 8x8 sprites)
const unsigned char player_tiles[] = {
    // Standing pose
    // Top Left (head + upper body + arm) - unchanged
    0x00, 0x00, 0x3C, 0x3C, 0x7E, 0x7E, 0x7E, 0x7E,
    0x7E, 0xFF, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E,

    // Top Right (upper rod - moved left)
    0x00, 0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60,
    0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60,

    // Bottom Left (lower body + legs) - unchanged
    0x7E, 0x7E, 0x7E, 0x7E, 0x3C, 0x3C, 0x3C, 0x3C,
    0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x24, 0x24,

    // Bottom Right (lower rod - moved left)
    0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60,
    0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60,

    // Casting pose
    // Top Left (head + upper body tilted back)
    0x00, 0x00, 0x3C, 0x3C, 0x7E, 0x7E, 0xFF, 0xFF,
    0xFF, 0xFF, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E,

    // Top Right (upper rod raised)
    0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,

    // Bottom Left (legs braced)
    0x7E, 0x7E, 0x7E, 0x7E, 0x3C, 0x3C, 0x3C, 0x3C,
    0x7E, 0x7E, 0x3C, 0x3C, 0x3C, 0x3C, 0x66, 0x66,

    // Bottom Right (lower rod raised)
    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,

    // Reeling pose
    // Top Left (head + upper body leaning forward)
    0x00, 0x00, 0x3C, 0x3C, 0x7E, 0x7E, 0x7E, 0x7E,
    0xFF, 0xFF, 0xFF, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E,

    // Top Right (upper rod at angle)
    0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,

    // Bottom Left (legs planted)
    0x7E, 0x7E, 0x7E, 0x7E, 0x3C, 0x3C, 0x7E, 0x7E,
    0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x42, 0x42,

    // Bottom Right (lower rod at angle)
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F
};

// Background tiles
const unsigned char background_tiles[] = {
    // Sky (0)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Cloud (1)
    0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x1E, 0x12, 
    0x3F, 0x21, 0x3F, 0x21, 0x1E, 0x12, 0x0C, 0x0C,

    // Water 1 (2)
    0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x20, 0x20,
    0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x20, 0x20,

    // Water 2 (3)
    0x20, 0x20, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00,
    0x20, 0x20, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00,

    // Pier top (4)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0xFF, 0xFF,

    // Pier post (5)
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,

};

// Fishing line and Lure sprites
const unsigned char fishing_sprites[] = {
    // Line segment (0)
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Lure normal (1)
    0x00, 0x08, 0x08, 0x1C, 0x1C, 0x3E, 0x3E, 0x1C,
    0x1C, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Lure splash (2)
    0x00, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81,
    0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00,

    // Lure bite (3)
    0x00, 0x18, 0x3C, 0x7E, 0x7E, 0x3C, 0x18, 0x00,
    0x00, 0x18, 0x3C, 0x7E, 0x7E, 0x3C, 0x18, 0x00,

    // Fish sprites (4-6)
    0x00, 0x1C, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x3E, 0x3E, 0x7F, 0xFF, 0xFF, 0x7F, 0x3E,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Fish data structure
struct Fish {
    uint8_t active;
    uint8_t x;
    uint8_t y;
    uint8_t type;      // 0=small, 1=medium, 2=large
    uint8_t direction; // 0=left, 1=right
    uint8_t speed;
    uint8_t sprite_id;
    uint8_t hooked;    // New flag for when fish is caught
	uint8_t interested; 
};

// Line segment structure
struct LineSegment {
    uint8_t x;
    uint8_t y;
    uint8_t active;
    uint8_t sprite_id;
};

struct TileAnimation {
    uint8_t num_frames;
    uint8_t *frame_tiles;
    uint8_t current_frame;
    uint8_t frame_delay;
    uint8_t frame_counter;
};

// Update the ScreenRegion struct definition
struct ScreenRegion {
    uint8_t x, y, width, height;
    uint8_t layer;
    uint8_t properties;
    uint8_t tile_start;
    uint8_t tile_count;
    struct TileAnimation *animation;
    void (*update)(struct ScreenRegion*);
};
uint8_t water_frames[] = {130, 131};
struct TileAnimation water_anim = {
    .num_frames = 2,
    .frame_tiles = water_frames,
    .current_frame = 0,
    .frame_delay = 30,
    .frame_counter = 0
};
// Complete game state structure
struct GameState {
    // Core state
    uint8_t state;
    uint8_t frame_counter;
    
    // Player state
    uint8_t player_x;
    uint8_t player_y;
    uint8_t facing_right;
    uint8_t player_state; // 0=standing, 1=casting, 2=reeling
    
    // Casting state
    uint8_t is_casting;
    uint8_t cast_power;
    uint8_t cast_phase;  // 0=charging, 1=flying, 2=landed
    
	uint8_t is_reeling;     // Flag for reeling state
    float reel_speed;       // Speed at which bobber returns
    // Fishing line state
    struct LineSegment line[MAX_LINE_SEGMENTS];
    
    // Bobber state
    uint8_t lure_x;
	uint8_t lure_y;
	int8_t lure_vel_x;
	int8_t lure_vel_y;
	uint8_t lure_state; // 0=normal, 1=splash, 2=bite
    uint8_t splash_timer;
    uint8_t bite_timer;
    
    // Fish states
    struct Fish fish[3];
    uint8_t active_fish;
    uint8_t has_bite;
    
    // Scoring and progression
    uint16_t score;
    uint16_t high_score;
    uint8_t fish_caught;
    uint8_t largest_fish;
	
	struct ScreenRegion regions[MAX_REGIONS];
    
	uint8_t num_regions;
	
	// Input state
	uint8_t prev_input;    // Previous frame's input state
	uint8_t curr_input;    // Current frame's input state
	uint8_t pressed;       // Buttons that were just pressed
	
	uint8_t is_transition;
	
	uint8_t fish_interested;
} game;

void display_gameplay(void);

void display_pause(void);

void remove_pause(void);

void update_animated_region(struct ScreenRegion* region);

void add_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height, 
                uint8_t layer, uint8_t properties, 
                uint8_t tile_start, uint8_t tile_count,
                void (*update)(struct ScreenRegion*));

void clear_regions(void);

// Animation handlers
void animate_player() {
   uint8_t base_tile = 0;
    switch(game.player_state) {
        case 0: // Standing
            base_tile = 0;
            break;
        case 1: // Casting
            base_tile = 4;
            break;
        case 2: // Reeling
            base_tile = 8;
            break;
    }

   if(!game.facing_right) {
       // Swap positions AND flip each sprite
       set_sprite_tile(0, base_tile + 1);
       set_sprite_tile(1, base_tile);
       set_sprite_tile(2, base_tile + 3);
       set_sprite_tile(3, base_tile + 2);
       // Flip all sprites horizontally
       set_sprite_prop(0, S_FLIPX);
       set_sprite_prop(1, S_FLIPX);
       set_sprite_prop(2, S_FLIPX);
       set_sprite_prop(3, S_FLIPX);
   } else {
       set_sprite_tile(0, base_tile);
       set_sprite_tile(1, base_tile + 1);
       set_sprite_tile(2, base_tile + 2);
       set_sprite_tile(3, base_tile + 3);
       // Clear flip properties
       set_sprite_prop(0, 0);
       set_sprite_prop(1, 0);
       set_sprite_prop(2, 0);
       set_sprite_prop(3, 0);
   }
}

void update_player_position() {
   // Update player sprite positions (4 sprites for 16x16)
   uint8_t base_x = game.player_x;
   uint8_t base_y = game.player_y - 3;  // Subtract 3 from Y position
   
   move_sprite(0, base_x, base_y);
   move_sprite(1, base_x + 8, base_y);
   move_sprite(2, base_x, base_y + 8);
   move_sprite(3, base_x + 8, base_y + 8);
}

void init_sprites() {
   // Load all sprite data
   set_sprite_data(0, 12, player_tiles);     // Player sprites (3 poses x 4 tiles)
   set_sprite_data(12, 7, fishing_sprites);  // Fishing line, bobber, and fish

   // Initialize player sprites
   set_sprite_tile(0, 0);
   set_sprite_tile(1, 1);
   set_sprite_tile(2, 2);
   set_sprite_tile(3, 3);

   // Initialize line segment sprites
   for(uint8_t i = 0; i < MAX_LINE_SEGMENTS; i++) {
       set_sprite_tile(4 + i, 12); // Line tile
       game.line[i].sprite_id = 4 + i;
   }

   // Initialize bobber sprite
   set_sprite_tile(12, 13); // Normal bobber tile

   // Initialize fish sprites
   for(uint8_t i = 0; i < 3; i++) {
       set_sprite_tile(13 + i, 16 + i); // Fish tiles
       game.fish[i].sprite_id = 13 + i;
   }
}

void init_background() {
   // Load background tiles at offset 128
   set_bkg_data(128, 6, background_tiles);

   // Draw sky with clouds
	for(uint8_t y = 0; y < 9; y++) {
		for(uint8_t x = 0; x < 20; x++) {
			// Place clouds at y=2 and y=3, spread across x positions
			if((y == 2 && (x == 3 || x == 11 || x == 18)) ||
			   (y == 3 && (x == 7 || x == 15))) {
				set_bkg_tile_xy(x, y, 129); // Cloud (using offset 128 + 1)
			} else {
				set_bkg_tile_xy(x, y, 128); // Sky (using offset 128 + 0)
			}
		}
	}

   // Update other tile references to use offset
   // Draw pier
   for(uint8_t x = 0; x < 6; x++) {
       set_bkg_tile_xy(x, 9, 132);  // Pier top (128 + 4)
       for(uint8_t y = 10; y < 14; y++) {
           if(x == 0 || x == 5) {
               set_bkg_tile_xy(x, y, 133); // Pier posts (128 + 5)
           }
       }
   }

   // Draw water
   for(uint8_t y = 10; y < 18; y++) {
       for(uint8_t x = 0; x < 20; x++) {
           if(x < 6 && y == 10) continue; // Skip pier area
           set_bkg_tile_xy(x, y, 130 + (y % 2)); // Water patterns (128 + 2 or 3)
       }
   }
}

void handle_fish() {
    for(uint8_t i = 0; i < 3; i++) {
        struct Fish *fish = &game.fish[i];
        
        if(fish->active) {
			if(!fish->hooked) {
				// Calculate distance to lure
				int16_t dx = (int16_t)fish->x - (int16_t)game.lure_x;
				if(dx < 0) dx = -dx;
				int16_t dy = (int16_t)fish->y - (int16_t)game.lure_y;
				if(dy < 0) dy = -dy;
				
				// Fish maintains interest if it was already interested, or gains interest if no other fish is interested
				if((fish->interested || (!game.fish_interested && game.cast_phase == 2 && 
					dx < FISH_INTEREST_RADIUS && dy < FISH_INTEREST_RADIUS))) {
					
					// Set both flags
					fish->interested = 1;
					game.fish_interested = 1;
					
					// Slow approach to lure
					if(fish->x < game.lure_x) fish->x++;
					if(fish->x > game.lure_x) fish->x--;
					if(fish->y < game.lure_y) fish->y++;
					if(fish->y > game.lure_y) fish->y--;
					
					// Lost interest if too far from lure
					if(dx > FISH_INTEREST_RADIUS + 10 || dy > FISH_INTEREST_RADIUS + 10) {
						fish->interested = 0;
						game.fish_interested = 0;
					}
					
					// Check if close enough to bite and player is reeling
					if(dx < 16 && dy < 16 && game.is_reeling) {
						fish->hooked = 1;
						game.score += 10;
						
						// Make other fish just lose interest, but keep swimming normally
						for(uint8_t j = 0; j < 3; j++) {
							if(j != i && game.fish[j].active) {
								game.fish[j].interested = 0;
							}
						}
					}
				} else {
					// Normal swimming behavior
					if(game.frame_counter % (2 + fish->type) == 0) {
						uint8_t next_x = fish->x + (fish->direction ? fish->speed : -fish->speed);
						
						if(next_x < 16 || next_x > 152) {
							fish->direction = !fish->direction;
							fish->x += (fish->direction ? 4 : -4);
						} else {
							fish->x = next_x;
						}
						
						if(game.frame_counter % 30 == 0) {
							int8_t y_move = (rand() % 3) - 1;
							if(fish->y + y_move >= WATER_LINE + 10 && 
							   fish->y + y_move <= WATER_LINE + 50) {
								fish->y += y_move;
							}
						}
					}
				}
			} else {
                // Hooked fish behavior
				if(game.is_reeling) {
					fish->x = game.lure_x;
					fish->y = game.lure_y;
				} else {
					if(game.frame_counter % 8 == 0) {
						fish->x += (rand() % 3) - 1;
						fish->y += (rand() % 3) - 1;
						
						// This is where the escape check goes
						if((rand() % 100) < 2) {
							fish->hooked = 0;
							fish->active = 0;
							game.fish_interested = 0;
						}
					}
				}
            }
            
            move_sprite(fish->sprite_id, fish->x, fish->y);
        } else {
            // Spawn logic (same as before)
            if((uint8_t)rand() % 60 == 0) {
				fish->active = 1;
				fish->hooked = 0;
				fish->interested = 0;
				fish->direction = rand() % 2;
				fish->x = fish->direction ? 30 : 130;
				fish->y = WATER_LINE + 10 + (rand() % 30);
				fish->type = rand() % 3;
				
				// More varied and slower speeds
				// Base speed of 0.5 to 1 (we'll only move every other frame or so)
				fish->speed = (rand() % 2) + 1;  // 1 or 2
				
				// Bigger fish move a bit slower
				if(fish->type > 0) {
					fish->speed = 1;  // Medium and large fish always move slower
				}
			}
        }
    }
}

void update_line() {
   if(game.is_casting) {
        uint8_t start_x = game.player_x + (game.facing_right ? 12 : -4);
        uint8_t start_y = game.player_y + 4;
        
        // Calculate line segment positions
        for(uint8_t i = 0; i < MAX_LINE_SEGMENTS; i++) {
            struct LineSegment *seg = &game.line[i];
            
            // Linear interpolation between rod and bobber
            seg->x = start_x + ((game.lure_x - start_x) * i) / (MAX_LINE_SEGMENTS - 1);
            seg->y = start_y + ((game.lure_y - start_y) * i) / (MAX_LINE_SEGMENTS - 1);
            
            if(game.cast_phase == 2) {
                // Simple wave effect when in water
                seg->y += (i & 1) ? 1 : -1;
            }
            
            move_sprite(seg->sprite_id, seg->x, seg->y);
            seg->active = 1;
        }
    } else {
       // Hide line segments
       for(uint8_t i = 0; i < MAX_LINE_SEGMENTS; i++) {
           move_sprite(game.line[i].sprite_id, 0, 0);
           game.line[i].active = 0;
       }
   }
}

void update_lure() {
   uint8_t next_x;  // Declare at start of function
   
   if(game.is_casting) {
       switch(game.cast_phase) {
           case 0: // Starting cast
               game.lure_x = game.player_x + (game.facing_right ? 12 : -4);
               game.lure_y = game.player_y + 4;
               break;
               
           case 1: // Flying through air
               next_x = game.lure_x + game.lure_vel_x;
               
               // Screen boundary checks
               if(next_x < 8 || (game.lure_vel_x < 0 && game.lure_x <= 8)) {
                   game.lure_x = 8;
                   game.lure_vel_x = 0;
               } else if(next_x > 160 || (game.lure_vel_x > 0 && game.lure_x >= 160)) {
                   game.lure_x = 160;
                   game.lure_vel_x = 0;
               } else {
                   game.lure_x = next_x;
               }
               
               if(game.lure_y + game.lure_vel_y < 0) {
                   game.lure_y = 0;
                   game.lure_vel_y = 0;
               } else {
                   game.lure_y += game.lure_vel_y;
               }
               
               game.lure_vel_y += GRAVITY;
               
               if(game.lure_y >= WATER_LINE) {
                   game.lure_y = WATER_LINE;
                   game.lure_vel_y = 0;
                   game.lure_vel_x = 0;
                   game.cast_phase = 2;
                   game.splash_timer = 30;
                   game.lure_state = 1; // Just splash animation
               }
               break;
               
           case 2: // In water
               if(!game.is_reeling) {
                   // Add slow sinking when not reeling
                   if(game.frame_counter % 4 == 0) {
                       if(game.lure_y < WATER_LINE + 50) {
                           game.lure_y++;
                       }
                   }
               }
               
               // Update splash animation
               if(game.splash_timer) {
                   game.splash_timer--;
                   if(!game.splash_timer) {
                       game.lure_state = 0; // Normal bobber
                   }
               }
               break;
       }
       
       move_sprite(12, game.lure_x, game.lure_y);
       set_sprite_tile(12, 13 + game.lure_state);
   } else {
       move_sprite(12, 0, 0);
   }
}

void start_cast() {
   game.is_casting = 1;
   game.cast_phase = 1;
   game.player_state = 1;
   
   // Reduce velocities significantly (divided by roughly 100x)
   game.lure_vel_x = game.cast_power * (game.facing_right ? 1 : -1);
   game.lure_vel_y = -game.cast_power;
   
   game.lure_x = game.player_x + (game.facing_right ? 12 : -4);
   game.lure_y = game.player_y + 4;
   game.lure_state = 0;
}
void handle_catch() {
    if(game.is_reeling) {
        // Calculate direction to player
        int16_t target_x = game.player_x + (game.facing_right ? 12 : -4);
        int16_t target_y = game.player_y + 4;
        
        int16_t dx = target_x - game.lure_x;
        int16_t dy = target_y - game.lure_y;
        
        int16_t abs_dx = dx > 0 ? dx : -dx;
        int16_t abs_dy = dy > 0 ? dy : -dy;
        int16_t manhattan_dist = abs_dx + abs_dy;
        
        if(manhattan_dist < 8) {  // Close enough to player
			// Reset fishing states
			game.is_casting = 0;
			game.is_reeling = 0;
			game.cast_phase = 0;
			game.player_state = 0;
			
			// Handle only the hooked fish
			for(uint8_t i = 0; i < 3; i++) {
				if(game.fish[i].active && game.fish[i].hooked) {
					// Add to score based on fish type
					game.score += (game.fish[i].type + 1) * 100;
					game.fish_caught++;
					if(game.fish[i].type > game.largest_fish) {
						game.largest_fish = game.fish[i].type;
					}
					
					// Deactivate just this fish
					game.fish[i].active = 0;
					game.fish[i].hooked = 0;
					move_sprite(game.fish[i].sprite_id, 0, 0); // Hide this fish sprite
					
					// Spawn a new fish in this same slot
					game.fish[i].active = 1;
					game.fish[i].interested = 0;
					game.fish[i].direction = rand() % 2;
					game.fish[i].x = game.fish[i].direction ? 30 : 130;
					game.fish[i].y = WATER_LINE + 10 + (rand() % 30);
					game.fish[i].type = rand() % 3;
					game.fish[i].speed = (rand() % 2) + 1;
					if(game.fish[i].type > 0) {
						game.fish[i].speed = 1;
					}
					
					break;  // We found and handled the hooked fish, no need to continue
				}
			}
			
			game.fish_interested = 0;  // Reset the interest flag
			
			// Hide bobber and line
			move_sprite(12, 0, 0);
			for(uint8_t i = 0; i < MAX_LINE_SEGMENTS; i++) {
				move_sprite(game.line[i].sprite_id, 0, 0);
			}
		} else {
            // Continue reeling in motion
            if(abs_dx > abs_dy) {
                if(dx > 0) game.lure_x += 1;
                if(dx < 0) game.lure_x -= 1;
                
                if(manhattan_dist > 16) {
                    if(dy > 0) game.lure_y += 1;
                    if(dy < 0) game.lure_y -= 1;
                }
            } else {
                if(dy > 0) game.lure_y += 1;
                if(dy < 0) game.lure_y -= 1;
                
                if(manhattan_dist > 16) {
                    if(dx > 0) game.lure_x += 1;
                    if(dx < 0) game.lure_x -= 1;
                }
            }
        }
    }
}

void update_power_meter() {
   if(game.cast_power > 0) {
       // Draw power meter background
       for(uint8_t i = 0; i < 10; i++) {
           move_sprite(23 + i, 20 + (i * 8), 140);
       }
       
       // Fill meter based on power
       uint8_t filled = (game.cast_power * 10) / MAX_CAST_POWER;
       for(uint8_t i = 0; i < 10; i++) {
           if(i < filled) {
               set_sprite_tile(23 + i, 12); // Filled segment
           } else {
               set_sprite_tile(23 + i, 15); // Empty segment
           }
       }
   } else {
       // Hide power meter
       for(uint8_t i = 0; i < 10; i++) {
           move_sprite(23 + i, 0, 0);
       }
   }
}

void draw_score() {
    gotoxy(1, 1);
    printf("SCORE:%d", game.score);
}

void update_input() {
    game.prev_input = game.curr_input;
    game.curr_input = joypad();
    game.pressed = (game.curr_input ^ game.prev_input) & game.curr_input;
}

void handle_input() {
    switch(game.state) {
        case STATE_TITLE:
            if(game.pressed & J_START) {
                game.state = STATE_PLAYING;
                game.is_transition = 1;  // Set transition flag
                display_gameplay();
            }
            break;
            
        case STATE_PLAYING:
            if(!game.is_casting && !game.is_reeling) {  // Not casting or reeling - can move and cast
                // Movement controls
                if(game.curr_input & J_LEFT && game.player_x > MIN_PLAYER_X) {
					game.player_x--;
					game.facing_right = 0;
				}
				if(game.curr_input & J_RIGHT && game.player_x < PIER_END_X) {  // Changed this line
					game.player_x++;
					game.facing_right = 1;
				}
							
                // Casting charge-up
                if(game.curr_input & J_A) {
					if(game.curr_input & J_A) {
						if(game.cast_power < MAX_CAST_POWER && (game.frame_counter % 6 == 0)) {  // Only increase every 6 frames
							game.cast_power++;
							game.player_state = 1;
						}
					}
				} else if(game.cast_power > 0) {
					if(game.cast_power >= MIN_CAST_POWER) {
						start_cast();
					}
					game.cast_power = 0;
				}
            } else if(game.is_casting) {  // Line is out
                if(game.curr_input & J_B) {  // While holding B
                    game.is_reeling = 1;
                    game.player_state = 2;  // Reeling animation
                } else {
                    game.is_reeling = 0;
                    if(game.cast_phase == 2) {  // If line is in water
                        game.player_state = 0;  // Back to standing
                    }
                }
            }
			if(game.pressed & J_START) {
				game.state = STATE_PAUSE;
				game.is_transition = 1;
				display_pause();
			}
            break;
            
        case STATE_PAUSE:
            if((game.pressed & J_START) && !game.is_transition) {
                game.state = STATE_PLAYING;
                game.is_transition = 1;
                remove_pause();
            }
            break;
    }

    // Clear transition flag when START is released
    if(!(game.curr_input & J_START)) {
        game.is_transition = 0;
    }
}

void display_title() {
    clear_regions();
    
    // Load background tiles at offset 128
    set_bkg_data(128, 6, background_tiles);

    // Initialize font system after background tiles
    font_t min_font;
    font_init();
    min_font = font_load(font_min); // This loads at tile 0
    font_set(min_font);

    // Add regions (keep these the same)
    add_region(0, 0, SCREEN_WIDTH, 10,
              REGION_LAYER_BKG, 0,
              128, 1, NULL);  // Sky uses tile 128

    add_region(0, 10, SCREEN_WIDTH, 8,
          REGION_LAYER_BKG, REGION_PROP_ANIMATED,
          130, 2, update_animated_region);  // Water uses tiles 130-131
	game.regions[game.num_regions-1].animation = &water_anim;
    // Draw initial background using offset tiles
    for(uint8_t y = 0; y < SCREEN_HEIGHT; y++) {
        for(uint8_t x = 0; x < SCREEN_WIDTH; x++) {
            if(y < 10) {
                set_bkg_tile_xy(x, y, 128);  // Sky tile
            } else {
                set_bkg_tile_xy(x, y, 130 + (y % 2));  // Water tiles start at 130
            }
        }
    }

    // Add and draw text regions
    add_region(6, 4, 7, 1, REGION_LAYER_BKG, REGION_PROP_TEXT, 0, 0, NULL);
    add_region(5, 8, 10, 1, REGION_LAYER_BKG, REGION_PROP_TEXT, 0, 0, NULL);
    add_region(7, 11, 10, 1, REGION_LAYER_BKG, REGION_PROP_TEXT, 0, 0, NULL);

    gotoxy(6, 4);
    printf("FISHING");
    gotoxy(5, 8);
    printf("PRESS START");
    gotoxy(7, 11);
    printf("SCORE:%d", game.high_score);
}

void display_gameplay() {
    clear_regions();
    
    // Load background tiles at offset 128
    set_bkg_data(128, 6, background_tiles);



    // Add regions for gameplay state (same as before)
    add_region(0, 0, SCREEN_WIDTH, 9,
              REGION_LAYER_BKG, 0,
              128, 1, NULL);  // Sky

    add_region(0, 10, SCREEN_WIDTH, 8,
              REGION_LAYER_BKG, REGION_PROP_ANIMATED,
              130, 2, update_animated_region);  // Water
    game.regions[game.num_regions-1].animation = &water_anim;

    // Score region
    add_region(1, 1, 10, 1,
              REGION_LAYER_BKG, REGION_PROP_TEXT | REGION_PROP_PERSIST,
              0, 0, NULL);

    // Draw initial background
    init_background();

    // Draw initial score
    gotoxy(1, 1);
    printf("SCORE:%d", game.score);
}

// Update function to handle all regions
void update_regions() {
    // Update all regions every frame
    for(uint8_t i = 0; i < game.num_regions; i++) {
        struct ScreenRegion* region = &game.regions[i];
        if(region->update && (region->properties & REGION_PROP_ANIMATED)) {
            region->update(region);
        }
    }
}

void display_pause() {
    // Add pause text region
    add_region(6, 8, 9, 1,
              REGION_LAYER_BKG, REGION_PROP_TEXT | REGION_PROP_PERSIST,
              0, 0, NULL);
              
    gotoxy(6, 8);
    printf(" PAUSED ");
}

void remove_pause() {
    // Remove the pause text region
    if(game.num_regions > 0) {
        game.num_regions--;
    }
    
    // Redraw the background where the pause text was
    for(uint8_t y = 8; y < 9; y++) {
        for(uint8_t x = 6; x < 15; x++) {
            set_bkg_tile_xy(x, y, 128);  // Sky tile
        }
    }
}

void init_game_state() {
   // Clear full game state
   memset(&game, 0, sizeof(game));
   
   // Set initial values
   game.state = STATE_TITLE;
   game.player_x = 50;
   game.player_y = PIER_HEIGHT;
   game.facing_right = 1;
   game.is_reeling = 0;
	game.reel_speed = 2.0;
   // Initialize fish array
   for(uint8_t i = 0; i < 3; i++) {
       game.fish[i].active = 0;
       game.fish[i].sprite_id = 13 + i;
   }
   
   // Initialize line segments
   for(uint8_t i = 0; i < MAX_LINE_SEGMENTS; i++) {
       game.line[i].active = 0;
       game.line[i].sprite_id = 4 + i;
   }
}

void update_animated_region(struct ScreenRegion* region) {
    struct TileAnimation *anim = region->animation;
    if(!anim) return;
    
    anim->frame_counter++;
    if(anim->frame_counter >= anim->frame_delay) {
        anim->frame_counter = 0;
        anim->current_frame = (anim->current_frame + 1) % anim->num_frames;
        
        uint8_t current_tile = anim->frame_tiles[anim->current_frame];
        uint8_t row_tiles[SCREEN_WIDTH];  // Buffer for a row of tiles
        
        // Fill buffer with the current animation frame tile
        for(uint8_t x = 0; x < region->width; x++) {
            row_tiles[x] = current_tile;
        }

        // Update each row at once
        for(uint8_t y = region->y; y < region->y + region->height; y++) {
            // Simple check if this row contains text
            uint8_t skip_row = 0;
            for(uint8_t i = 0; i < game.num_regions; i++) {
                struct ScreenRegion* r = &game.regions[i];
                if((r->properties & REGION_PROP_TEXT) && 
                   y >= r->y && y < r->y + r->height) {
                    skip_row = 1;
                    break;
                }
            }
            
            if(!skip_row) {
                // Update entire row at once
                set_bkg_tiles(region->x, y, region->width, 1, row_tiles);
            }
        }
    }
}

void add_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height, 
                uint8_t layer, uint8_t properties, 
                uint8_t tile_start, uint8_t tile_count,
                void (*update)(struct ScreenRegion*)) {
    if(game.num_regions < MAX_REGIONS) {
        struct ScreenRegion* region = &game.regions[game.num_regions++];
        region->x = x;
        region->y = y;
        region->width = width;
        region->height = height;
        region->layer = layer;
        region->properties = properties;
        region->tile_start = tile_start;
        region->tile_count = tile_count;
        region->update = update;
    }
}

void clear_regions() {
    game.num_regions = 0;
}

void update_game() {
    update_player_position();
    animate_player();
    
    if(game.is_casting || game.is_reeling) {  // Keep updating line and bobber while either casting or reeling
        update_lure();
        update_line();
        if(game.is_reeling) {
            handle_catch();  // Process reeling animation
        }
    }
    
    handle_fish();
    update_power_meter();
    draw_score();
}

void check_high_score() {
   if(game.score > game.high_score) {
       game.high_score = game.score;
   }
}

void init() {
   // Initialize random number generator
   initrand(DIV_REG);
   
   // Set up graphics system
   SPRITES_8x8;
   
   // Initialize all game components
   init_sprites();
   init_game_state();
   
   // Show display layers
   SHOW_BKG;
   SHOW_SPRITES;
}

void main() {
    init();
    display_title();
    
    while(1) {
		update_input();
        handle_input();
        
        switch(game.state) {
            case STATE_TITLE:
                update_regions();
                break;
                
            case STATE_PLAYING:
                update_game();
                update_regions();
                break;
                
            case STATE_PAUSE:
                update_regions();
                break;
                
            case STATE_CATCH:
                update_game();
                update_regions();
                handle_catch();
                game.state = STATE_PLAYING;
                break;
        }
        
        game.frame_counter++;
        
        if(game.state == STATE_CATCH) {
            check_high_score();
        }
        
        wait_vbl_done();
    }
}