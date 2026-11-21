#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <time.h>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <iostream>
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
bool gamestart = false, gamestarted = true, spacepressed = false, showNosave = false, power = false, wideactive = false;
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
void drawbricks(RenderWindow& window) {

	const float BRICK_W = 60.f, BRICK_H = 25.f;   // one brick is 60 x 25 pixels 
	const float BRICKS_Y = 100.f;                 // the bricks start at y = 100 

	sf::Color colors[4] = { Color(25, 25, 45),    // 0: no brick 
							sf::Color::Green,         // 1: one hit left 
							sf::Color::Yellow,        // 2: two hits left 
							sf::Color::Red };         // 3: three hits left 

	sf::RectangleShape brick({ BRICK_W - 2, BRICK_H - 2 });   // smaller, so a gap shows 

	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			if (bricks[r][c] != 0) {
				brick.setFillColor(colors[bricks[r][c]]);
				brick.setPosition({ c * BRICK_W + 1, BRICKS_Y + r * BRICK_H + 1 });
				window.draw(brick);
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
void initalmoveball(int &dx, int &dy) {
	
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
					spawnpowerup(c*60, (r*25) +100);
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
	if(wideactive)
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

		if (powerup == 'W')
			powerShape.setFillColor(Color::Green);
		else
			powerShape.setFillColor(Color::Blue);

		window.draw(powerShape);
	}
}

void drawGameOver(RenderWindow& window) {

	window.clear(Color(20, 20, 35));

	Text heading(font);
	Text finalScore(font);
	Text instruction(font);

	
	if (won)
		heading.setString("YOU WIN!");
	else
		heading.setString("GAME OVER");

	heading.setCharacterSize(55);
	heading.setFillColor(won ? Color::Green : Color::Red);
	heading.setPosition({ 115.f, 180.f });

	
	finalScore.setString("Final Score: " + to_string(score));
	finalScore.setCharacterSize(30);
	finalScore.setFillColor(Color::White);
	finalScore.setPosition({ 175.f, 310.f });

	
	instruction.setString("Press ENTER to return to MENU");
	instruction.setCharacterSize(22);
	instruction.setFillColor(Color::Yellow);
	instruction.setPosition({ 105.f, 430.f });

	window.draw(heading);
	window.draw(finalScore);
	window.draw(instruction);

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

	if (ballY < 0) {
		dy = -dy;
		ballY = 0;
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
	window.clear();

	drawbricks(window);

	window.draw(paddle);
	window.draw(ball);
	drawPowerUp(window);

	scoreText.setString("Score: " + to_string(score));
	livesText.setString("Lives: " + to_string(lives));

	window.draw(scoreText);
	window.draw(livesText);

	window.display();
}

void drawMenu(RenderWindow& window) {

	window.clear(Color(20, 25, 40));

	titleText.setString("BLOCK BUSTER");
	titleText.setCharacterSize(55);
	titleText.setFillColor(Color::White);
	titleText.setPosition({ 95.f, 130.f });

	window.draw(titleText);

	if (showNosave) {
		Text message(font);
		message.setString("No saved game");
		message.setCharacterSize(22);
		message.setFillColor(Color::Red);
		message.setPosition({ 190.f, 550.f });
		window.draw(message);
	}

	string options[3] = {
		"NEW GAME",
		"CONTINUE",
		"EXIT"
	};

	for (int i = 0; i < 3; i++) {

		menuOptions[i].setString(options[i]);
		menuOptions[i].setCharacterSize(30);
		menuOptions[i].setPosition({ 210.f, 330.f + i * 65.f });

		if (i == selectedOption)
			menuOptions[i].setFillColor(Color::Yellow);
		else
			menuOptions[i].setFillColor(Color::White);

		window.draw(menuOptions[i]);
	}

	window.display();
}


int main() {
	
	
	
	RenderWindow window(VideoMode({ 600,700 }), "Block Buster");
	if (!font.openFromFile("arial.ttf")) {
		cout << "Font loading failed.";
		return 1;
	}

	scoreText.setCharacterSize(24);
	scoreText.setFillColor(Color::White);
	scoreText.setPosition({ 10.f, 15.f });

	livesText.setCharacterSize(24);
	livesText.setFillColor(Color::White);
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