extern byte up, down, left, right;
extern byte kill_up, kill_down, kill_left, kill_right;
extern byte b1, b2, b3, b4;
extern byte kill_b1, kill_b2, kill_b3, kill_b4;

#define UnUp() { kill_up = true; up = false; }
#define UnDown() { kill_down = true; down = false; }
#define UnLeft() { kill_left = true; left = false; }
#define UnRight() { kill_right = true; right = false; }
#define UnB1() { kill_b1 = true; b1 = false; }
#define UnB2() { kill_b2 = true; b2 = false; }
#define UnB3() { kill_b3 = true; b3 = false; }
#define UnB4() { kill_b4 = true; b4 = false; }

void UpdateControls();
