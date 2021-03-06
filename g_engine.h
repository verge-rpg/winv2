/******************************/
#define NORTH 1
#define SOUTH 2
#define WEST 3
#define EAST 4
/******************************/
class Entity
{
public:
	Entity();
	~Entity();
	int getx();
	int gety();
	void setxy(int x1, int y1);
	int getspeed();
	void setspeed(int s);
	void setchr(CHR *c);
	int get_waypointx();
	int get_waypointy();
	void set_waypoint(int x1, int y1);
	void set_waypoint_relative(int x1, int y1);
	void stalk(Entity *e);	
	Entity* getfollower();
	void registerfollower(Entity *e);
	bool ready();
	void think();
	void thinkanim();
	int  GetArg();

	void draw();
	void set_face(int d);
	int  get_face();
	void stopanim();
	void stop();

	void set_movescript(char *s);
	void set_wander(int x1, int y1, int x2, int y2, int step, int delay);
	void set_wanderzone(int step, int delay);
		
	void move_tick();
	void do_movescript();
	void do_wander();
	void do_wanderzone();

	bool visible, active;
	int x, y;
	int waypointx, waypointy;
	int speed, speedct;
	int delay;
	CHR *chr;
	Entity *follow, *follower;
	int  actscript;
	int face, framect, specframe, animdelay;
	char movestr[256], moveofs, *animofs;
	char movecode;
	int  wx1, wy1, wx2, wy2, wstep, wdelay;
};
/******************************/
extern Entity *entity[256], *myself;
extern int entities, player;
extern int xwin, ywin;
extern int cameratracking, tracker;
extern bool done;
extern MAP *current_map;
/******************************/
void Engine_Start(char *map);
void Render();
void RenderEntities();
void ProcessEntities();
int AllocateEntity(int x, int y, char *chr);
bool EntityObstructed(int ex, int ey);
/******************************/
