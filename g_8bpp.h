extern byte *screen8, *realscreen, transtbl[256][256];
extern quad pal[256], base_pal[256];
extern byte pal8[256*3], base_pal8[256*3];
extern int clip_x, clip_y, clip_xend, clip_yend;
extern int screen_width, screen_height;

void Init8bpp();
void PutPixel8(int x, int y, int c);
byte ReadPixel8(int x, int y);
void HLine8(int x, int y, int xe, int color);
void VLine8(int x, int y, int ye, int color);
void Line8(int x, int y, int xe, int ye, int color);
void Box8(int x, int y, int x2, int y2, int color);
void Rect8(int x, int y, int x2, int y2, int color);
void Sphere8(int x, int y, int xradius, int yradius, int color);
void Circle8(int x, int y, int xradius, int yradius, int color);
void CopySprite8(int x, int y, int width, int height, byte *source);
void TCopySprite8(int x, int y, int width, int height, byte *source);
void ScaleSprite8(int x, int y, int sw, int sh, int dw, int dh, byte *s);
void TScaleSprite8(int x, int y, int sw, int sh, int dw, int dh, byte *s);
void WrapBlit8(int x, int y, int sw, int sh, byte *src);
void TWrapBlit8(int x, int y, int sw, int sh, byte *src);
void RotScale8(int posx, int posy, quad width, quad height, float angle, float scale, byte* src);
void Silhouette8(int x, int y, int width, int height, byte color, byte* source);
void ShowPage8();

void PutPixel8_Lucent(int x, int y, int c);
void HLine8_Lucent(int x, int y, int xe, int color);
void VLine8_Lucent(int x, int y, int ye, int color);
void Line8_Lucent(int x, int y, int xe, int ye, int color);
void Rect8_Lucent(int x, int y, int x2, int y2, int color);
void Box8_Lucent(int x, int y, int x2, int y2, int color);
void Sphere8_Lucent(int x, int y, int xradius, int yradius, int color);
void Circle8_Lucent(int x, int y, int xradius, int yradius, int color);
void CopySprite8_Lucent(int x, int y, int width, int height, byte *source);
void TCopySprite8_Lucent(int x, int y, int width, int height, byte *source);
void WrapBlit8_Lucent(int x, int y, int sw, int sh, byte *src);
void TWrapBlit8_Lucent(int x, int y, int sw, int sh, byte *src);
void ScaleSprite8_Lucent(int x, int y, int sw, int sh, int dw, int dh, byte *s);
void TScaleSprite8_Lucent(int x, int y, int sw, int sh, int dw, int dh, byte *s);
void RotScale8_Lucent(int posx, int posy, quad width, quad height, float angle, float scale, byte* src);
void Silhouette8_Lucent(int x, int y, int width, int height, byte color, byte* source);