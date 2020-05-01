#define NES_MIRRORING 1
#include "neslib.h"
#include <string.h>
//#resource "game3.chr"

#include "temp.h"
//#link "temp.s"

#include "Sprites.h" 
#include "rooms.c"
#include "game3.h"

//#link "famitone2.s"
// music and sfx

#define VRAMBUF ((byte*)0x700)


//void setup_graphics() {
//  oam_clear();
 // pal_all(PALETTE);
//}

//void title_screen(void){
 // while(1)
 // {
  //  if(pad_trigger(0)&PAD_START || pad_trigger(0)&PAD_A) {
//	ppu_off(); 
    	//clrscr();
  //  	break;
 // }
//}
//



/* void PlayGame(void){
  
   	ppu_off(); 
  	pal_bg(palette_bg);
	pal_spr(palette_sp);	
	bank_spr(1);
	set_vram_buffer(); 
	clear_vram_buffer();
	
	load_room();
	
	ppu_on_all(); 
	

	while (1){    
		ppu_wait_nmi();
			
		pad1_new = pad_trigger(0); 
		pad1 = pad_state(0);		
		clear_vram_buffer(); 			
		movement();
           	check_spr_objects(); // see which objects are on screen
		sprite_collisions();
		enemy_moves();
		set_scroll_x(scroll_x);
		set_scroll_y(scroll_y);
		draw_screen_R();
		draw_sprites();	
          
          if(Jim.y < 700)
            break;
	}
  
  PlayGame();
  
  
}

*/


void main (void) {
 //  vram_adr(NTADR_A(10,18));
//vram_write("PRESS START!", 12);
  
  //vram_adr(NTADR_A(11,10));
  //vram_write("Jim Jim Ghost Adventure", 9);
  //title_screen();
  //ppu_on_all();
  
  
   //PlayGame();

  	ppu_off(); 
  	pal_bg(palette_bg);
	pal_spr(palette_sp);	
	bank_spr(1);
	set_vram_buffer(); 
	clear_vram_buffer();
	
	load_room();
	
	ppu_on_all(); 
	

	while (1){    
		ppu_wait_nmi();
			
		pad1_new = pad_trigger(0); 
		pad1 = pad_state(0);		
		clear_vram_buffer(); 			
		movement();
           	check_spr_objects(); // see which objects are on screen
		sprite_collisions();
		enemy_moves();
		set_scroll_x(scroll_x);
		set_scroll_y(scroll_y);
		draw_screen_R();
		draw_sprites();		
	}
}




void check_spr_objects(void){
	// mark each object "active" if they are, and get the screen x
	
	

	for(index = 0; index < MAX_ENEMY; ++index){
		enemy_active[index] = 0; //default to zero
		if(enemy_y[index] != TURN_OFF){
			temp5 = (enemy_room[index] << 8) + enemy_actual_x[index];
			enemy_active[index] = get_position();
			enemy_x[index] = temp_x; // screen x coords
		}

	}

	
}
void load_room(void){
	set_data_pointer(Rooms[0]);
	set_mt_pointer(metatiles1); 
	for(y=0; ;y+=0x20){
		for(x=0; ;x+=0x20){
			clear_vram_buffer();
			address = get_ppu_addr(0, x, y);
			index = (y & 0xf0) + (x >> 4);
			buffer_4_mt(address, index); 
			flush_vram_update(VRAMBUF);
			if (x == 0xe0) break;
		}
		if (y == 0xe0) break;
	}
	
	set_data_pointer(Rooms[1]);
	for(y=0; ;y+=0x20){
		x = 0;
		clear_vram_buffer(); 
		address = get_ppu_addr(1, x, y);
		index = (y & 0xf0);
		buffer_4_mt(address, index); 
		flush_vram_update(VRAMBUF);
		if (y == 0xe0) break;
	}
	clear_vram_buffer();
	memcpy (c_map, Rooms[0], 240);
	sprite_obj_init();

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
	
	
	
	
	
	for(index = 0; index < MAX_ENEMY; ++index){
		temp_y = enemy_y[index];
		if(temp_y == TURN_OFF) continue;
		temp1 = enemy_active[index];
		temp2 = enemy_x[index];
		if(temp2 > 0xf0) continue;
		if(temp1 && (temp_y < 0xf0)) {
			sprid = oam_meta_spr(temp2, temp_y, sprid, EnemySpr);
		}
	}
	
	
	
	// 
	
}
	
void movement(void){
	old_x = Jim.x;
	if(pad1 & PAD_LEFT){
		direction = LEFT;
		if(Jim.x <= 0x100) {
			Jim.vel_x = 0;
			Jim.x = 0x100;
		}
		else if(Jim.x < 0x400) {
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
	else { 
		if(Jim.vel_x >= 0x100)Jim.vel_x -= ACCEL;
		else if(Jim.vel_x < -0x100) Jim.vel_x += ACCEL;
		else Jim.vel_x = 0;
	}
	
	Jim.x += Jim.vel_x;
	
	if(Jim.x > 0xf800) { 
		Jim.x = 0x100;
		Jim.vel_x = 0;
	} 
	
	L_R_switch = 1; 
	
	Generic.x = high_byte(Jim.x); 
	Generic.y = high_byte(Jim.y);
	Generic.width = HERO_WIDTH;
	Generic.height = HERO_HEIGHT;
	bg_collision();
	if(collision_R && collision_L){ 
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


	if(Jim.vel_y < 0x300){
		Jim.vel_y += GRAVITY;
	}
	else{
		Jim.vel_y = 0x300; 
	}
	Jim.y += Jim.vel_y;
	
	L_R_switch = 0;
	Generic.x = high_byte(Jim.x);
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
	Generic.y = high_byte(Jim.y); 
	bg_check_low();
	if(collision_D) {
		if(pad1_new & PAD_A) {
			Jim.vel_y = JUMP_VEL; // JUMP
			sfx_play(SFX_JUMP, 0);
		}
	}
	if((scroll_x & 0xff) < 4){
		new_cmap();
	}
	
	temp5 = Jim.x;
	if (Jim.x > MAX_RIGHT){
		temp1 = (Jim.x - MAX_RIGHT) >> 8;
		scroll_x += temp1;
		high_byte(Jim.x) = high_byte(Jim.x) - temp1;
	}

	if(scroll_x >= MAX_SCROLL) {
		scroll_x = MAX_SCROLL; 
		Jim.x = temp5; 
		if(high_byte(Jim.x) >= 0xf1) {
			Jim.x = 0xf100;
		}
	}
}	

void bg_collision(void){
	collision_L = 0;
	collision_R = 0;
	collision_U = 0;
	collision_D = 0;
	
	if(Generic.y >= 0xf0) return;
	
	temp6 = temp5 = Generic.x + scroll_x; 
	temp1 = temp5 & 0xff;
	temp2 = temp5 >> 8;
	
	eject_L = temp1 | 0xf0;
	
	temp3 = Generic.y;
	
	eject_U = temp3 | 0xf0;
	
	if(L_R_switch) temp3 += 2;
	
	bg_collision_sub();
	
	if(collision & COL_ALL){ 
		++collision_L;
		++collision_U;
	}
	
	temp5 += Generic.width;
	temp1 = temp5 & 0xff; 
	temp2 = temp5 >> 8; 
	
	eject_R = (temp1 + 1) & 0x0f;
	
	// temp3 is unchanged
	bg_collision_sub();
	
	if(collision & COL_ALL){ // find a corner in the collision map
		++collision_R;
		++collision_U;
	}
	
	temp3 = Generic.y + Generic.height; //y bottom
	if(L_R_switch) temp3 -= 2; 
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
void enemy_moves(void){
	
	temp1 = high_byte(Jim.x);
	
	if(get_frame_count() & 0x01) return; // half speed
	
	for(index = 0; index < MAX_ENEMY; ++index){
		if(enemy_active[index]){
			if(enemy_x[index] > temp1){
				if(enemy_actual_x[index] == 0) --enemy_room[index];
				--enemy_actual_x[index];
			}
			else if(enemy_x[index] < temp1){
				++enemy_actual_x[index];
				if(enemy_actual_x[index] == 0) ++enemy_room[index];
			}
		}
	}
}
void sprite_collisions(void){

	Generic.x = high_byte(Jim.x);
	Generic.y = high_byte(Jim.y);
	Generic.width = HERO_WIDTH;
	Generic.height = HERO_HEIGHT;


	Generic2.width = ENEMY_WIDTH;
	Generic2.height = ENEMY_HEIGHT;
	
	for(index = 0; index < MAX_ENEMY; ++index){
		if(enemy_active[index]){
			Generic2.x = enemy_x[index];
			Generic2.y = enemy_y[index];
			if(check_collision(&Generic, &Generic2)){
				enemy_y[index] = TURN_OFF;
				sfx_play(SFX_NOISE, 0);
				if(coins) --coins;
			}
		}
	}
}

void draw_screen_R(void){
	pseudo_scroll_x = scroll_x + 0x120;
	
	temp1 = pseudo_scroll_x >> 8;
	
	set_data_pointer(Rooms[temp1]);
	nt = temp1 & 1;
	x = pseudo_scroll_x & 0xff;
		
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
	temp5 -= scroll_x;
	temp_x = temp5 & 0xff;
	if(high_byte(temp5)) return 0;
	return 1;
}
void sprite_obj_init(void){

	pointer = level_1_enemy;
	
	//pointer = level_1_enemies;
	for(index = 0,index2 = 0;index < MAX_ENEMY; ++index){
		
		enemy_x[index] = 0;

		temp1 = pointer[index2]; // get a byte of data
		enemy_y[index] = temp1;
		
		if(temp1 == TURN_OFF) break;

		++index2;
		
		enemy_active[index] = 0;
		
		temp1 = pointer[index2]; // get a byte of data
		enemy_room[index] = temp1;
		
		++index2;
		
		temp1 = pointer[index2]; // get a byte of data
		enemy_actual_x[index] = temp1;
		
		++index2;
	}
	
	for(++index;index < MAX_ENEMY; ++index){
		enemy_y[index] = TURN_OFF;
	}
}
