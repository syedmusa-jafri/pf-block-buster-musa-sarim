#include <SFML/Graphics.hpp>
#include <time.h>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <vector>
#include <deque>
#include <string>
using namespace std;
using namespace sf;
Font font;
Text titleText(font);
Text menuOptions[3] = { Text(font), Text(font), Text(font) };
int selectedOption = 0;
Text scoreText(font);
Text livesText(font);
char powerup = '\0';
const int MENU = 0, PLAYING = 1, GAMEOVER = 2;
int gamestate = MENU;
int lives = 3;
int dx = 0, dy = 0;
int bricks[8][10];
int score = 0;
int rows, cols;
int widetimer = 0;
float powerx, powery;
float paddleX = 250.f;
float ballX = 294.f;
float ballY = 638.f;
bool gamestart = false, gamestarted = true, showNosave = false, power = false, wideactive = false;
bool won = false;
RectangleShape paddle({ 100.f, 14.f });
RectangleShape powerShape({ 25.f, 20.f });
CircleShape ball(6.f);
void starter() {
	ofstream out("level.txt");
	int rows = 8, cols = 10;
	int temp;
	srand(time(0));

	if (out.is_open()) {
		out << rows << " " << cols << endl;

		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				temp = (rand() % 4);
				out << temp << " ";
			}
			out << endl;
		}


		out.close();
	}
	else {
		cout << "Error opening file.\n";
	}
}
void loadlevel() {
	ifstream fin("level.txt");
	if (!fin.is_open()) {
		cout << "Error opening file";
		return;
	}

	fin >> rows >> cols;

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			fin >> bricks[i][j];
		}
	}
}

/* =====================================================================
   VISUAL EFFECTS LAYER  (front-end only - reads game state, never writes it,
   never touches rand() so the game's random sequence is unaffected)
   ===================================================================== */
static const float FX_PI = 3.14159265f;
Clock fxClock;
float fxLastT = 0.f;
float fxDt = 0.016f;
float fxNow = 0.f;

void fxTick() {
	fxNow = fxClock.getElapsedTime().asSeconds();
	fxDt = fxNow - fxLastT;
	if (fxDt > 0.05f) fxDt = 0.05f;
	if (fxDt < 0.f) fxDt = 0.f;
	fxLastT = fxNow;
}

Color mk(int r, int g, int b, int a = 255) {
	if (r < 0) r = 0; if (r > 255) r = 255;
	if (g < 0) g = 0; if (g > 255) g = 255;
	if (b < 0) b = 0; if (b > 255) b = 255;
	if (a < 0) a = 0; if (a > 255) a = 255;
	return Color((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
}
Color fxLerp(Color a, Color b, float t) {
	return mk((int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t),
		(int)(a.b + (b.b - a.b) * t), (int)(a.a + (b.a - a.a) * t));
}

// private visual-only random generator (does NOT use rand())
unsigned fxSeed = 2463534242u;
float fxRnd() {
	fxSeed ^= fxSeed << 13;
	fxSeed ^= fxSeed >> 17;
	fxSeed ^= fxSeed << 5;
	return (fxSeed & 0xFFFFFF) / 16777215.f;
}

void fxGradRect(RenderWindow& w, float x, float y, float wd, float ht, Color top, Color bot) {
	VertexArray va(PrimitiveType::TriangleStrip, 4);
	va[0].position = { x, y };           va[0].color = top;
	va[1].position = { x + wd, y };      va[1].color = top;
	va[2].position = { x, y + ht };      va[2].color = bot;
	va[3].position = { x + wd, y + ht }; va[3].color = bot;
	w.draw(va);
}

void fxRoundRect(RenderWindow& w, float x, float y, float wd, float ht, float r,
	Color fill, Color outline = Color(0, 0, 0, 0), float th = 0.f) {
	if (r > wd / 2) r = wd / 2;
	if (r > ht / 2) r = ht / 2;
	ConvexShape s;
	const int N = 6;
	s.setPointCount(4 * (N + 1));
	float cx[4] = { x + wd - r, x + wd - r, x + r, x + r };
	float cy[4] = { y + r, y + ht - r, y + ht - r, y + r };
	float a0[4] = { -90.f, 0.f, 90.f, 180.f };
	int idx = 0;
	for (int k = 0; k < 4; k++) {
		for (int i = 0; i <= N; i++) {
			float ang = (a0[k] + 90.f * i / N) * FX_PI / 180.f;
			s.setPoint(idx++, { cx[k] + r * cos(ang), cy[k] + r * sin(ang) });
		}
	}
	s.setFillColor(fill);
	if (th > 0.f) { s.setOutlineColor(outline); s.setOutlineThickness(th); }
	w.draw(s);
}

CircleShape fxCircle(32.f);
void fxGlow(RenderWindow& w, float cx, float cy, float radius, Color c, int layers = 8) {
	fxCircle.setPointCount(36);
	for (int i = layers; i >= 1; i--) {
		float rr = radius * i / layers;
		fxCircle.setRadius(rr);
		fxCircle.setOrigin({ rr, rr });
		fxCircle.setPosition({ cx, cy });
		float k = (float)(layers - i + 1) / layers;
		fxCircle.setFillColor(mk(c.r, c.g, c.b, (int)(c.a * k * k * 0.9f / (layers * 0.35f))));
		w.draw(fxCircle, RenderStates(BlendAdd));
	}
}

void fxText(RenderWindow& w, const string& s, unsigned size, float x, float y, Color fill,
	Color outline, float th, bool center, bool bold = true) {
	Text t(font);
	t.setString(s);
	t.setCharacterSize(size);
	if (bold) t.setStyle(Text::Bold);
	t.setFillColor(fill);
	t.setOutlineColor(outline);
	t.setOutlineThickness(th);
	FloatRect b = t.getLocalBounds();
	if (center) x = 300.f - b.size.x / 2.f - b.position.x;
	t.setPosition({ x, y });
	w.draw(t);
}

void fxGlowText(RenderWindow& w, const string& s, unsigned size, float y, Color fill, Color glow) {
	Text t(font);
	t.setString(s);
	t.setCharacterSize(size);
	t.setStyle(Text::Bold);
	FloatRect b = t.getLocalBounds();
	float x = 300.f - b.size.x / 2.f - b.position.x;
	for (int ring = 3; ring >= 1; ring--) {
		for (int k = 0; k < 8; k++) {
			float a = k * FX_PI / 4.f;
			t.setFillColor(mk(glow.r, glow.g, glow.b, 28));
			t.setPosition({ x + cos(a) * ring * 2.5f, y + sin(a) * ring * 2.5f });
			w.draw(t, RenderStates(BlendAdd));
		}
	}
	t.setFillColor(mk(0, 0, 0, 160));
	t.setPosition({ x + 3.f, y + 4.f });
	w.draw(t);
	t.setFillColor(fill);
	t.setOutlineColor(mk(255, 255, 255, 90));
	t.setOutlineThickness(1.f);
	t.setPosition({ x, y });
	w.draw(t);
}

void fxHeart(RenderWindow& w, float cx, float cy, float s, Color c) {
	float r = s * 0.27f;
	CircleShape a(r);
	a.setOrigin({ r, r });
	a.setFillColor(c);
	a.setPosition({ cx - r, cy - r * 0.4f });
	w.draw(a);
	a.setPosition({ cx + r, cy - r * 0.4f });
	w.draw(a);
	ConvexShape t(3);
	t.setPoint(0, { cx - 2 * r + 0.6f, cy - 0.05f * r });
	t.setPoint(1, { cx + 2 * r - 0.6f, cy - 0.05f * r });
	t.setPoint(2, { cx, cy + 2.0f * r });
	t.setFillColor(c);
	w.draw(t);
}

Color fxBrickColor(int level) {
	if (level == 1) return mk(70, 230, 130);
	if (level == 2) return mk(255, 205, 60);
	return mk(255, 70, 100);
}

void fxBackground(RenderWindow& w) {
	fxGradRect(w, 0, 0, 600, 700, mk(8, 8, 28), mk(38, 14, 62));
	// breathing nebula glows
	fxGlow(w, 120, 260, 260, mk(70, 40, 190, (int)(70 + 25 * sin(fxNow * 0.6f))), 10);
	fxGlow(w, 500, 520, 280, mk(190, 40, 140, (int)(60 + 25 * sin(fxNow * 0.5f + 2))), 10);
	fxGlow(w, 300, 680, 220, mk(30, 120, 200, 60), 8);
	// twinkling, drifting stars
	RectangleShape st;
	for (int i = 0; i < 110; i++) {
		float x = fmodf(i * 97.31f, 600.f);
		float sp = 4.f + (i % 5) * 5.f;
		float y = fmodf(i * 53.77f + fxNow * sp, 700.f);
		int a = (int)(110 + 110 * sin(fxNow * 2.f + i));
		float sz = 1.f + (i % 3) * 0.8f;
		st.setSize({ sz, sz });
		st.setPosition({ x, y });
		st.setFillColor(mk(200 + (i % 3) * 20, 210, 255, a));
		w.draw(st);
	}
}

/* ---- particles ---- */
struct FxParticle { float x, y, vx, vy, life, maxlife, size; Color col; };
vector<FxParticle> fxParticles;
int fxPrevBricks[8][10];
bool fxPrevValid = false;

void fxBurst(float x, float y, Color c, int n, float speed) {
	for (int i = 0; i < n; i++) {
		float a = fxRnd() * 2.f * FX_PI;
		float sp = speed * (0.35f + fxRnd());
		FxParticle p;
		p.x = x; p.y = y;
		p.vx = cos(a) * sp; p.vy = sin(a) * sp - 30.f;
		p.maxlife = p.life = 0.45f + fxRnd() * 0.5f;
		p.size = 2.f + fxRnd() * 3.f;
		p.col = c;
		fxParticles.push_back(p);
	}
}

// watches the brick array (read-only) and spawns sparks when a brick is hit
void fxTrackBricks(bool spawn) {
	int changes = 0;
	if (fxPrevValid && spawn) {
		for (int r = 0; r < 8; r++)
			for (int c = 0; c < 10; c++)
				if (bricks[r][c] < fxPrevBricks[r][c]) changes++;
		if (changes > 0 && changes <= 4) {
			for (int r = 0; r < 8; r++)
				for (int c = 0; c < 10; c++)
					if (bricks[r][c] < fxPrevBricks[r][c]) {
						float bx = c * 60.f + 30.f, by = 100.f + r * 25.f + 12.f;
						Color col = fxBrickColor(fxPrevBricks[r][c]);
						if (bricks[r][c] == 0) fxBurst(bx, by, col, 22, 190.f);
						else fxBurst(bx, by, col, 7, 110.f);
					}
		}
	}
	for (int r = 0; r < 8; r++)
		for (int c = 0; c < 10; c++)
			fxPrevBricks[r][c] = bricks[r][c];
	fxPrevValid = true;
}

void fxUpdateDrawParticles(RenderWindow& w) {
	RectangleShape sq;
	for (size_t i = 0; i < fxParticles.size();) {
		FxParticle& p = fxParticles[i];
		p.life -= fxDt;
		if (p.life <= 0.f) { fxParticles.erase(fxParticles.begin() + i); continue; }
		p.vy += 420.f * fxDt;
		p.x += p.vx * fxDt;
		p.y += p.vy * fxDt;
		float k = p.life / p.maxlife;
		sq.setSize({ p.size * k + 1.f, p.size * k + 1.f });
		sq.setOrigin({ (p.size * k + 1.f) / 2.f, (p.size * k + 1.f) / 2.f });
		sq.setRotation(degrees(p.life * 600.f));
		sq.setPosition({ p.x, p.y });
		sq.setFillColor(mk(p.col.r, p.col.g, p.col.b, (int)(255 * k)));
		w.draw(sq, RenderStates(BlendAdd));
		i++;
	}
}

deque<Vector2f> fxTrail;

void drawbricks(RenderWindow& window) {

	const float BRICK_W = 60.f, BRICK_H = 25.f;   // one brick is 60 x 25 pixels 
	const float BRICKS_Y = 100.f;                 // the bricks start at y = 100 

	// faint backdrop grid behind the brick area
	RectangleShape line;
	for (int c = 0; c <= 10; c++) {
		line.setSize({ 1.f, rows * BRICK_H });
		line.setPosition({ c * BRICK_W, BRICKS_Y });
		line.setFillColor(mk(120, 140, 255, 14));
		window.draw(line);
	}
	for (int r = 0; r <= rows; r++) {
		line.setSize({ 10 * BRICK_W, 1.f });
		line.setPosition({ 0.f, BRICKS_Y + r * BRICK_H });
		line.setFillColor(mk(120, 140, 255, 14));
		window.draw(line);
	}

	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			if (bricks[r][c] != 0) {
				Color base = fxBrickColor(bricks[r][c]);
				float x = c * BRICK_W + 1, y = BRICKS_Y + r * BRICK_H + 1;
				float bw = BRICK_W - 2, bh = BRICK_H - 2;

				// soft neon underglow
				fxRoundRect(window, x - 2, y - 2, bw + 4, bh + 4, 6, mk(base.r, base.g, base.b, 38));
				// body (3D gradient)
				fxRoundRect(window, x, y, bw, bh, 4, fxLerp(base, mk(0, 0, 0), 0.45f));
				fxGradRect(window, x + 2, y + 2, bw - 4, bh - 4,
					fxLerp(base, mk(255, 255, 255), 0.35f), fxLerp(base, mk(0, 0, 0), 0.25f));
				// glossy top highlight
				RectangleShape hi({ bw - 8, 3.f });
				hi.setPosition({ x + 4, y + 3 });
				hi.setFillColor(mk(255, 255, 255, 110));
				window.draw(hi);
				// armour pips show how many hits remain
				for (int p = 0; p < bricks[r][c]; p++) {
					RectangleShape pip({ 4.f, 4.f });
					pip.setPosition({ x + bw / 2 - bricks[r][c] * 4.f + p * 8.f + 2.f, y + bh - 8.f });
					pip.setFillColor(mk(255, 255, 255, 170));
					window.draw(pip);
				}
				// travelling shimmer
				float sh = sin(fxNow * 2.2f - (c + r) * 0.55f);
				if (sh > 0.85f) {
					RectangleShape sw({ 8.f, bh - 4 });
					sw.setPosition({ x + 3 + (sh - 0.85f) / 0.15f * (bw - 14), y + 2 });
					sw.setFillColor(mk(255, 255, 255, 70));
					window.draw(sw, RenderStates(BlendAdd));
				}
			}
		}
	}


}
void initObjects() {
	paddle.setPosition({ 250.f, 650.f });
	ball.setPosition({ 294.f, 638.f });
}
void movePaddle() {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
		paddleX = paddleX - 8;          // true for every frame the key is held down 
		if (paddleX < 0)
			paddleX = 0;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
		paddleX = paddleX + 8;          // true for every frame the key is held down 
		if (!wideactive) {
			if (paddleX > 500)
				paddleX = 500;
		}
		else {
			if (paddleX > 450)
				paddleX = 450;
		}

	}
}
void initalmoveball(int& dx, int& dy) {

	dy = -5;
	if (rand() % 2)
		dx = 3;
	else
		dx = -3;

	gamestarted = false;

}
void spawnpowerup(float x, float y);


bool brickcollision() {
	bool hit = false;
	int temp[4][2];
	int f = 0;


	for (int i = 0; i < 2; i++) {
		for (int j = 1; j >= 0; j--) {

			int x = ballX + (12 * j);
			int y = ballY + (12 * i);


			if (y >= 100 && y < 300 &&
				x >= 0 && x < 600) {

				int r = (y - 100) / 25;
				int c = x / 60;

				temp[f][0] = r;
				temp[f][1] = c;
			}
			else {
				temp[f][0] = -1;
				temp[f][1] = -1;
			}

			f++;
		}
	}


	for (int i = 0; i < f; i++) {

		if (temp[i][0] == -1)
			continue;

		for (int j = i + 1; j < f; j++) {

			if (temp[j][0] == temp[i][0] &&
				temp[j][1] == temp[i][1]) {

				temp[j][0] = -1;
				temp[j][1] = -1;
			}
		}
	}


	for (int i = 0; i < f; i++) {

		if (temp[i][0] >= 0 && temp[i][1] >= 0) {

			int r = temp[i][0];
			int c = temp[i][1];

			if (bricks[r][c] > 0) {
				bricks[r][c]--;
				hit = true;
				score += 10;
				if (bricks[r][c] == 0) {
					score += 40;
					spawnpowerup(c * 60, (r * 25) + 100);
				}
			}
		}
	}
	return hit;
}

void newgame() {
	score = 0;
	lives = 3;
	gamestart = false;
	gamestarted = true;
	won = false;
	dx = 0;
	dy = 0;
	paddleX = 250.f;
	ballX = 294.f;
	ballY = 638.f;
	power = false;
	powerup = '\0';
	wideactive = false;
	widetimer = 0;
	paddle.setSize({ 100.f, 14.f });

	starter();
	loadlevel();
	initObjects();
	gamestate = PLAYING;

}

void savegame() {
	ofstream out("savegame.txt");
	if (!out.is_open()) {
		cout << "Error opening save file";
		return;
	}



	out << score << " " << lives << endl;

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			out << bricks[i][j] << " ";
		}
		out << endl;
	}
	out.close();
}
bool loadgame() {
	ifstream fin("savegame.txt");

	if (!fin.is_open()) {
		return false;
	}

	rows = 8;
	cols = 10;

	if (!(fin >> score >> lives)) {
		return false;
	}

	if (lives <= 0) {
		return false;
	}

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if (!(fin >> bricks[i][j])) {
				return false;
			}
		}
	}
	gamestart = false;
	won = false;
	gamestarted = true;
	dx = 0;
	dy = 0;
	paddleX = 250.f;
	ballX = 294.f;
	ballY = 638.f;
	power = false;
	powerup = '\0';
	wideactive = false;
	widetimer = 0;
	paddle.setSize({ 100.f, 14.f });

	initObjects();
	return true;
}

bool allBricksDestroyed() {
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if (bricks[i][j])
				return false;
		}
	}
	return true;
}

void spawnpowerup(float x, float y) {
	if (!power) {
		int temp = rand() % 5;
		if (temp == 3) {
			temp = rand() % 2;

			if (temp)
				powerup = 'W';
			else
				powerup = '+';
			power = true;
			powerx = x;
			powery = y;
		}


	}
}
void updatewidepaddle() {
	if (wideactive) {
		widetimer--;

		if (widetimer <= 0) {
			wideactive = false;
			widetimer = 0;
			paddle.setSize({ 100.f, 14.f });

			if (paddleX > 500)
				paddleX = 500;
		}
	}
}
void applypowerup() {
	if (powerup == 'W') {
		wideactive = true;
		widetimer = 600;
		paddle.setSize({ 150.f, 14.f });

	}
	else
		lives++;
}
void updatepowerup() {
	if (!power)
		return;
	bool collision = (powerx + 25 >= paddleX) && (powerx < paddleX + 100) && powery + 20 >= 650.f && powery <= 664.f;
	if (wideactive)
		collision = (powerx + 25 >= paddleX) && (powerx < paddleX + 150) && powery + 20 >= 650.f && powery <= 664.f;
	if (powery <= 700 && !collision) {
		powery += 3;
	}

	if (collision) {
		applypowerup();
		power = false;
	}
	if (powery > 700)
		power = false;


}

void drawPowerUp(RenderWindow& window) {
	if (power) {
		powerShape.setPosition({ powerx, powery });

		Color c = (powerup == 'W') ? mk(60, 255, 130) : mk(70, 150, 255);
		float pulse = 0.5f + 0.5f * sin(fxNow * 8.f);

		fxGlow(window, powerx + 12.5f, powery + 10.f, 30.f + 6.f * pulse, mk(c.r, c.g, c.b, 150), 8);
		fxRoundRect(window, powerx - 1, powery - 1, 27, 22, 9, fxLerp(c, mk(0, 0, 0), 0.5f));
		fxGradRect(window, powerx + 2, powery + 2, 21, 16,
			fxLerp(c, mk(255, 255, 255), 0.5f), fxLerp(c, mk(0, 0, 0), 0.2f));
		fxText(window, string(1, powerup), 16, powerx + 7.f, powery - 1.f,
			Color::White, mk(0, 0, 0, 160), 1.f, false);

		if (fxRnd() < 0.5f)
			fxBurst(powerx + 12.f, powery + 10.f, c, 1, 40.f);
	}
}

void fxDrawPaddle(RenderWindow& w) {
	Vector2f p = paddle.getPosition();
	Vector2f s = paddle.getSize();
	Color c1 = wideactive ? mk(150, 255, 180) : mk(120, 235, 255);
	Color c2 = wideactive ? mk(20, 160, 80) : mk(40, 90, 230);

	for (int i = 5; i >= 1; i--)
		fxRoundRect(w, p.x - i * 2.5f, p.y - i * 1.5f, s.x + i * 5.f, s.y + i * 3.f, 7 + i,
			mk(c2.r, c2.g, c2.b, 16));

	fxRoundRect(w, p.x, p.y, s.x, s.y, 7, fxLerp(c2, mk(0, 0, 0), 0.5f));
	fxGradRect(w, p.x + 4, p.y + 2, s.x - 8, s.y - 4, c1, c2);
	RectangleShape hi({ s.x - 18, 2.f });
	hi.setPosition({ p.x + 9, p.y + 3 });
	hi.setFillColor(mk(255, 255, 255, 150));
	w.draw(hi);

	// glowing end-caps
	fxGlow(w, p.x + 7, p.y + s.y / 2, 12, mk(c1.r, c1.g, c1.b, 160), 5);
	fxGlow(w, p.x + s.x - 7, p.y + s.y / 2, 12, mk(c1.r, c1.g, c1.b, 160), 5);

	// energy bar when the wide power-up is active
	if (wideactive) {
		RectangleShape bar({ (s.x - 10) * widetimer / 600.f, 3.f });
		bar.setPosition({ p.x + 5, p.y + s.y + 5 });
		bar.setFillColor(mk(100, 255, 150, 190));
		w.draw(bar, RenderStates(BlendAdd));
	}
}

void fxDrawBall(RenderWindow& w) {
	Vector2f bp = ball.getPosition();
	Vector2f ctr = { bp.x + 6.f, bp.y + 6.f };

	if (gamestart) {
		if (!fxTrail.empty()) {
			float ddx = ctr.x - fxTrail.back().x, ddy = ctr.y - fxTrail.back().y;
			if (ddx * ddx + ddy * ddy > 2500.f) fxTrail.clear();
		}
		fxTrail.push_back(ctr);
		if (fxTrail.size() > 14) fxTrail.pop_front();
	}
	else fxTrail.clear();

	CircleShape t(6.f);
	for (size_t i = 0; i < fxTrail.size(); i++) {
		float k = (float)(i + 1) / fxTrail.size();
		float r = 6.f * k;
		t.setRadius(r);
		t.setOrigin({ r, r });
		t.setPosition(fxTrail[i]);
		t.setFillColor(mk(255, 170, 70, (int)(110 * k)));
		w.draw(t, RenderStates(BlendAdd));
	}

	fxGlow(w, ctr.x, ctr.y, 24.f, mk(255, 220, 140, 170), 8);
	CircleShape core(6.f);
	core.setOrigin({ 6.f, 6.f });
	core.setPosition(ctr);
	core.setFillColor(mk(255, 255, 255));
	core.setOutlineThickness(1.5f);
	core.setOutlineColor(mk(255, 200, 90));
	w.draw(core);
	CircleShape spec(2.f);
	spec.setOrigin({ 2.f, 2.f });
	spec.setPosition({ ctr.x - 2.f, ctr.y - 2.f });
	spec.setFillColor(mk(255, 255, 255, 230));
	w.draw(spec);
}

void drawGameOver(RenderWindow& window) {
	fxTick();
	fxTrackBricks(false);

	window.clear(Color(20, 20, 35));
	fxBackground(window);

	// falling confetti on win, embers on lose
	RectangleShape cf;
	for (int i = 0; i < 70; i++) {
		float x = fmodf(i * 83.17f + 30.f * sin(fxNow + i), 600.f);
		float y = won ? fmodf(i * 41.3f + fxNow * (60.f + (i % 5) * 25.f), 720.f)
		              : 720.f - fmodf(i * 41.3f + fxNow * (30.f + (i % 5) * 15.f), 720.f);
		cf.setSize({ 5.f, 8.f });
		cf.setOrigin({ 2.5f, 4.f });
		cf.setRotation(degrees(fxNow * 180.f + i * 40.f));
		cf.setPosition({ x, y });
		Color col = won ? fxBrickColor(1 + i % 3) : mk(255, 120 + (i % 4) * 25, 40);
		cf.setFillColor(mk(col.r, col.g, col.b, 190));
		window.draw(cf, RenderStates(BlendAdd));
	}

	Color accent = won ? mk(80, 255, 140) : mk(255, 60, 90);
	fxGlow(window, 300, 230, 260, mk(accent.r, accent.g, accent.b, 90), 10);

	// panel
	fxRoundRect(window, 60, 130, 480, 380, 22, mk(accent.r, accent.g, accent.b, 60));
	fxRoundRect(window, 64, 134, 472, 372, 20, mk(14, 14, 34, 235), mk(accent.r, accent.g, accent.b, 200), 2.f);

	float bob = 4.f * sin(fxNow * 2.f);
	fxGlowText(window, won ? "YOU WIN!" : "GAME OVER", 58, 175.f + bob, accent, accent);

	fxText(window, "FINAL SCORE", 18, 0, 300.f, mk(160, 170, 220), mk(0, 0, 0, 120), 1.f, true, false);
	fxText(window, to_string(score), 52, 0, 325.f, Color::White, accent, 2.f, true);

	int a = (int)(150 + 105 * sin(fxNow * 4.f));
	fxText(window, "Press ENTER to return to MENU", 22, 0, 440.f, mk(255, 230, 90, a), mk(0, 0, 0, 160), 1.f, true);

	window.display();
}

void handleinput(RenderWindow& window) {

	while (const optional event = window.pollEvent()) {

		if (event->is<Event::Closed>())
			window.close();

		if (const auto* key = event->getIf<Event::KeyPressed>()) {

			switch (gamestate) {

			case MENU: {

				if (key->code == Keyboard::Key::Down ||
					key->code == Keyboard::Key::S) {

					selectedOption++;

					if (selectedOption > 2)
						selectedOption = 0;
				}

				if (key->code == Keyboard::Key::Up ||
					key->code == Keyboard::Key::W) {

					selectedOption--;

					if (selectedOption < 0)
						selectedOption = 2;
				}
				if (key->code == Keyboard::Key::Enter) {
					if (selectedOption == 0) {
						newgame();
					}
					else if (selectedOption == 1) {
						if (loadgame()) {
							showNosave = false;
							gamestate = PLAYING;
						}
						else {
							showNosave = true;
						}
					}
					else if (selectedOption == 2) {
						window.close();
					}

				}
				break;
			}

			case PLAYING: {
				if (key->code == Keyboard::Key::Escape) {
					savegame();
					gamestate = MENU;
					selectedOption = 0;
				}


				if (!gamestart &&
					key->code == Keyboard::Key::Space) {
					gamestart = true;
				}
				break;
			}

			case GAMEOVER: {
				if (key->code == Keyboard::Key::Enter) {
					gamestate = MENU;
					selectedOption = 0;
				}
				break;
			}

			}
		}
	}
}

void update() {

	updatepowerup();
	updatewidepaddle();

	ballX += dx;

	if (brickcollision()) {
		ballX -= dx;
		dx = -dx;
	}


	ballY += dy;

	if (brickcollision()) {
		ballY -= dy;
		dy = -dy;
	}

	if (allBricksDestroyed()) {
		won = true;
		gamestate = GAMEOVER;
		ofstream out("savegame.txt", ios::trunc);
		out.close();
	}

	paddle.setPosition({ (float)paddleX, 650.f });
	if (!gamestart)
		ballX = paddleX + 44;

	if (ballX < 0 || ballX > 588)
		dx = -dx;

	if (ballY < 50) {
		dy = -dy;
		ballY = 50;
	}

	if (ballX < 0)
		ballX = 0;
	if (ballX > 588)
		ballX = 588;
	if (!wideactive) {
		if (dy > 0 && (ballY + 12 >= 650 && ballY <= 664) && (ballX >= paddleX - 12 && ballX <= paddleX + 100)) {
			dy = -5;
			ballY = 638;
			if (ballX + 6 >= paddleX && ballX + 6 < paddleX + 25)
				dx = -5;
			else if (ballX + 6 >= paddleX + 25 && ballX + 6 < paddleX + 50)
				dx = -3;
			else if (ballX + 6 >= paddleX + 50 && ballX + 6 < paddleX + 75)
				dx = 3;
			else if (ballX + 6 >= paddleX + 75 && ballX + 6 <= paddleX + 100)
				dx = 5;
		}
	}
	else {
		if (dy > 0 && (ballY + 12 >= 650 && ballY <= 664) &&
			(ballX >= paddleX - 12 && ballX <= paddleX + 150)) {

			dy = -5;
			ballY = 638;

			if (ballX + 6 >= paddleX && ballX + 6 < paddleX + 37.5)
				dx = -5;
			else if (ballX + 6 >= paddleX + 37.5 && ballX + 6 < paddleX + 75)
				dx = -3;
			else if (ballX + 6 >= paddleX + 75 && ballX + 6 < paddleX + 112.5)
				dx = 3;
			else if (ballX + 6 >= paddleX + 112.5 && ballX + 6 <= paddleX + 150)
				dx = 5;
		}
	}


	ball.setPosition({ (float)ballX, (float)ballY });
	if (ballY > 700) {
		lives--;

		if (lives <= 0) {
			lives = 0;
			gamestate = GAMEOVER;

			ofstream out("savegame.txt", ios::trunc);
			out.close();

			return;
		}

		gamestart = false;
		gamestarted = true;
		dx = 0;
		dy = 0;
		ballX = 294.f;
		ballY = 638.f;
	}
}

void draw(RenderWindow& window) {
	fxTick();
	window.clear();
	fxBackground(window);
	fxTrackBricks(true);

	// glowing side walls
	fxGradRect(window, 0, 50, 3, 650, mk(90, 120, 255, 90), mk(90, 120, 255, 0));
	fxGradRect(window, 597, 50, 3, 650, mk(90, 120, 255, 90), mk(90, 120, 255, 0));

	// HUD
	RectangleShape hud({ 600.f, 50.f });
	hud.setFillColor(Color(40, 40, 70));
	fxGradRect(window, 0, 0, 600, 50, mk(48, 40, 100), mk(20, 18, 48));
	fxGradRect(window, 0, 48, 600, 2, mk(90, 220, 255, 230), mk(180, 90, 255, 230));
	fxGlow(window, 300, 50, 300, mk(110, 130, 255, 40), 3);

	drawbricks(window);

	fxUpdateDrawParticles(window);
	fxDrawPaddle(window);
	fxDrawBall(window);
	drawPowerUp(window);

	scoreText.setString("Score: " + to_string(score));
	livesText.setString("x" + to_string(lives));
	livesText.setPosition({ 478.f, 15.f });

	Text sh = scoreText;
	sh.setFillColor(mk(0, 0, 0, 160));
	sh.setPosition({ 12.f, 17.f });
	window.draw(sh);
	window.draw(scoreText);

	float beat = 1.f + 0.08f * sin(fxNow * 6.f);
	fxGlow(window, 458.f, 26.f, 18.f, mk(255, 60, 100, 110), 5);
	fxHeart(window, 458.f, 24.f, 26.f * beat, mk(255, 70, 110));
	window.draw(livesText);

	if (!gamestart) {
		int a = (int)(150 + 105 * sin(fxNow * 5.f));
		fxText(window, "PRESS SPACE TO LAUNCH", 20, 0, 560.f, mk(255, 235, 120, a), mk(0, 0, 0, 160), 1.5f, true);
		fxText(window, "A / D  or  < >  to move   |   ESC to save & quit", 14, 0, 590.f,
			mk(170, 180, 230, 170), mk(0, 0, 0, 120), 1.f, true, false);
	}

	window.display();
}

void drawMenu(RenderWindow& window) {
	fxTick();
	fxTrackBricks(false);

	window.clear(Color(20, 25, 40));
	fxBackground(window);

	titleText.setString("BLOCK BUSTER");
	titleText.setCharacterSize(55);
	titleText.setFillColor(Color::White);
	titleText.setPosition({ 95.f, 130.f });

	// animated, colour-shifting glowing title (same string/size as before)
	Color tc = mk((int)(200 + 55 * sin(fxNow)), (int)(215 + 40 * sin(fxNow * 1.3f + 1.f)), 255);
	fxGlowText(window, "BLOCK BUSTER", 55, 125.f + 5.f * sin(fxNow * 1.5f), tc, mk(90, 150, 255));

	// decorative bobbing brick row + bouncing ball
	for (int i = 0; i < 10; i++) {
		float yy = 225.f + 7.f * sin(fxNow * 2.f + i * 0.6f);
		Color col = fxBrickColor(1 + i % 3);
		float xx = 30.f + i * 54.f;
		fxRoundRect(window, xx - 2, yy - 2, 50, 20, 6, mk(col.r, col.g, col.b, 40));
		fxRoundRect(window, xx, yy, 46, 16, 4, fxLerp(col, mk(0, 0, 0), 0.4f));
		fxGradRect(window, xx + 2, yy + 2, 42, 12, fxLerp(col, mk(255, 255, 255), 0.35f), fxLerp(col, mk(0, 0, 0), 0.2f));
	}
	float bx = 300.f + 230.f * sin(fxNow * 1.7f);
	float by = 300.f - fabsf(34.f * sin(fxNow * 3.4f));
	fxGlow(window, bx, by, 20.f, mk(255, 220, 140, 160), 7);
	CircleShape db(6.f);
	db.setOrigin({ 6.f, 6.f });
	db.setPosition({ bx, by });
	window.draw(db);

	if (showNosave) {
		fxText(window, "No saved game", 22, 0, 550.f, mk(255, 90, 110), mk(0, 0, 0, 170), 1.5f, true);
	}

	string options[3] = {
		"NEW GAME",
		"CONTINUE",
		"EXIT"
	};

	for (int i = 0; i < 3; i++) {

		menuOptions[i].setString(options[i]);
		menuOptions[i].setCharacterSize(30);
		menuOptions[i].setStyle(Text::Bold);

		float oy = 330.f + i * 65.f;
		FloatRect b = menuOptions[i].getLocalBounds();
		float ox = 300.f - b.size.x / 2.f - b.position.x;
		menuOptions[i].setPosition({ ox, oy });

		if (i == selectedOption) {
			float pulse = 0.5f + 0.5f * sin(fxNow * 6.f);
			fxGlow(window, 300.f, oy + 21.f, 150.f, mk(255, 210, 60, 70), 6);
			fxRoundRect(window, 120, oy - 4, 360, 50, 14, mk(255, 200, 60, 70));
			fxRoundRect(window, 122, oy - 2, 356, 46, 12, mk(30, 26, 60, 235), mk(255, 220, 90, (int)(170 + 80 * pulse)), 2.f);
			fxText(window, ">", 30, 140.f + 6.f * pulse, oy, mk(255, 220, 90), mk(0, 0, 0, 120), 1.f, false);
			fxText(window, "<", 30, 438.f - 6.f * pulse, oy, mk(255, 220, 90), mk(0, 0, 0, 120), 1.f, false);
			menuOptions[i].setFillColor(Color::Yellow);
			menuOptions[i].setOutlineColor(mk(120, 70, 0, 200));
			menuOptions[i].setOutlineThickness(1.5f);
		}
		else {
			fxRoundRect(window, 140, oy - 2, 320, 46, 12, mk(255, 255, 255, 10), mk(150, 160, 255, 60), 1.f);
			menuOptions[i].setFillColor(mk(190, 195, 235));
			menuOptions[i].setOutlineColor(mk(0, 0, 0, 150));
			menuOptions[i].setOutlineThickness(1.f);
		}

		window.draw(menuOptions[i]);
	}

	fxText(window, "W / S  or  UP / DOWN  to choose    |    ENTER to select", 15, 0, 650.f,
		mk(160, 170, 220, 190), mk(0, 0, 0, 120), 1.f, true, false);

	window.display();
}


int main() {



	RenderWindow window(VideoMode({ 600,700 }), "Block Buster");
	if (!font.openFromFile("arial.ttf")) {
		cout << "Font loading failed.";
		return 1;
	}

	scoreText.setCharacterSize(24);
	scoreText.setStyle(Text::Bold);
	scoreText.setFillColor(mk(255, 245, 210));
	scoreText.setOutlineColor(mk(0, 0, 0, 180));
	scoreText.setOutlineThickness(1.5f);
	scoreText.setPosition({ 10.f, 15.f });

	livesText.setCharacterSize(24);
	livesText.setStyle(Text::Bold);
	livesText.setFillColor(mk(255, 200, 215));
	livesText.setOutlineColor(mk(0, 0, 0, 180));
	livesText.setOutlineThickness(1.5f);
	livesText.setPosition({ 450.f, 15.f });
	window.setFramerateLimit(60);

	initObjects();


	while (window.isOpen()) {



		handleinput(window);
		switch (gamestate) {
		case MENU: {
			drawMenu(window);
			break;
		}
		case PLAYING: {
			if (gamestart && gamestarted)
				initalmoveball(dx, dy);
			movePaddle();
			update();
			draw(window);
			break;
		}
		case GAMEOVER: {
			drawGameOver(window);
			break;
		}
		}




	}




	return 0;
}