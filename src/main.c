
#include <SDL.h>
#include <SDL_mixer.h>
#include <math.h>
#include "SDL_FontCache.h"

#define APP_VEC_IMPLEMENTATION
#include "vec.h"

#define PIXC_IMPLEMENTATION
#include "pixc.h"

int have_mouse;

#define VW 96
#define VH 128

#define W (VW) 
#define H (VH) 
int toggle_fullscreen = 0;
texture32 game_texture;
texture32 video_texture;
int done = 0;
vec2 dir;
int render_flash = 0;

#define ANGLE_0 (0)
#define ANGLE_45 (M_PI / 4.0)
#define ANGLE_60 (M_PI / 3.0)
#define ANGLE_90 (M_PI / 2.0)
#define ANGLE_120 (2.0 * M_PI / 3.0)
#define ANGLE_135 (3.0 * M_PI / 4.0)
#define ANGLE_180 (M_PI)
#define ANGLE_270 (3.0 * M_PI / 2.0)
#define ANGLE_360 (0)

#define SOUND_BOMBHIT   "assets/bombhit.wav"
#define SOUND_TURRET    "assets/turret.wav"
#define SOUND_OUCH      "assets/ouch.wav"
#define SOUND_DING1     "assets/ding1.wav"
#define SOUND_DING2     "assets/ding2.wav"

Mix_Chunk *sound_turret;
Mix_Chunk *sound_bombhit;
Mix_Chunk *sound_ouch;
Mix_Chunk *sound_ding1;
Mix_Chunk *sound_ding2;

FC_Font *font;

struct Bullet {
    vec2 pos;
    float angle;
    int alive;
    int num_hits;
    u32 color;
};

struct Star {
    vec2 pos;
    vec2 vel;
    float angle;
    int alive;
    u32 color;
};

#define MAX_STARS 200
struct Stars {
    struct Star list[MAX_STARS];
    int current_star;
    u32 last_spawn;
};

#define TURRET_MAX_BULLETS 100
struct Turret {
    vec2 start;
    vec2 end;
    float xvel;
    float yvel;
    float angle;
    float len;
    struct Bullet bullets[TURRET_MAX_BULLETS];
    int current_bullet;
    u32 last_fire;
};

#define MAX_ENEMIES 400
struct Enemy {
    vec2 pos;
    float angle;
    int alive;
    int explode;
    u32 color;
};

struct Enemies {
    struct Enemy list[MAX_ENEMIES];
    int current_enemy;
    u32 last_spawn;
};

struct Turret player_turret = {0};
struct Enemies enemies = {0};
struct Stars stars = {0};
int enemy_spawn_countdown = 190;//600;
int star_spawn_countdown = 1;
int turret_fire_rate_countdown = 180;
u32 score = 0;

static inline void putpixel(texture32 *t, int x, int y, u32 color)
{
    if (x < 0 || y < 0) return;
    if (x >= t->w || y >= t->h) return;
    t->pixels[y * W + x] = color;
}

int random (int lower, int upper)
{
    return (rand() % (upper - lower + 1)) + lower;
}

void init_buildings()
{

}

int render_building (int startx)
{
    int width = random(15, 30);
    int height = random(15, 30);

    width = 10.0 + SDL_abs(SDL_cos(startx) * 30.0);
    height = SDL_abs(SDL_sin(startx) * 40.0);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width && x < W; ++x) {
            if (startx + x < W) {
                putpixel(&game_texture, (startx + x), (H - y), 0xff0a0a0a);
            }
        }
    }
    return width;
}

int render_buildings()
{
    return 1;
    int w = 0;
    for (int x = 0; x < W; x += w + 1) {
        w = render_building(x);
    }
    return 1;
}

void render_line(texture32 *t, int x0, int y0, int x1, int y1, u32 color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;      /* error value e_xy */

    for (;;) {                  /* loop */
        if (y0 < t->h && x0 < t->w && y0 > -1 && x0 > -1) t->pixels[y0 * t->w + x0] = color;
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }                       /* e_xy+e_x > 0 */
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }                       /* e_xy+e_y < 0 */
    }
}


void render_half_circle (texture32 *t, int x0, int y0, int radius, u32 color)
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {

        render_line(t, x0 - x, y0 - y, x0 + x, y0 - y, color);
        render_line(t, x0 - y, y0 - x, x0 + y, y0 - x, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }

        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void spawn_star()
{
    if (SDL_GetTicks() - stars.last_spawn < star_spawn_countdown) {
        return;
    }

    if (stars.current_star >= MAX_STARS) {
        stars.current_star = 0;
    }

    if (stars.list[stars.current_star].alive) {
        return;

    }

    stars.list[stars.current_star].pos.x = random(0, W - -1);
    stars.list[stars.current_star].pos.y = 0;
    stars.list[stars.current_star].angle  = ANGLE_270;
    stars.list[stars.current_star].alive = 1;
    stars.list[stars.current_star].color = random(130, 240);
    stars.list[stars.current_star].vel.y = random(1, 4);
    stars.list[stars.current_star].vel.x = stars.list[stars.current_star].vel.y;
    stars.last_spawn = SDL_GetTicks();

    stars.current_star++;
}

void spawn_enemy()
{
    if (SDL_GetTicks() - enemies.last_spawn < enemy_spawn_countdown) {
        return;
    }

    if (enemies.current_enemy >= MAX_ENEMIES) {
        enemies.current_enemy = 0;
    }

    if (enemies.list[enemies.current_enemy].alive) {
        return;
    }

    enemies.list[enemies.current_enemy].color = 0xff00ff00;
    enemies.list[enemies.current_enemy].pos.x = random(5, W - 5);
    enemies.list[enemies.current_enemy].pos.y = 0;
    enemies.list[enemies.current_enemy].angle  = ANGLE_270;
    enemies.list[enemies.current_enemy].alive = 1;
    enemies.list[enemies.current_enemy].explode = 0;
    enemies.last_spawn = SDL_GetTicks();

    enemies.current_enemy++;
    if (enemy_spawn_countdown > 90) {
        enemy_spawn_countdown -= 25;
        //enemy_spawn_countdown += random (-100, 100);
    }
    /*
    if (turret_fire_rate_countdown > 800) {
        turret_fire_rate_countdown -=5;
    }
    */
}

void render_stars()
{
    for (int i = 0; i < MAX_STARS; ++i) {
        if (stars.list[i].alive == 1) {
            int x = stars.list[i].pos.x;
            int y = stars.list[i].pos.y;
            u32 c = stars.list[i].color;
            putpixel(&game_texture, x, y, SDL_MapRGB(game_texture.surface->format, c, c, c));
            if (stars.list[i].vel.y > 3) {
                putpixel(&game_texture, x, y - 1, SDL_MapRGB(game_texture.surface->format, c, c, c));
            }
        }
    }
}

void render_enemies()
{
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        int x = enemies.list[i].pos.x;
        int y = enemies.list[i].pos.y;
        if (enemies.list[i].alive == 1) {
            putpixel(&game_texture, x, y, 0xffff0000);
            putpixel(&game_texture, x + 1, y, enemies.list[i].color);
            putpixel(&game_texture, x - 1, y, enemies.list[i].color);
            putpixel(&game_texture, x, y - 1, enemies.list[i].color);
            putpixel(&game_texture, x, y + 1, enemies.list[i].color);
        } else if (enemies.list[i].explode) {
            for(int j = 0; j < 20; ++j) {
                int c = random(100, 256);
                putpixel(&game_texture, x, y, SDL_MapRGB(game_texture.surface->format, c, c, 0));
                putpixel(&game_texture, x + 1, y + 1, SDL_MapRGB(game_texture.surface->format, c, 0, 0));
                x += random(-2, 2);
                y += random(-2, 2);
            }
            if (enemies.list[i].explode > 0) {
                enemies.list[i].explode--;
            }
        }
    }
}

void fire_bullet(struct Turret *turret)
{
    if (turret->current_bullet >= TURRET_MAX_BULLETS) {
        turret->current_bullet = 0;
    }

    if (SDL_GetTicks() - player_turret.last_fire < turret_fire_rate_countdown)
    {
        return;
    }

    //turret->bullets[turret->current_bullet].color = turret->current_bullet % 2 ? 0xffff0000 : 0xffffff00;
    turret->bullets[turret->current_bullet].color = 0xffff0000;
    turret->bullets[turret->current_bullet].pos.x = turret->end.x;
    turret->bullets[turret->current_bullet].pos.y = turret->end.y;
    turret->bullets[turret->current_bullet].angle = turret->angle;
    turret->bullets[turret->current_bullet].alive = 1;
    turret->bullets[turret->current_bullet].num_hits = 0;
    turret->last_fire = SDL_GetTicks();

    turret->current_bullet++;
    score--;

    if (turret->current_bullet % 2 == 0) {
        Mix_PlayChannel(1, sound_turret, 0);
    }
}

static void render_bullets()
{
    for (int i = 0; i < TURRET_MAX_BULLETS; ++i) {
        if (player_turret.bullets[i].alive == 1) {
            int x = player_turret.bullets[i].pos.x;
            int y = player_turret.bullets[i].pos.y;
            putpixel(&game_texture, x, y, 0xffffff00);
            //putpixel(&game_texture, x, y, player_turret.bullets[i].color);
            putpixel(&game_texture, x, y + 1, player_turret.bullets[i].color);
        }
    }
}

static int app_handle_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            done = 1;
            return 1;
        case SDL_MOUSEMOTION:
            /*
            if (event.motion.xrel > 0) {
                player_turret.angle = CLAMP(player_turret.angle - .01, ANGLE_45, ANGLE_135);
            } else if (event.motion.xrel < 0) {
                player_turret.angle = CLAMP(player_turret.angle + .01, ANGLE_45, ANGLE_135);
            }
            */
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                done = 1;
                return 1;
            case SDLK_LEFT:
            case SDLK_j:
                break;
            case SDLK_RIGHT:
            case SDLK_l:
                break;
            case SDLK_UP:
            case SDLK_w:
                //player_turret.start.y -= .1;
                break;
            case SDLK_DOWN:
            case SDLK_s:
                break;
            case SDLK_a:
                break;
            case SDLK_d:
                break;
            case SDLK_i:
                break;
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.sym) {
            case SDLK_LEFT:
            case SDLK_j:
                break;
            case SDLK_RIGHT:
            case SDLK_l:
                break;
            case SDLK_UP:
            case SDLK_w:
                break;
            case SDLK_DOWN:
            case SDLK_s:
                break;
            case SDLK_a:
                break;
            case SDLK_d:
                break;
            case SDLK_i:
                break;
            case SDLK_m:
                have_mouse = have_mouse == SDL_TRUE ? SDL_FALSE : SDL_TRUE;
                break;
            case SDLK_f:
                toggle_fullscreen = 1;
                break;
            }
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
                break;
            }
        }
    }
    return 0;
}

static void hblur (uint32_t* scan, int width, int height, int pitch)
{
	int x,y;
	u8 *pixel;
	
	width -= 1;
	for (y=0; y<height; y++)
	{
		pixel = (u8*)scan;
		for (x=0; x<width; x++)
		{
			pixel[0] = (pixel[0] + pixel[4]) >> 1;
			pixel[1] = (pixel[1] + pixel[5]) >> 1;
			pixel[2] = (pixel[2] + pixel[6]) >> 1;
			pixel += sizeof(uint32_t);
		}
		scan += pitch;
	}
}

static void render_cabinet()
{
    for (int y = 0; y < video_texture.w; y++) {
        u32 c = 0xff000000 | y >> 2;
        if (render_flash) {
           c = 0xffff0000; 
        }
        render_line(&video_texture, 0, y, video_texture.w - 1, y, c);
    }
}

static void app_render_frame(SDL_Renderer * renderer)
{
    // Clear backgrounds
    memset(game_texture.pixels, 0x10, game_texture.h * sizeof(u32) * game_texture.w);
    //memset(video_texture.pixels, 0xffff, video_texture.h * sizeof(u32) * video_texture.w);
    render_cabinet();
    for (int y = 0; y < VH; ++y) {
        for (int x = 0; x < VW; ++x) {
            //video_texture.pixels[y * VW + x] = 0xff402618;
        }
    }

    // Configure turret color
    int turret_color = 0xff0099ff;
    int turret_base_color = 0xff0000ff;
    if (render_flash) {
       turret_color = 0xffff0000; 
       turret_base_color = 0xffff0000; 
    }

    // Draw buildings
    //render_buildings();

    // Draw stars
    render_stars();

    // Draw planet
    for (int r = game_texture.w / 2; r > 0; r--) {
        u32 cc = game_texture.w / 2 - r;
        u32 c = SDL_MapRGB(game_texture.surface->format, cc, cc, cc);
        render_half_circle(&game_texture, game_texture.w / 2, game_texture.h - 1, r, c);
    }

    // Draw turret gun
    render_line(&game_texture, player_turret.start.x, player_turret.start.y,
                player_turret.end.x, player_turret.end.y, turret_color);
    /*
    render_line(&game_texture, player_turret.start.x - 1, player_turret.start.y,
                player_turret.end.x - 1, player_turret.end.y, turret_color);
    render_line(&game_texture, player_turret.start.x + 1, player_turret.start.y,
                player_turret.end.x + 1, player_turret.end.y, turret_color);
    */

    // Draw turret base
    render_half_circle(&game_texture, player_turret.start.x, player_turret.start.y + 3, 4, turret_base_color);

    // Draw bullets
    render_bullets();

    // Draw enemies
    render_enemies();

    // Blit the game area to the main video area
    SDL_Rect renderQuad = { VW / 2 - W / 2, 0, W, H };
    if (render_flash) {
        renderQuad.x += SDL_cos(SDL_GetTicks()) * 5.0;
        SDL_BlitSurface(game_texture.surface, NULL, video_texture.surface, &renderQuad);
    } else {
        SDL_BlitSurface(game_texture.surface, NULL, video_texture.surface, &renderQuad);
    }

    // Visual horizontal blur
    hblur(video_texture.pixels, video_texture.w, video_texture.h, video_texture.w);

    // Update the gpu texture
    SDL_Rect r = {.w = video_texture.w,.h = video_texture.h,.x = 0,.y = 0 };
    SDL_UpdateTexture(screenBuffer, &r, video_texture.pixels,
		      video_texture.w * sizeof(u32));
    SDL_RenderCopy(renderer, screenBuffer, NULL, NULL);

    // Do hardware drawing
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    FC_Draw(font, renderer, 0, 0, "Score\n%d", score);
    FC_Draw(font, renderer, VW - 45, 0, "Arcade\nBoomer\nShooter");

    // Present
    SDL_RenderPresent(renderer);


    if (render_flash) {
        --render_flash;
    }
}

static void app_state_step(void)
{

    // Get keyboard state
    const u8 *keys = SDL_GetKeyboardState(NULL);

    // Handle up
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
        if (player_turret.yvel > -3.0) player_turret.yvel -= .07;
    } else {
        player_turret.yvel *= 0.99;
    }

    // Handle down
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
        if (player_turret.yvel < 3.0) player_turret.yvel += .07;
    } else {
        player_turret.yvel *= 0.99;
    }

    // Handle left
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
        if (player_turret.xvel > -3.0) player_turret.xvel -= .10;
    } else {
        player_turret.xvel *= 0.99;
    }


    // Handle right
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
        //player_turret.start.x = CLAMP(player_turret.start.x + 3.0, 10, W - 10);
        if (player_turret.xvel < 3.0) player_turret.xvel += .10;
    } else {
        player_turret.xvel *= 0.99;
    }

    // Apply velocity
    player_turret.start.x = CLAMP(player_turret.start.x + player_turret.xvel, 10, W - 10);
    player_turret.start.y = CLAMP(player_turret.start.y + player_turret.yvel, 10, H - 10);

    // Bounce off wall
    if (player_turret.start.x == 10 || player_turret.start.x == W - 10) {
        player_turret.xvel *= -1.0;
    }
    

    int mouseX, mouseY;
    int mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

    // Check mouse click bullet fire
    if (mouseState & SDL_BUTTON_LMASK) {
        fire_bullet(&player_turret);
    }
    if (mouseState & SDL_BUTTON_RMASK) {
        turret_fire_rate_countdown = 5;
    } else {
        turret_fire_rate_countdown = 40;
    }

    // Update turret angle by mouse then set new endpoint
    player_turret.angle = CLAMP(player_turret.angle - (mouseX * .001), ANGLE_60, ANGLE_120);
    player_turret.end.x = player_turret.start.x + SDL_cos(player_turret.angle) * player_turret.len;
    player_turret.end.y = player_turret.start.y - SDL_sin(player_turret.angle) * player_turret.len;

    // Advance alive stars, kill out-of-bounds stars
   for (int i = 0; i < MAX_STARS; ++i) {
        
            stars.list[i].pos.x += SDL_cos(stars.list[i].angle) * stars.list[i].vel.x;
            stars.list[i].pos.y -= SDL_sin(stars.list[i].angle) * stars.list[i].vel.y;

            if(stars.list[i].pos.x >= W) stars.list[i].alive = 0;
            if(stars.list[i].pos.x <  0) stars.list[i].alive = 0;

            if(stars.list[i].pos.y >= H) stars.list[i].alive = 0;
            if(stars.list[i].pos.y < 0) stars.list[i].alive = 0;
   }

    // Advance alive bullets, kill out-of-bounds bullets
    for (int i = 0; i < TURRET_MAX_BULLETS; ++i) {
        if (player_turret.bullets[i].alive == 1) {
            player_turret.bullets[i].pos.x += SDL_cos(player_turret.bullets[i].angle) * 3.0;
            player_turret.bullets[i].pos.y -= SDL_sin(player_turret.bullets[i].angle) * 3.0;

            if(player_turret.bullets[i].pos.x >= W) player_turret.bullets[i].alive = 0;
            if(player_turret.bullets[i].pos.x <  0) player_turret.bullets[i].alive = 0;

            if(player_turret.bullets[i].pos.y >= H) player_turret.bullets[i].alive = 0;
            if(player_turret.bullets[i].pos.y < 0) player_turret.bullets[i].alive = 0;

        }
    }

    // Advance alive enemies, kill out-of-bounds enemies 
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (enemies.list[i].alive == 1) {
            enemies.list[i].pos.x += SDL_cos(i) * .2;// + random(-1, 1);
            enemies.list[i].pos.y -= SDL_sin(enemies.list[i].angle) * 1.1;

            if(enemies.list[i].pos.x >= W) enemies.list[i].alive = 0;
            if(enemies.list[i].pos.x <  0) enemies.list[i].alive = 0;

            if(enemies.list[i].pos.y >= H) enemies.list[i].alive = 0;
            if(enemies.list[i].pos.y < 0) enemies.list[i].alive = 0;


            if(enemies.list[i].pos.y >= game_texture.h - random(5, 35)) {
                render_flash = 10;
                Mix_PlayChannel(-1, sound_ouch, 0);
                enemies.list[i].explode = 20;
                enemies.list[i].alive = 0;
                score -= 5;
            }
        }
    }

    // Collide bullets with enemies
    for (int b = 0; b < TURRET_MAX_BULLETS; ++b) {
        for (int e = 0; e < MAX_ENEMIES; ++e) {
            if (SDL_abs(player_turret.bullets[b].pos.x - enemies.list[e].pos.x) < 5
                    && SDL_abs(player_turret.bullets[b].pos.y - enemies.list[e].pos.y) < 5
                    && enemies.list[e].alive == 1 && player_turret.bullets[b].alive == 1) {
                enemies.list[e].alive = 0;
                enemies.list[e].explode = 10;
                score += 10;
                player_turret.bullets[b].alive = 0;
                //if (++player_turret.bullets[b].num_hits >= 1) {
                    //player_turret.bullets[b].alive = 0;
                //}
                Mix_PlayChannel(-1, sound_bombhit, 0);
                Mix_PlayChannel(-1, e % 2 == 0 ? sound_ding1 : sound_ding2, 0);
            }
        }
    }

    spawn_enemy();
    spawn_star();
}

int init_audio()
{
    //Initialize SDL2_mixer
    printf("got here");
        fflush(stdout);
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 512) == -1)
    {
        printf("SDL2_mixer could not be initialized!\n"
               "SDL_Error: %s\n", SDL_GetError());
        fflush(stdout);
        return 0;
    }


    sound_turret = Mix_LoadWAV(SOUND_TURRET);
    sound_bombhit = Mix_LoadWAV(SOUND_BOMBHIT);
    sound_ouch = Mix_LoadWAV(SOUND_OUCH);
    sound_ding1 = Mix_LoadWAV(SOUND_DING1);
    sound_ding2 = Mix_LoadWAV(SOUND_DING2);

    int volume = Mix_Volume(-1, -1);
    Mix_Volume(1, volume / 2);

    return 0;
}

int main(int argc, char *argv[])
{

    /* sdl init */
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    init_audio();


    /* create window */
    SDL_Window *window = SDL_CreateWindow("pixc test",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          W, H,
                                          SDL_WINDOW_FULLSCREEN_DESKTOP);
    /*
       SDL_Window *window = SDL_CreateWindow("pixc test",
       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
       W, H, SDL_WINDOW_RESIZABLE);
     */

    /* create renderer */
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
                                                SDL_RENDERER_ACCELERATED |
                                                SDL_RENDERER_TARGETTEXTURE);

    /* pixc init */
    pixc_init(renderer, VW, VH);

    /* create drawing surface */
    pixc_make_texture32(W, H, &game_texture);
    pixc_make_texture32(VW, VH, &video_texture);

    SDL_SetRelativeMouseMode(SDL_TRUE);


    font = FC_CreateFont();
    //FC_LoadFont(font, renderer, "assets/04B_30__.ttf", 10, FC_MakeColor(255,255,0,255), TTF_STYLE_NORMAL);
    FC_LoadFont(font, renderer, "assets/dpcomic.ttf", 14, FC_MakeColor(255,255,0,255), TTF_STYLE_NORMAL);

    SDL_SetSurfaceBlendMode(video_texture.surface, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(game_texture.surface, SDL_BLENDMODE_NONE);

    player_turret.start.x = W / 2;
    player_turret.start.y = H - 3 - 25;
    player_turret.end.x = W / 2;
    player_turret.end.y = H - H / 4;
    player_turret.angle = M_PI / 2.0;
    player_turret.len = 3.0;
    player_turret.current_bullet = 0;
    player_turret.xvel = 0;
    memset(player_turret.bullets, 0, sizeof(player_turret.bullets));

    srand(0);

    u32 lastMillis = 0;
    u32 currentMillis = 0;

    while (!done) {
        currentMillis = SDL_GetTicks();
        app_handle_events();
        app_state_step();
        app_render_frame(renderer);
        while (currentMillis < lastMillis + 16) {
            currentMillis = SDL_GetTicks();
        }

        if (SDL_GetTicks() % 100 == 0) {
            printf("%d\n", currentMillis - lastMillis);
            fflush(stdout);
        }

        lastMillis = currentMillis;

        if (toggle_fullscreen) {
            pixc_toggle_fullscreen(window, renderer);
            toggle_fullscreen = 0;
        }

    }
    return 0;
}