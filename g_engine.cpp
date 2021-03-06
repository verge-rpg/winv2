/******************************************************************
 * winv2: g_engine.cpp                                            *
 * copyright (c) 2001 vecna                                       *
 ******************************************************************/

#include "xerxes.h"

/****************************** data ******************************/

Entity *entity[256];
int entities=0, player;
int cameratracking=1, tracker=0;
MAP *current_map=0;
Entity *myself;
int xwin, ywin;
bool done, inscroller=false;

/****************************** code ******************************/

int gaytbl[] = { 1, 0, 2, 3 };

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
	face = 0;
	framect = 0;
	specframe = 0;
	movestr[0] = 0;
	moveofs = 0;
	movecode = 0;
	animdelay = 0;
	actscript = 0;
	visible = true;
	active = true;
}

Entity::~Entity() { return; }
int Entity::getx() { return x / 65535; }
int Entity::gety() { return y / 65535; }

void Entity::setxy(int x1, int y1)
{
	if (x1>65535) x1 = 64000;
	if (y1>65535) y1 = 64000;
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

void Entity::setchr(CHR *c) { chr = c; set_face(SOUTH); }
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

	thinkanim();

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
	x += sgn(dx) * 65535;
	y += sgn(dy) * 65535;
}

int Entity::GetArg()
{
	static char	token[10];

	while (*animofs == ' ')
		animofs++;

	int n = 0;
	while (*animofs >= '0' && *animofs <= '9')
		token[n++] = *animofs++;
	token[n] = 0;

	return atoi(token);
}

void Entity::thinkanim()
{
	if (!chr) 
		return;

	if (animdelay)
	{
		animdelay--;
		return;
	}
	
	while (*animofs == ' ')
	    animofs++;

	switch (*animofs++)
	{
		case 'f':
		case 'F':
			framect = GetArg();
			break;
		case 'w':
		case 'W':
			animdelay = GetArg();
			break;
		case 0:
			switch (face)
			{
				case SOUTH: animofs = chr->danim; break;
				case NORTH: animofs = chr->uanim; break;
				case WEST:  animofs = chr->lanim; break;
				case EAST:  animofs = chr->ranim; break;
			}
			break;
	}
}

void Entity::think()
{
	int num_ticks;

	if (!active) return;
	if (delay>systemtime)
	{		
		switch (face)
		{
			case SOUTH: animofs = chr->danim; break;
			case NORTH: animofs = chr->uanim; break;
			case WEST:  animofs = chr->lanim; break;
			case EAST:  animofs = chr->ranim; break;
		}
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
				case 1: do_wander(); break;
				case 2: do_wanderzone(); break;
				case 4: do_movescript(); break;
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
	else if (chr)
	{
		switch (face)
		{
			case SOUTH:	frame = chr->idle[0]; break;
			case NORTH:	frame = chr->idle[1]; break;
			case WEST: frame = chr->idle[3]; break;
			case EAST: frame = chr->idle[2]; break;
			default: err("Entity::draw(), unknown FACE value (%d)", face);
		}
		if (!ready()) frame = framect;
	}

	int zx = getx() - xwin,
		zy = gety() - ywin;

	if (chr)
		chr->Render(zx, zy, frame, myscreen);
	else
		Rect(zx, zy, zx + 15, zy + 15, 0, myscreen);
}

void Entity::set_face(int d) 
{ 
	if (face == d) return;
	face = d; 
	framect = 0;
	animdelay = 0;
	switch (d)
	{
		case SOUTH: animofs = chr->danim; break;
		case NORTH: animofs = chr->uanim; break;
		case WEST:  animofs = chr->lanim; break;
		case EAST:  animofs = chr->ranim; break;
	}
	thinkanim();
}

int  Entity::get_face() { return face; }
void Entity::stopanim() { framect = 0; animdelay = 0; }
void Entity::stop() { set_waypoint(getx(), gety()); framect = 0; movecode = 0; }

void Entity::set_movescript(char *s)
{
	strcpy(movestr, s);
	moveofs = 0;
	movecode = 4;
}

void Entity::set_wander(int x1, int y1, int x2, int y2, int step, int delay)
{
	wx1 = x1;
	wy1 = y1;
	wx2 = x2; 
	wy2 = y2;
	wstep = step;
	wdelay = delay;
	movecode = 1;
}

void Entity::set_wanderzone(int step, int delay)
{
	wstep = step;
	wdelay = delay;
	movecode = 2;
}

void Entity::do_movescript()
{
	static char vc2me[] = { 2, 1, 3, 4 };
	int arg;

	while ((movestr[moveofs] >= '0' && movestr[moveofs] <= '9') || movestr[moveofs] == ' ')
		moveofs++;
	switch(toupper(movestr[moveofs]))
	{
		case 'L': moveofs++;
				  if (face != WEST) set_face(WEST);
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint_relative(-arg*16, 0);
				  break;
		case 'R': moveofs++;
				  if (face != EAST) set_face(EAST);
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint_relative(arg*16, 0);
				  break;			
		case 'U': moveofs++;
			      if (face != NORTH) set_face(NORTH);
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint_relative(0, -arg*16);
				  break;
		case 'D': moveofs++;
				  if (face != SOUTH) set_face(SOUTH);
				  arg = atoi(&movestr[moveofs]);
				  set_waypoint_relative(0, arg*16);
				  break;
		case 'S': moveofs++;
				  setspeed(atoi(&movestr[moveofs]));
				  break;
		case 'W': moveofs++;
				  delay = systemtime + atoi(&movestr[moveofs]);
				  break;
		case 'F': moveofs++;
				  set_face(vc2me[atoi(&movestr[moveofs])]);
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
	bool ub=false, db=false, lb=false, rb=false;
	int ex = getx()/16;
	int ey = gety()/16;

	if (ex==31)
		ex=31;

	if (EntityObstructed(ex+wstep, ey) || ex+wstep > wx2) rb=true;
	if (EntityObstructed(ex-wstep, ey) || ex-wstep < wx1) lb=true;
	if (EntityObstructed(ex, ey+wstep) || ey+wstep > wy2) db=true;
	if (EntityObstructed(ex, ey-wstep) || ey-wstep < wy1) ub=true;

	if (rb && lb && db && ub) return; // Can't move in any direction

	delay = systemtime + wdelay;
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
	}
}

void Entity::do_wanderzone()
{
	bool ub=false, db=false, lb=false, rb=false;
	int ex = getx()/16;
	int ey = gety()/16;
	int myzone = current_map->zone(ex, ey);

	if (EntityObstructed(ex+wstep, ey) || current_map->zone(ex+wstep, ey) != myzone) rb=true;
	if (EntityObstructed(ex-wstep, ey) || current_map->zone(ex-wstep, ey) != myzone) lb=true;
	if (EntityObstructed(ex, ey+wstep) || current_map->zone(ex, ey+wstep) != myzone) db=true;
	if (EntityObstructed(ex, ey-wstep) || current_map->zone(ex, ey-wstep) != myzone) ub=true;

	if (rb && lb && db && ub) return; // Can't move in any direction

	delay = systemtime + wdelay;
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
	}
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

static int _cdecl cmpent(const void* a, const void* b)
{
	return entity[*(byte*)a]->gety() - entity[*(byte*)b]->gety();
}

void RenderEntities()
{
	static byte entidx[256];
	int entnum = 0;

	// Build a list of entities that are onscreen/visible.
	// FIXME: Make it actually only be entities that are onscreen
	for (int i=0; i<entities; i++)
		entidx[entnum++]=i;

	// Ysort that list, then draw.	
	qsort(entidx, entnum, 1, cmpent);
	for (i=0; i<entnum; i++)
		entity[entidx[i]]->draw();
}

void ProcessEntities()
{
	for (int i=0; i<entities; i++)
		entity[i]->think();
}

int EntityAt(int x, int y)
{
	for (int i=0; i<entities; i++)
		if (entity[i]->get_waypointx()/16 == x &&
			entity[i]->get_waypointy()/16 == y)
			return i;
	return -1;
}

bool EntityObstructed(int ex, int ey)
{
	if (current_map->obstructed(ex, ey)) 
		return true;
	if (EntityAt(ex, ey) != -1)
		return true;
	return false;
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

	if (current_map->obstructed(ex, ey)) 
		return true;
	if (EntityAt(ex, ey) != -1)
		return true;
	return false;
}

void CheckZone()
{
	int cur_timer = timer;
	int cz = current_map->zone(myself->getx()/16, myself->gety()/16);
	if (rnd(0,255) < current_map->zones[cz].percent)
		ExecuteEvent(current_map->zones[cz].script);
	timer = cur_timer;
}


void ProcessControls()
{
	UpdateControls();

	if (!myself || !myself->ready()) return;
	
	if (up)
	{
		myself->set_face(NORTH);
		if (!PlayerObstructed(NORTH))
		{
			myself->set_waypoint_relative(0, -16);
			return;
		}
	}
	if (down)
	{
		myself->set_face(SOUTH);
		if (!PlayerObstructed(SOUTH))
		{
			myself->set_waypoint_relative(0, 16);
			return;
		}
	}
	if (left)
	{
		myself->set_face(WEST);
		if (!PlayerObstructed(WEST))
		{
			myself->set_waypoint_relative(-16, 0);
			return;
		}
	}	
	if (right)
	{
		myself->set_face(EAST);
		if (!PlayerObstructed(EAST))
		{
			myself->set_waypoint_relative(16, 0);
			return;
		}
	}
	if (b1)
	{
		int ex, ey;
		switch (myself->face)
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
		int i = EntityAt(ex, ey);
		if (i != -1 && entity[i]->actscript)
		{
			UnB1();
			switch (myself->face)
			{
				case NORTH: entity[i]->face = SOUTH; break;
				case SOUTH: entity[i]->face = NORTH; break;
				case WEST: entity[i]->face = EAST; break;					
				case EAST: entity[i]->face = WEST; break;
			}	
			ExecuteEvent(entity[i]->actscript);
			return;
		}
		int cz = current_map->zone(ex, ey);
		if (current_map->zones[cz].script)
		{
			UnB1();
			int cur_timer = timer;
			ExecuteEvent(current_map->zones[cz].script);
			timer = cur_timer;
		}
	}
}

void MapScroller()
{
	inscroller = true;
	int oldx = xwin;
	int oldy = ywin;
	int oldtimer = timer;
	int oldvctimer = vctimer;
	int oldcamera = cameratracking;
	lastpressed = 0;

	while (lastpressed != 41)
	{
		if (keys[SCAN_UP]) ywin--;
		if (keys[SCAN_DOWN]) ywin++;
		if (keys[SCAN_LEFT]) xwin--;
		if (keys[SCAN_RIGHT]) xwin++;
		UpdateControls();
		Render();
		ShowPage8();
	}

	lastpressed = 0;
	keys[41] = 0;
    cameratracking = oldcamera;
	vctimer = oldvctimer;
	timer = oldtimer;
	ywin = oldy;
	xwin = oldx;
	inscroller = false;
}

void Render()
{	
	if (!current_map) return;

	if (cheats && !inscroller && lastpressed == 41)
		MapScroller();
	if (cheats && lastpressed == 197)
	{
		keys[197] = 0;
		lastpressed = 0;
		obszone ^= 1;
	}

	int rmap = (current_map->mapwidth() * 16);
	int dmap = (current_map->mapheight() * 16);
	
	switch (cameratracking)
	{
		case 0: 
			if (xwin + myscreen->width >= rmap)
				xwin = rmap - myscreen->width;
			if (ywin + myscreen->height >= dmap)
				ywin = dmap - myscreen->height;
			if (xwin < 0) xwin = 0;
			if (ywin < 0) ywin = 0;			
			break;
		case 1:	
			if (myself)
			{
				xwin = (myself->getx() + 8) - (myscreen->width / 2);
				ywin = (myself->gety() + 8) - (myscreen->height / 2);
			}
			else xwin=0, ywin=0;
			if (xwin + myscreen->width >= rmap)
				xwin = rmap - myscreen->width;
			if (ywin + myscreen->height >= dmap)
				ywin = dmap - myscreen->height;
			if (xwin < 0) xwin = 0;
			if (ywin < 0) ywin = 0;
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
			if (xwin + myscreen->width >= rmap)
				xwin = rmap - myscreen->width;
			if (ywin + myscreen->height >= dmap)
				ywin = dmap - myscreen->height;
			if (xwin < 0) xwin = 0;
			if (ywin < 0) ywin = 0;			
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
		ShowPage8();
		Sleep(1);
	}
	delete current_map;
	current_map = 0;
	kill = 0;
}