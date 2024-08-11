#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <cstdlib>
#include <cmath>

using namespace sf;
using namespace std;

const int mapWidth = 6400;
const int mapHeight = 4800;
const int viewWidth = 640;
const int viewHeight = 480;
const float playerSpeed = 0.3f;
const float bulletSpeed = 4 * playerSpeed;
const float maxBulletDistance = 2.0f * viewWidth;

unique_ptr<Shape> randomShapeGenerator(int i, Vector2f position) {
    if (i < 70) {  // 70% chance to generate a rectangle
        auto rect = make_unique<RectangleShape>(Vector2f(30.0f, 30.0f));
        rect->setFillColor(Color::Blue);
        rect->setPosition(position);
        return rect;
    } else {  // 30% chance to generate a circle
        auto circle = make_unique<CircleShape>(25.0f);
        circle->setFillColor(Color::Red);
        circle->setPosition(position);
        return circle;
    }
}

bool checkCollision(const Shape& newShape, const vector<unique_ptr<Shape>>& shapes) {
    FloatRect newBounds = newShape.getGlobalBounds();
    for (const auto& shape : shapes) {
        FloatRect existingBounds = shape->getGlobalBounds();
        if (newBounds.intersects(existingBounds)) {
            return true;
        }
    }
    return false;
}

bool checkShapeCollision(const Shape& shape1, const Shape& shape2) {
    return shape1.getGlobalBounds().intersects(shape2.getGlobalBounds());
}

Vector2f normalize(Vector2f v) {
    float length = sqrt(v.x * v.x + v.y * v.y);
    return (length != 0) ? v / length : Vector2f(0, 0);
}

struct Bullet {
    CircleShape shape;
    Vector2f direction;
    float distanceTravelled = 0.0f;  // Track the distance traveled
};

void fireBullet(vector<Bullet>& bullets, const vector<CircleShape>& smallCircles, const Vector2f& mousePosition) {
    for (const auto& smallCircle : smallCircles) {
        Vector2f circleCenter = smallCircle.getPosition() + Vector2f(smallCircle.getRadius(), smallCircle.getRadius());
        Vector2f bulletDirection = normalize(mousePosition - circleCenter);

        Bullet bullet;
        bullet.shape.setRadius(5.f);
        bullet.shape.setFillColor(Color::Black);
        bullet.shape.setPosition(circleCenter - Vector2f(bullet.shape.getRadius(), bullet.shape.getRadius()));
        bullet.direction = bulletDirection * bulletSpeed; // Bullet speed

        bullets.push_back(bullet);
    }
}

void updateBullets(vector<Bullet>& bullets, RenderWindow& window) {
    for (auto& bullet : bullets) {
        bullet.shape.move(bullet.direction);
        bullet.distanceTravelled += sqrt(bullet.direction.x * bullet.direction.x + bullet.direction.y * bullet.direction.y);
    }

    bullets.erase(remove_if(bullets.begin(), bullets.end(), [](const Bullet& bullet) {
        return bullet.shape.getPosition().x < 0 || bullet.shape.getPosition().x > mapWidth ||
               bullet.shape.getPosition().y < 0 || bullet.shape.getPosition().y > mapHeight ||
               bullet.distanceTravelled > maxBulletDistance;
    }), bullets.end());
}

void drawGrid(RenderWindow& window, const View& view) {
    RectangleShape line(Vector2f(mapWidth, 1));
    line.setFillColor(Color(200, 200, 200));  // Light grey lines
    
    for (int i = 0; i < mapHeight; i += 100) {  // Horizontal lines
        line.setPosition(0, i);
        window.draw(line);
    }
    
    line.setSize(Vector2f(1, mapHeight));
    for (int i = 0; i < mapWidth; i += 100) {  // Vertical lines
        line.setPosition(i, 0);
        window.draw(line);
    }
}

int main() {
    RenderWindow window(VideoMode(viewWidth, viewHeight), "Diep.io", Style::Default);
    View view(FloatRect(0, 0, viewWidth, viewHeight));
    vector<unique_ptr<Shape>> shapesToDraw;
    vector<Vector2f> position;
    vector<Bullet> bullets;
    srand(static_cast<unsigned>(time(NULL)));

    // Define the large circle
    CircleShape largeCircle(40.0f);
    largeCircle.setFillColor(Color::Green);
    largeCircle.setPosition(mapWidth / 2 - largeCircle.getRadius(), mapHeight / 2 - largeCircle.getRadius());

    // Define the small circle(s)
    vector<CircleShape> smallCircles = { CircleShape(10.0f) };
    smallCircles[0].setFillColor(Color::Red);

    float orbitRadius = largeCircle.getRadius() + smallCircles[0].getRadius();
    float angle = 0.0f;

    Vector2f largeCircleCenter(mapWidth / 2, mapHeight / 2);

    bool collisionOccurred = false;
    int score = 0;
    Clock clock;
    Time lastFireTime = Time::Zero;

    Font font;
    if (!font.loadFromFile("Arial.ttf")) {
        return -1;  // Handle font loading failure
    }

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
        }

        // Player movement
        if (Keyboard::isKeyPressed(Keyboard::A)) {
            largeCircle.move(Vector2f(-playerSpeed, 0.f));
        }
        if (Keyboard::isKeyPressed(Keyboard::W)) {
            largeCircle.move(Vector2f(0, -playerSpeed));
        }
        if (Keyboard::isKeyPressed(Keyboard::D)) {
            largeCircle.move(Vector2f(playerSpeed, 0));
        }
        if (Keyboard::isKeyPressed(Keyboard::S)) {
            largeCircle.move(Vector2f(0, playerSpeed));
        }
        largeCircleCenter = largeCircle.getPosition() + Vector2f(largeCircle.getRadius(), largeCircle.getRadius());

        // Mouse shooting
        if (Mouse::isButtonPressed(Mouse::Left)) {
            Time currentTime = clock.getElapsedTime();
            if (currentTime - lastFireTime > milliseconds(500)) { // Fire rate: 500 ms
                Vector2f mousePosition = window.mapPixelToCoords(Mouse::getPosition(window), view);
                fireBullet(bullets, smallCircles, mousePosition);
                lastFireTime = currentTime;
            }
        }

        // Update the small circle(s) position
        Vector2f mousePosition = window.mapPixelToCoords(Mouse::getPosition(window), view);
        Vector2f direction = mousePosition - largeCircleCenter;
        angle = atan2(direction.y, direction.x);

        for (int i = 0; i < smallCircles.size(); ++i) {
            float x = largeCircleCenter.x + orbitRadius * cos(angle + i * M_PI / 4); // Offset for multiple circles
            float y = largeCircleCenter.y + orbitRadius * sin(angle + i * M_PI / 4);
            smallCircles[i].setPosition(x - smallCircles[i].getRadius(), y - smallCircles[i].getRadius());
        }

        // Generate random shapes within the map if a shape is destroyed
        if (shapesToDraw.size() < 100) {
            int i = rand() % 100;
            bool collide;

            do {
                collide = false;
                Vector2f newPos(rand() % (mapWidth - 50), rand() % (mapHeight - 50));
                auto newShape = randomShapeGenerator(i, newPos);
                collide = checkCollision(*newShape, shapesToDraw) || checkShapeCollision(*newShape, largeCircle);
                if (!collide) {
                    shapesToDraw.push_back(move(newShape));
                }
            } while (collide);
        }

        // Check for collision between large circle and shapes
        auto it = shapesToDraw.begin();
        while (it != shapesToDraw.end()) {
            if (checkShapeCollision(**it, largeCircle)) {
                collisionOccurred = true;
                break;
            }
            ++it;
        }

        // Check for collision between shapes and bullets
        it = shapesToDraw.begin();
        while (it != shapesToDraw.end()) {
            bool shapeCollided = false;
            for (const auto& bullet : bullets) {
                if (checkShapeCollision(**it, bullet.shape)) {
                    if (dynamic_cast<RectangleShape*>(it->get())) {
                        score += 1;  // Score increases by 1 for rectangles
                    } else if (dynamic_cast<CircleShape*>(it->get())) {
                        score += 2;  // Score increases by 2 for circles
                    }
                    shapeCollided = true;
                    break;
                }
            }
            if (shapeCollided) {
                it = shapesToDraw.erase(it); // Remove colliding shape
            } else {
                ++it;
            }
        }

        // Update bullets
        updateBullets(bullets, window);

        // Update view
        view.setCenter(largeCircleCenter);
        window.setView(view);

        window.clear(Color::White);

        drawGrid(window, view);

        for (const auto& shape : shapesToDraw) {
            window.draw(*shape);
        }

        // Draw large circle and small circles
        window.draw(largeCircle);
        for (const auto& smallCircle : smallCircles) {
            window.draw(smallCircle);
        }

        // Draw bullets
        for (const auto& bullet : bullets) {
            window.draw(bullet.shape);
        }

        // Score handling and spawning additional small circle
        if (score >= 50 && smallCircles.size() == 1) {
            CircleShape secondSmallCircle = smallCircles[0]; // Copy the existing small circle
            smallCircles.push_back(secondSmallCircle); // Add a second small circle
        }

        // Display score
        Text scoreText;
        scoreText.setFont(font);
        scoreText.setString("Score: " + to_string(score));
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(Color::Black);
        scoreText.setPosition(view.getCenter().x - viewWidth / 2 + 10, view.getCenter().y - viewHeight / 2 + 10);
        window.draw(scoreText);

        window.display();
    }

    return 0;
}
