/******************************************************************
 * winv2: g_engine.cpp                                            *
 * copyright (c) 2001 vecna                                       *
 ******************************************************************/

#include "xerxes.h"

/****************************** data ******************************/

Entity *entity[256];
int entities=0, player;
int cameratracking=1, tracker=0;
MAP *current_map;
Entity *myself;
int xwin, ywin;
bool done;

/****************************** code ******************************/

Entity::Entity()
{
	x = 0;
	y = 0;
	waypointx = 0;
	waypointy = 0;
	chr = 0;
	delay = 0;
	follow = 0;
	follower = 0;
	setspeed(100);
	face = SOUTH;
	framect = 0;
	specframe = 0;
	movestr[0] = 0;
	moveofs = 0;
	movecode = 0;
	visible = true;
}

Entity::~Entity() { return; }
int Entity::getx() { return x / 65535; }
int Entity::gety() { return y / 65535; }

void Entity::setxy(int x1, int y1)
{
	x = x1 * 65535;
	y = y1 * 65535;
	set_waypoint(x1, y1);
}

int Entity::getspeed() { return speed; }
void Entity::setspeed(int s)
{
	speed = s;
	speedct = 0;
	if (follower) follower->setspeed(s);
}

void Entity::setchr(CHR *c) { chr = c; }
int Entity::get_waypointx() { return waypointx; }
int Entity::get_waypointy() { return waypointy; }

void Entity::set_waypoint(int x1, int y1)
{	
	waypointx = x1;
	waypointy = y1;

	switch (sgn(y1-gety()))
	{
		case -1: face = NORTH; break;
		case 0:  break;
		case 1:  face = SOUTH; break;
	}
	switch (sgn(x1-getx()))
	{
		case -1: face = WEST; break;
		case 0:  break;
		case 1:  face = EAST; break;
	}
}

void Entity::set_waypoint_relative(int x1, int y1)
{
	waypointx += x1;
	waypointy += y1;

	switch (sgn(y1))
	{
		case -1: face = NORTH; break;
		case 0:  break;
		case 1:  face = SOUTH; break;
	}
	switch (sgn(x1))
	{
		case -1: face = WEST; break;
		case 0:  break;
		case 1:  face = EAST; break;
	}
}

Entity* Entity::getfollower() {	return follower; }
void Entity::registerfollower(Entity *e) { follower = e; }

void Entity::stalk(Entity *e)
{
	if (e -> getfollower()) return;
	follow = e;
	e -> registerfollower(this);
}

bool Entity::ready()
{ return (x/65535 == waypointx && y/65535 == waypointy); }

void Entity::move_tick()
{
	int dx = waypointx - getx();
	int dy = waypointy - gety();

	framect++;
	if (framect >= 80) framect = 0;

	if (!(getx() % 16) && !(gety() % 16) && follower)
			follower -> set_waypoint(getx(), gety());

	if (dx && !dy) // Horizontal movement component only
	{
		x += sgn(dx) * 65535;
		return;
	}
	
	if (!dx && dy) // Vertical movement component only
	{
		y += sgn(dy) * 65535;
		return;
	}

	// Horizontal and vertical movement components
//	x += sgn(dx) * 46340;
//	y += sgn(dy) * 46340;
	x += sgn(dx) * 65535;
	y += sgn(dy) * 65535;
}

void Entity::think()
{
	int num_ticks;

	if (delay)
	{		
		framect = 0;
		delay--;
		return;
	}

	speedct += speed;
	num_ticks = speedct / 100;
	speedct %= 100;		// carry over

	while (num_ticks)
	{
		num_ticks--;

		if (ready())
			switch (movecode)
			{
				case 0: return;
				case 1: do_movescript(); break;
				case 2: do_wander(); break;
				default: err("Entity::think(), unknown movecode value");
			}
		if (!ready())
			move_tick();		
	}	
}

void Entity::draw()
{
	int frame;

	if (!visible) return;

	if (specframe)
		frame = specframe;
	else 
	{
		switch (face)
		{
			case SOUTH:	frame = 0; break;
			case NORTH:	frame = 5; break;
			case WEST: frame = 10; break;
			case EAST: frame = 15; break;
			default: err("Entity::draw(), unknown FACE value");
		}

		if (framect >= 10 && framect < 20) frame += 1;
		if (framect >= 20 && framect < 30) frame += 2;
		if (framect >= 30 && framect < 40) frame += 1;
		if (framect >= 50 && framect < 60) frame += 3;
		if (framect >= 60 && framect < 70) frame += 4;
		if (framect >= 70 && framect < 80) frame += 3;
	}

	int zx = getx() - xwin,
		zy = gety() - ywin;

	if (chr)
		chr -> Render(zx, zy, frame, myscreen);
	else
		Rect(zx, zy, zx + 15, zy + 15, 0, myscreen);
}

void Entity::set_face(int d) { face = d; }
int  Entity::get_face() { return face; }
void Entity::stopanim() { framect = 0; }
void Entity::stop() { set_waypoint(getx(), gety()); framect = 0; }

void Entity::set_movescript(char *s)
{
	strcpy(movestr, s);
	moveofs = 0;
	movecode = 1;
}

void Entity::set_wander(int x1, int y1, int x2, int y2, int step, int delay)
{
/*	wx1 = x1;
	wy1 = y1;
	wx2 = x2;
	wy2 = y2;
	wstep = step;
	wdelay = delay;
	movecode = 2;*/
}

void Entity::do_movescript()
{
	int arg;

	while ((movestr[moveofs] >= '0' && movestr[moveofs] <= '9') || movestr[moveofs] == ' ')
		moveofs++;
	switch(toupper(movestr[moveofs]))
	{
		case 'L': moveofs++;
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint_relative(-arg*16, 0);
				  break;
		case 'R': moveofs++;
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint_relative(arg*16, 0);
				  break;			
		case 'U': moveofs++;
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint_relative(0, -arg*16);
				  break;
		case 'D': moveofs++;
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint_relative(0, arg*16);
				  break;
		case 'S': moveofs++;
				  setspeed(atoi(&movestr[moveofs]));
				  break;
		case 'W': moveofs++;
				  delay = atoi(&movestr[moveofs]);
				  break;
		case 'F': moveofs++;
				  face = atoi(&movestr[moveofs]);
				  break;
		case 'B': moveofs = 0; break;
		case 'X': moveofs++;
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint(arg*16, gety());
				  break;
		case 'Y': moveofs++;
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint(getx(), arg*16);
				  break;
		case 'Z': moveofs++;
				  specframe = atoi(&movestr[moveofs]);				  
				  break;
		case 0:   moveofs = 0; movecode = 0; framect = 0; return;
		default: err("Entity::do_movescript(), unidentify movescript command");
	} 
}

void Entity::do_wander()
{
/*	bool ub=false, db=false, lb=false, rb=false;
	int ex = getx()/16;
	int ey = gety()/16;

	if (EntityObstructed(ex+wstep, ey) || ex+wstep > wx2) rb=true;
	if (EntityObstructed(ex-wstep, ey) || ex-wstep < wx1) lb=true;
	if (EntityObstructed(ex, ey+wstep) || ey+wstep > wy2) db=true;
	if (EntityObstructed(ex, ey-wstep) || ey-wstep < wy1) ub=true;

	if (rb && lb && db && ub) return; // Can't move in any direction

	delay = wdelay;
	while (1)
	{
		int i = rnd(0,3);
		switch (i)
		{
			case 0:
				if (rb) break;
				set_waypoint_relative(wstep*16, 0);
				return;
			case 1:
				if (lb) break;
				set_waypoint_relative(-wstep*16, 0);
				return;
			case 2:
				if (db) break;
				set_waypoint_relative(0, wstep*16);
				return;
			case 3:
				if (ub) break;
				set_waypoint_relative(0, -wstep*16);
				return;
		}
	}*/
}

/**************************************************************************/

int AllocateEntity(int x, int y, char *chr)
{
	entity[entities] = new Entity();
	entity[entities]->setxy(x, y);
	CHR *mychr = new CHR(chr);
	entity[entities]->setchr(mychr);	
	return entities++;
}

void RenderEntities()
{
	for (int i=0; i<entities; i++)
		entity[i]->draw();
}

void ProcessEntities()
{
	for (int i=0; i<entities; i++)
		entity[i]->think();
}

bool PlayerObstructed(int d)
{
	int ex, ey;

	switch (d)
	{
		case NORTH: 
			ex = myself->getx() / 16; 
			ey = (myself->gety() / 16) - 1; 
			break;
		case SOUTH:
			ex = myself->getx() / 16; 
			ey = (myself->gety() / 16) + 1; 
			break;
		case WEST:			
			ex = (myself->getx() / 16) - 1; 
			ey = myself->gety() / 16; 
			break;
		case EAST:			
			ex = (myself->getx() / 16) + 1; 
			ey = myself->gety() / 16; 
			break;
	}

	if (!current_map->obstructed(ex, ey)) 
		return false;
	return true;
}

void CheckZone()
{
	int cz = current_map->zone(myself->getx()/16, myself->gety()/16);
	if (rnd(0,255) < current_map->zones[cz].percent)
		ExecuteEvent(current_map->zones[cz].script);
}


void ProcessControls()
{
	UpdateControls();

	if (!myself || !myself->ready()) return;
	
	if (up && !PlayerObstructed(NORTH))
	{
		myself->set_waypoint_relative(0, -16);
		return;
	}
	if (down && !PlayerObstructed(SOUTH))
	{
		myself->set_waypoint_relative(0, 16);
		return;
	}
	if (left && !PlayerObstructed(WEST))
	{
		myself->set_waypoint_relative(-16, 0);
		return;
	}
	if (right && !PlayerObstructed(EAST))
	{
		myself->set_waypoint_relative(16, 0);
		return;
	}
}

void Render()
{
	switch (cameratracking)
	{
		case 0: 
			break;
		case 1:	
			if (myself)
			{
				xwin = (myself->getx() + 8) - (myscreen->width / 2);
				ywin = (myself->gety() + 8) - (myscreen->height / 2);
			}
			else xwin=0, ywin=0;
			if (xwin < 0) xwin = 0;
			if (ywin < 0) ywin = 0;
/*				if (xwin + myscreen->width > rmap)
				xwin = rmap - myscreen->width;
			if (ywin + myscreen->height > dmap)
				ywin = dmap - myscreen->height;*/
			break;
		case 2:
			if (tracker>=entities)
			{
				xwin = 0;
				ywin = 0;
			}
			else
			{
				xwin = (entity[tracker]->getx() + 8) - (myscreen->width/2);
				ywin = (entity[tracker]->gety() + 8) - (myscreen->height/2);
			}
			if (xwin < 0) xwin = 0;
			if (ywin < 0) ywin = 0;
			//fixme lower/right map bounds!
			break;
	}	
	current_map->Render(xwin, ywin, myscreen);
}

void Engine_Start(char *mapname)
{
	bool checkzone;

	for (int i=0; i<entities; i++)
	{
		delete entity[i]->chr;
		delete entity[i];
	}
	entities = 0;
	player = -1;
	myself = 0;
	
	xwin = ywin = 0;
	done = false;
	checkzone = false;
	kill = 0;
	current_map = new MAP(mapname);
	timer = 0;	

	while (!done)
	{
		while (timer)
		{
			if (myself && !myself->ready()) 
				checkzone = true;
			ProcessEntities();
			if (checkzone && myself->ready()) 
				CheckZone();
			if (done) break;
			ProcessControls();
			checkzone = false;
			timer--;
		}
		Render();
		ShowPage();
		Sleep(1);
	}
	delete current_map;
	current_map = 0;
	kill = 0;
}