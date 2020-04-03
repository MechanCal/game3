#define NES_MIRRORING 1
#include "neslib.h"
#include <string.h>
//#resource "game3.chr"

#include "temp.h"
//#link "temp.s"

#include "Sprites.h" 
#include "rooms.c"
#include "game3.h"

/* famitone stuff */
//#link "famitone2.s"
// music and sfx

#define VRAMBUF ((byte*)0x700)

void main (void) {
	
	ppu_off(); 
	
	// load the palettes
	pal_bg(palette_bg);
	pal_spr(palette_sp);
	
	// use the second set of tiles for sprites
	// both bg and sprites are set to 0 by default
	bank_spr(1);
	
	set_vram_buffer(); // do at least once
	clear_vram_buffer();
	
	load_room();
	
	ppu_on_all(); // turn on screen
	
//#link "screens.c"

	while (1){    
		ppu_wait_nmi();
			
		pad1_new = pad_trigger(0); // read the first controller
		pad1 = pad_state(0);
			
		clear_vram_buffer(); // do at the beginning of each frame
			
		// there is a visual delay of 1 frame, so properly you should
		// 1. move user 2.check collisions 3.allow enemy moves 4.draw sprites
		movement();
		// set scroll
		set_scroll_x(scroll_x);
		set_scroll_y(scroll_y);
		draw_screen_R();
		draw_sprites();		
	}
}

void load_room(void){
	set_data_pointer(Rooms[0]);
	set_mt_pointer(metatiles1); 
	for(y=0; ;y+=0x20){
		for(x=0; ;x+=0x20){
			clear_vram_buffer(); // do each frame, and before putting anything in the buffer
			address = get_ppu_addr(0, x, y);
			index = (y & 0xf0) + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			flush_vram_update(VRAMBUF);
			if (x == 0xe0) break;
		}
		if (y == 0xe0) break;
	}
	
	// a little bit in the next room
	set_data_pointer(Rooms[1]);
	for(y=0; ;y+=0x20){
		x = 0;
		clear_vram_buffer(); // do each frame, and before putting anything in the buffer
		address = get_ppu_addr(1, x, y);
		index = (y & 0xf0);
		buffer_4_mt(address, index); // ppu_address, index to the data
		flush_vram_update(VRAMBUF);
		if (y == 0xe0) break;
	}
	clear_vram_buffer();
	
	// copy the room to the collision map
	// the second one should auto-load with the scrolling code
	memcpy (c_map, Rooms[0], 240);
	
}

void draw_sprites(void){
	// clear all sprites from sprite buffer
	oam_clear();

	// reset index into the sprite buffer
	sprid = 0;
	
	// draw 1 hero
	if(direction == LEFT) {
		sprid = oam_meta_spr(high_byte(Jim.x), high_byte(Jim.y), sprid, RoundSprL);
	}
	else{
		sprid = oam_meta_spr(high_byte(Jim.x), high_byte(Jim.y), sprid, RoundSprR);
	}
}
	
void movement(void){
	
// handle x
	old_x = Jim.x;
	
	if(pad1 & PAD_LEFT){
		direction = LEFT;
		if(Jim.x <= 0x100) {
			Jim.vel_x = 0;
			Jim.x = 0x100;
		}
		else if(Jim.x < 0x400) { // don't want to wrap around to the other side
			Jim.vel_x = -0x100;
		}
		else {
			Jim.vel_x -= ACCEL;
			if(Jim.vel_x < -MAX_SPEED) Jim.vel_x = -MAX_SPEED;
		}
	}
	else if (pad1 & PAD_RIGHT){
		
		direction = RIGHT;

		Jim.vel_x += ACCEL;
		if(Jim.vel_x > MAX_SPEED) Jim.vel_x = MAX_SPEED;
	}
	else { // nothing pressed
		if(Jim.vel_x >= 0x100)Jim.vel_x -= ACCEL;
		else if(Jim.vel_x < -0x100) Jim.vel_x += ACCEL;
		else Jim.vel_x = 0;
	}
	
	Jim.x += Jim.vel_x;
	
	if(Jim.x > 0xf800) { // make sure no wrap around to the other side
		Jim.x = 0x100;
		Jim.vel_x = 0;
	} 
	
	L_R_switch = 1; // shinks the y values in bg_coll, less problems with head / feet collisions
	
	Generic.x = high_byte(Jim.x); // this is much faster than passing a pointer to BoxGuy1
	Generic.y = high_byte(Jim.y);
	Generic.width = HERO_WIDTH;
	Generic.height = HERO_HEIGHT;
	bg_collision();
	if(collision_R && collision_L){ // if both true, probably half stuck in a wall
		Jim.x = old_x;
		Jim.vel_x = 0;
	}
	else if(collision_L) {
		Jim.vel_x = 0;
		high_byte(Jim.x) = high_byte(Jim.x) - eject_L;
		
	}
	else if(collision_R) {
		Jim.vel_x = 0;
		high_byte(Jim.x) = high_byte(Jim.x) - eject_R;
	} 	
// handle y
// gravity

	// BoxGuy1.vel_y is signed
	if(Jim.vel_y < 0x300){
		Jim.vel_y += GRAVITY;
	}
	else{
		Jim.vel_y = 0x300; // consistent
	}
	Jim.y += Jim.vel_y;
	
	L_R_switch = 0;
	Generic.x = high_byte(Jim.x); // the rest should be the same
	Generic.y = high_byte(Jim.y);
	bg_collision();
	
	if(collision_U) {
		high_byte(Jim.y) = high_byte(Jim.y) - eject_U;
		Jim.vel_y = 0;
	}
	else if(collision_D) {
		high_byte(Jim.y) = high_byte(Jim.y) - eject_D;
		Jim.y &= 0xff00;
		if(Jim.vel_y > 0) {
			Jim.vel_y = 0;
		}
	}
	// check collision down a little lower than hero
	Generic.y = high_byte(Jim.y); // the rest should be the same
	bg_check_low();
	if(collision_D) {
		if(pad1_new & PAD_A) {
			Jim.vel_y = JUMP_VEL; // JUMP
			sfx_play(SFX_JUMP, 0);
		}
	}
	// do we need to load a new collision map? (scrolled into a new room)
	if((scroll_x & 0xff) < 4){
		new_cmap();
	}
	
// scroll
	temp5 = Jim.x;
	if (Jim.x > MAX_RIGHT){
		temp1 = (Jim.x - MAX_RIGHT) >> 8;
		scroll_x += temp1;
		high_byte(Jim.x) = high_byte(Jim.x) - temp1;
	}

	if(scroll_x >= MAX_SCROLL) {
		scroll_x = MAX_SCROLL; // stop scrolling right, end of level
		Jim.x = temp5; // but allow the x position to go all the way right
		if(high_byte(Jim.x) >= 0xf1) {
			Jim.x = 0xf100;
		}
	}
}	

void bg_collision(void){
	// note, uses bits in the metatile data to determine collision
	// sprite collision with backgrounds
	// load the object's x,y,width,height to Generic, then call this
	

	collision_L = 0;
	collision_R = 0;
	collision_U = 0;
	collision_D = 0;
	
	if(Generic.y >= 0xf0) return;
	
	temp6 = temp5 = Generic.x + scroll_x; // upper left (temp6 = save for reuse)
	temp1 = temp5 & 0xff; // low byte x
	temp2 = temp5 >> 8; // high byte x
	
	eject_L = temp1 | 0xf0;
	
	temp3 = Generic.y; // y top
	
	eject_U = temp3 | 0xf0;
	
	if(L_R_switch) temp3 += 2; // fix bug, walking through walls
	
	bg_collision_sub();
	
	if(collision & COL_ALL){ // find a corner in the collision map
		++collision_L;
		++collision_U;
	}
	
	// upper right
	temp5 += Generic.width;
	temp1 = temp5 & 0xff; // low byte x
	temp2 = temp5 >> 8; // high byte x
	
	eject_R = (temp1 + 1) & 0x0f;
	
	// temp3 is unchanged
	bg_collision_sub();
	
	if(collision & COL_ALL){ // find a corner in the collision map
		++collision_R;
		++collision_U;
	}
	
	
	// again, lower
	
	// bottom right, x hasn't changed
	
	temp3 = Generic.y + Generic.height; //y bottom
	if(L_R_switch) temp3 -= 2; // fix bug, walking through walls
	eject_D = (temp3 + 1) & 0x0f;
	if(temp3 >= 0xf0) return;
	
	bg_collision_sub();
	
	if(collision & COL_ALL){ // find a corner in the collision map
		++collision_R;
	}
	if(collision & (COL_DOWN|COL_ALL)){ // find a corner in the collision map
		++collision_D;
	}
	
	// bottom left
	temp1 = temp6 & 0xff; // low byte x
	temp2 = temp6 >> 8; // high byte x
	
	//temp3, y is unchanged

	bg_collision_sub();
	
	if(collision & COL_ALL){ // find a corner in the collision map
		++collision_L;
	}
	if(collision & (COL_DOWN|COL_ALL)){ // find a corner in the collision map
		++collision_D;
	}

	if((temp3 & 0x0f) > 3) collision_D = 0; // for platforms, only collide with the top 3 pixels

}

void bg_collision_sub(void){
	coordinates = (temp1 >> 4) + (temp3 & 0xf0);
	
	map = temp2&1; // high byte
	if(!map){
		collision = c_map[coordinates];
	}
	else{
		collision = c_map2[coordinates];
	}
	
	collision = is_solid[collision];
}

void draw_screen_R(void){
	// scrolling to the right, draw metatiles as we go
	pseudo_scroll_x = scroll_x + 0x120;
	
	temp1 = pseudo_scroll_x >> 8;
	
	set_data_pointer(Rooms[temp1]);
	nt = temp1 & 1;
	x = pseudo_scroll_x & 0xff;
	
	// important that the main loop clears the vram_buffer
	
	switch(scroll_count){
		case 0:
			address = get_ppu_addr(nt, x, 0);
			index = 0 + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			
			address = get_ppu_addr(nt, x, 0x20);
			index = 0x20 + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			break;
			
		case 1:
			address = get_ppu_addr(nt, x, 0x40);
			index = 0x40 + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			
			address = get_ppu_addr(nt, x, 0x60);
			index = 0x60 + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			break;
			
		case 2:
			address = get_ppu_addr(nt, x, 0x80);
			index = 0x80 + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			
			address = get_ppu_addr(nt, x, 0xa0);
			index = 0xa0 + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			break;
			
		default:
			address = get_ppu_addr(nt, x, 0xc0);
			index = 0xc0 + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
			
			address = get_ppu_addr(nt, x, 0xe0);
			index = 0xe0 + (x >> 4);
			buffer_4_mt(address, index); // ppu_address, index to the data
	}

	++scroll_count;
	scroll_count &= 3; //mask off top bits, keep it 0-3
}

void new_cmap(void){
	// copy a new collision map to one of the 2 c_map arrays
	room = ((scroll_x >> 8) +1); //high byte = room, one to the right
	
	map = room & 1; //even or odd?
	if(!map){
		memcpy (c_map, Rooms[room], 240);
	}
	else{
		memcpy (c_map2, Rooms[room], 240);
	}
}

void bg_check_low(void){

	// floor collisions
	collision_D = 0;
	
	temp5 = Generic.x + scroll_x;    //left
	temp1 = temp5 & 0xff; //low byte
	temp2 = temp5 >> 8; //high byte
	
	temp3 = Generic.y + Generic.height + 1; // bottom
	
	if(temp3 >= 0xf0) return;
	
	bg_collision_sub();
	
	if(collision & (COL_DOWN|COL_ALL)){ // find a corner in the collision map
		++collision_D;
	}
	
	//temp5 = right
	temp5 += Generic.width;
	temp1 = temp5 & 0xff; //low byte
	temp2 = temp5 >> 8; //high byte
	
	//temp3 is unchanged
	bg_collision_sub();
	
	if(collision & (COL_DOWN|COL_ALL)){ // find a corner in the collision map
		++collision_D;
	}
	
	if((temp3 & 0x0f) > 3) collision_D = 0; // for platforms, only collide with the top 3 pixels

}


char get_position(void){
	// is it in range ? return 1 if yes
	
	temp5 -= scroll_x;
	temp_x = temp5 & 0xff;
	if(high_byte(temp5)) return 0;
	return 1;
}

