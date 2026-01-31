#include <graphics.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>

struct Point
{
    int x, y;
};

struct Seed
{
    double x, y;
    double angle;
    double velocityY;
    bool active;
};

class AnimatedTreeDrawer
{
private:
    int screenWidth, screenHeight;
    int groundLevel;
    int seedX, seedY;
    std::chrono::steady_clock::time_point lastTime;
    double sunAngle; // for moving sun

    // Animation state variables
    double treeGrowthScale;
    double flowerScale;
    bool showFlowers;
    int flowerPosX, flowerPosY;
    std::vector<Point> flowerPositions;
    std::vector<Seed> fallingSeeds;
    int animationPhase; // 0: seed germination, 1: tree growth, 2: flowering, 3: seed fall, 4: reset
    int phaseTimer;
    int rightmostBranchX, rightmostBranchY;
    double zoomScale;
    int cameraOffsetX, cameraOffsetY;

    // Colors
    const int BROWN = COLOR(139, 69, 19);
    const int DARK_BROWN = COLOR(101, 67, 33);
    const int LEAF_GREEN = COLOR(34, 139, 34);
    const int LIGHT_GREEN = COLOR(50, 205, 50);
    const int SKY_BLUE = COLOR(135, 206, 235);
    const int SOIL_BROWN = COLOR(90, 50, 20);

    // Draw a seed with rotation
    void drawSeed(int x, int y, double angle, double scale = 1.0)
    {
        int oldColor = getcolor();

        setcolor(COLOR(160, 82, 45));
        setfillstyle(SOLID_FILL, COLOR(160, 82, 45));

        int size = static_cast<int>(8 * scale);

        // Draw seed as rotated oval using rotation transformation
        // Calculate 4 points of the oval and rotate them
        const int numPoints = 12;
        int points[numPoints * 2];

        for (int i = 0; i < numPoints; i++)
        {
            double t = i * 2 * 3.14159 / numPoints;
            // Oval shape (wider than tall)
            double localX = size * cos(t);
            double localY = size * 0.5 * sin(t);

            // Apply rotation transformation
            int rotatedX = static_cast<int>(localX * cos(angle) - localY * sin(angle));
            int rotatedY = static_cast<int>(localX * sin(angle) + localY * cos(angle));

            points[i * 2] = x + rotatedX;
            points[i * 2 + 1] = y + rotatedY;
        }

        // Fill the rotated seed
        fillpoly(numPoints, points);

        // Draw a line through the seed to show rotation clearly
        int lineLength = static_cast<int>(size * 0.8);
        int lineX1 = x + static_cast<int>(lineLength * cos(angle));
        int lineY1 = y + static_cast<int>(lineLength * sin(angle));
        int lineX2 = x - static_cast<int>(lineLength * cos(angle));
        int lineY2 = y - static_cast<int>(lineLength * sin(angle));

        setcolor(COLOR(100, 50, 20));
        setlinestyle(SOLID_LINE, 0, std::max(1, static_cast<int>(scale / 3)));
        line(lineX1, lineY1, lineX2, lineY2);

        setcolor(oldColor);
    }

    // Draw soil layers
    void drawSoil()
    {
        // Ground surface
        setcolor(DARK_BROWN);
        setfillstyle(SOLID_FILL, DARK_BROWN);
        bar(0, groundLevel, screenWidth, groundLevel + 50);

        // Underground soil (darker)
        setcolor(SOIL_BROWN);
        setfillstyle(SOLID_FILL, SOIL_BROWN);
        bar(0, groundLevel + 50, screenWidth, screenHeight);
    }

    // Draw a branch recursively with scaling
    void drawBranch(int x1, int y1, double length, double angle, int depth, double scale, double growthProgress = 1.0)
    {
        if (depth <= 0 || scale <= 0.1)
            return;

        double branchProgress = std::min(1.0, std::max(0.0, (growthProgress * 10) - (8 - depth)));
        if (branchProgress <= 0)
            return;

        double scaledLength = length * scale * branchProgress;

        int x2 = x1 + static_cast<int>(scaledLength * cos(angle));
        int y2 = y1 - static_cast<int>(scaledLength * sin(angle));

        // Track rightmost position for flower/seed
        if (depth == 1 && x2 > rightmostBranchX)
        {
            rightmostBranchX = x2;
            rightmostBranchY = y2;
        }

        if (depth > 4)
        {
            setcolor(BROWN);
            setlinestyle(SOLID_LINE, 0, static_cast<int>(depth * scale) + 1);
        }
        else
        {
            setcolor(LEAF_GREEN);
            setlinestyle(SOLID_LINE, 0, std::max(1, static_cast<int>(depth * scale)));
        }

        line(x1, y1, x2, y2);

        if (depth <= 5 && scale > 0.5 && branchProgress > 0.8)
        {
            setcolor(LIGHT_GREEN);
            setfillstyle(SOLID_FILL, LIGHT_GREEN);

            int numLeaves = (depth <= 3) ? 3 : 2;
            for (int i = 0; i < numLeaves; i++)
            {
                int leafX = x2 + (rand() % 10 - 5);
                int leafY = y2 + (rand() % 10 - 5);
                int leafSize = static_cast<int>(4 * scale);
                fillellipse(leafX, leafY, leafSize, leafSize);
            }
        }

        // Draw flowers only at depth 1
        if (depth == 1 && showFlowers && scale > 0.8 && branchProgress > 0.9)
        {
            drawFlower(x2, y2, flowerScale);
        }

        double newLength = length * 0.7;
        drawBranch(x2, y2, newLength, angle - 0.3, depth - 1, scale, growthProgress);
        drawBranch(x2, y2, newLength, angle + 0.3, depth - 1, scale, growthProgress);
        drawBranch(x2, y2, newLength * 0.8, angle, depth - 1, scale, growthProgress);
    }

    void drawFlower(int x, int y, double scale)
    {
        if (scale <= 0)
            return;

        // Collect all flower positions
        Point flowerPos;
        flowerPos.x = x;
        flowerPos.y = y;
        flowerPositions.push_back(flowerPos);

        int petalSize = static_cast<int>(4 * scale);

        setcolor(COLOR(255, 192, 203));
        setfillstyle(SOLID_FILL, COLOR(255, 192, 203));

        for (int i = 0; i < 5; i++)
        {
            double angle = i * 2 * 3.14159 / 5;
            int petalX = x + static_cast<int>(petalSize * cos(angle));
            int petalY = y + static_cast<int>(petalSize * sin(angle));
            fillellipse(petalX, petalY, petalSize, petalSize);
        }

        setcolor(YELLOW);
        setfillstyle(SOLID_FILL, YELLOW);
        fillellipse(x, y, petalSize - 1, petalSize - 1);
    }

    // Draw the sun
    void drawSun()
    {
        int radius = 30;
        int skyHeight = 150;
        int sunX = static_cast<int>(screenWidth * sunAngle / 3.14159);
        int sunY = static_cast<int>(skyHeight * sin(sunAngle)) + 50;

        setcolor(YELLOW);
        setfillstyle(SOLID_FILL, YELLOW);
        fillellipse(sunX, sunY, radius, radius);

        for (int i = 0; i < 12; i++)
        {
            double angle = i * 30 * 3.14159 / 180.0;
            int x1 = sunX + static_cast<int>((radius + 5) * cos(angle));
            int y1 = sunY + static_cast<int>((radius + 5) * sin(angle));
            int x2 = sunX + static_cast<int>((radius + 20) * cos(angle));
            int y2 = sunY + static_cast<int>((radius + 20) * sin(angle));
            line(x1, y1, x2, y2);
        }
    }

    // Draw clouds
    void drawClouds()
    {
        setcolor(WHITE);
        setfillstyle(SOLID_FILL, WHITE);

        for (int cloud = 0; cloud < 3; cloud++)
        {
            int cloudX = 100 + cloud * 200;
            int cloudY = 80 + (cloud * 17) % 50;

            for (int i = 0; i < 5; i++)
            {
                int circleX = cloudX + i * 25;
                int circleY = cloudY + ((i * 13) % 20 - 10);
                int radius = 20 + (i * 7) % 10;
                fillellipse(circleX, circleY, radius, radius);
            }
        }
    }

    // Update falling seeds with physics
    void updateFallingSeeds()
    {
        for (auto &seed : fallingSeeds)
        {
            if (!seed.active)
                continue;

            // Apply gravity
            seed.velocityY += 0.3; // Slightly slower fall
            seed.y += seed.velocityY;

            // Wind effect - drift to the right
            seed.x += 1.5; // Horizontal wind drift

            // Rotation during fall - faster rotation
            seed.angle += 0.2; // Increased from 0.1

            // Check if seed hit the ground
            if (seed.y >= groundLevel + 80)
            {
                seed.active = false;
                seedX = static_cast<int>(seed.x);
                seedY = groundLevel + 25;
            }
        }
    }

    // Display phase information
    void displayPhaseInfo()
    {
        setcolor(WHITE);
        settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);

        char title[100];
        switch (animationPhase)
        {
        case 0:
            sprintf(title, "Phase 1: Seed Germination");
            break;
        case 1:
            sprintf(title, "Phase 2: Seedling (Leaves)");
            break;
        case 2:
            sprintf(title, "Phase 3: Tree Growth");
            break;
        case 3:
            sprintf(title, "Phase 4: Flowering");
            break;
        case 4:
            sprintf(title, "Phase 5: Seed Dispersal");
            break;
        case 5:
            sprintf(title, "Phase 6: Cycle Reset");
            break;
        }
        outtextxy(10, 10, title);

        settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
        char msg[] = "Press ESC to exit, SPACE to restart";
        outtextxy(10, screenHeight - 20, msg);
    }

    void drawSeedlingLeaves(int x, int y, double progress)
    {
        // progress goes from 0 to 1
        int stemHeight = static_cast<int>(60 * progress); // Increased from 30

        // Draw stem (this becomes the trunk)
        setcolor(LEAF_GREEN);
        setlinestyle(SOLID_LINE, 0, std::max(2, static_cast<int>(progress * 4)));
        line(x, y, x, y - stemHeight);

        // Leaves appear and grow
        if (progress > 0.2)
        { // Changed from 0.3 to appear earlier
            double leafProgress = (progress - 0.2) / 0.8;
            int leafSize = static_cast<int>(20 * leafProgress);   // Increased from 15
            int leafYOffset = static_cast<int>(stemHeight * 0.5); // Position leaves partway up stem

            setcolor(LIGHT_GREEN);
            setfillstyle(SOLID_FILL, LIGHT_GREEN);

            // Left leaf - angled outward
            fillellipse(x - leafSize, y - leafYOffset, leafSize, static_cast<int>(leafSize * 0.6));

            // Right leaf - angled outward
            fillellipse(x + leafSize, y - leafYOffset, leafSize, static_cast<int>(leafSize * 0.6));
        }
    }

public:
    AnimatedTreeDrawer()
        : screenWidth(800),
          screenHeight(600),
          groundLevel(480),
          seedX(400),
          seedY(groundLevel + 25),
          sunAngle(0.0),
          treeGrowthScale(0.0),
          flowerScale(0.0),
          showFlowers(false),
          animationPhase(0),
          phaseTimer(0),
          rightmostBranchX(0),
          rightmostBranchY(0),
          zoomScale(1.0),
          cameraOffsetX(0),
          cameraOffsetY(0) {}

    void initialize()
    {
        lastTime = std::chrono::steady_clock::now();
        initwindow(screenWidth, screenHeight, "Animated Tree Life Cycle");
        setbkcolor(SKY_BLUE);
        cleardevice();
        setactivepage(0);
        setvisualpage(0);
    }

    void resetAnimation()
    {
        treeGrowthScale = 0.0;
        flowerScale = 0.0;
        showFlowers = false;
        animationPhase = 0;
        phaseTimer = 0;
        fallingSeeds.clear();
        seedX = screenWidth / 2;
        seedY = groundLevel + 25;
        zoomScale = 1.0;
        cameraOffsetX = 0;
        cameraOffsetY = 0;
        flowerPositions.clear();
        rightmostBranchX = 0;
        rightmostBranchY = 0;
    }

    void update()
    {
        phaseTimer += 2;

        switch (animationPhase)
        {
        case 0: // Seed germination (0-40 frames)
            if (phaseTimer < 40)
            {
                treeGrowthScale = 0.0;
            }
            else
            {
                animationPhase = 1;
                phaseTimer = 0;
            }
            break;

        case 1:
        { // Leaf phase (0-60 frames) - grows smoothly
            if (phaseTimer < 60)
            {
                // Don't set to 0, let it transition smoothly
                treeGrowthScale = std::min(0.15, phaseTimer / 400.0); // Very gradual start
            }
            else
            {
                animationPhase = 2;
                phaseTimer = 0;
            }
            break;
        }

        case 2:
        { // Tree growth (0-100 frames)
            if (phaseTimer < 100)
            {
                treeGrowthScale = 0.15 + (phaseTimer / 100.0) * 0.85; // Continue from 0.15
            }
            else
            {
                animationPhase = 3;
                phaseTimer = 0;
                showFlowers = true;
            }
            break;
        }

        case 3:
        { // Flowering (0-25 frames)
            if (phaseTimer < 25)
            {
                flowerScale = phaseTimer / 25.0;
            }
            else
            {
                animationPhase = 4;
                phaseTimer = 0;

                // Create seed at rightmost branch tip (where flowers are)
                Seed newSeed;
                newSeed.x = rightmostBranchX > 0 ? rightmostBranchX : seedX + 100;
                newSeed.y = rightmostBranchY > 0 ? rightmostBranchY : groundLevel - 150;
                newSeed.angle = 0;
                newSeed.velocityY = 0;
                newSeed.active = true;
                fallingSeeds.clear();
                fallingSeeds.push_back(newSeed);
            }
            break;
        }

        case 4:
        { // Seed dispersal with moderate zoom
            updateFallingSeeds();

            // Get the falling seed position
            if (!fallingSeeds.empty() && fallingSeeds[0].active)
            {
                Seed &seed = fallingSeeds[0];

                // Moderate zoom - 1x to 8x (reduced from 25x)
                if (phaseTimer < 60)
                {
                    zoomScale = 1.0 + (phaseTimer / 55.0) * 7.0;

                    cameraOffsetX = screenWidth / 2 - static_cast<int>(seed.x * zoomScale);
                    cameraOffsetY = screenHeight / 2 - static_cast<int>(seed.y * zoomScale);
                }
            }
            else
            {
                // Seed has landed - hold zoom on the seed in ground
                if (phaseTimer < 90)
                {
                    // Keep zoomed on seed in ground for a moment
                    zoomScale = 8.0;
                    cameraOffsetX = screenWidth / 2 - static_cast<int>(seedX * zoomScale);
                    cameraOffsetY = screenHeight / 2 - static_cast<int>(seedY * zoomScale);
                }
            }

            bool allFallen = true;
            for (const auto &seed : fallingSeeds)
            {
                if (seed.active)
                    allFallen = false;
            }

            // Wait a bit after seed lands before zooming out
            if (allFallen && phaseTimer > 90)
            {
                animationPhase = 5;
                phaseTimer = 0;
            }
            break;
        }

        case 5:
        { // Zoom out from seed in ground to initial view
            if (phaseTimer < 50)
            {
                // Fade out old tree
                treeGrowthScale = 1.0 - (phaseTimer / 50.0);
                flowerScale = 1.0 - (phaseTimer / 50.0);

                // Zoom out from 8x back to 1x
                double zoomProgress = phaseTimer / 50.0;
                zoomScale = 8.0 - (zoomProgress * 7.0);

                // Move camera from seed position back to center
                int targetOffsetX = 0;
                int targetOffsetY = 0;
                int startOffsetX = screenWidth / 2 - static_cast<int>(seedX * 8.0);
                int startOffsetY = screenHeight / 2 - static_cast<int>(seedY * 8.0);

                cameraOffsetX = startOffsetX + static_cast<int>((targetOffsetX - startOffsetX) * zoomProgress);
                cameraOffsetY = startOffsetY + static_cast<int>((targetOffsetY - startOffsetY) * zoomProgress);
            }
            else
            {
                resetAnimation();
            }
            break;
        }
        }
        auto currentTime = std::chrono::steady_clock::now();
        double elapsedSeconds = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

        sunAngle += 0.5 * elapsedSeconds; // radians per second
        if (sunAngle > 2 * 3.14159)
            sunAngle -= 2 * 3.14159;
    }

    void render()
    {
        setactivepage(1 - getactivepage());
        int r = 100 - static_cast<int>(50 * -sin(sunAngle));
        int g = 170 - static_cast<int>(100 * -sin(sunAngle));
        int b = 200 - static_cast<int>(80 * -sin(sunAngle));
        r = std::max(0, std::min(255, r));
        g = std::max(0, std::min(255, g));
        b = std::max(0, std::min(255, b));

        setbkcolor(COLOR(r, g, b));
        cleardevice();

        drawSun();
        drawClouds();

        int drawOffsetX = cameraOffsetX;
        int drawOffsetY = cameraOffsetY;

        int transformedGroundLevel = groundLevel + drawOffsetY;
        int originalGroundLevel = groundLevel;
        groundLevel = transformedGroundLevel;
        drawSoil();
        groundLevel = originalGroundLevel;

        // Draw seed underground
        if (animationPhase <= 1)
        {
            double seedScale = 1.0 + (animationPhase == 0 ? phaseTimer / 20.0 : 2.0);
            int seedDrawX = static_cast<int>((seedX + drawOffsetX) * zoomScale - (zoomScale - 1.0) * screenWidth / 2);
            int seedDrawY = static_cast<int>((seedY + drawOffsetY) * zoomScale - (zoomScale - 1.0) * screenHeight / 2);
            drawSeed(seedDrawX, seedDrawY, 0, seedScale * zoomScale);

            // Draw sprout ONLY during germination and ONLY when seed has started growing
            if (animationPhase == 0 && phaseTimer > 20)
            { // Start after 20 frames
                setcolor(LIGHT_GREEN);
                double sproutProgress = (phaseTimer - 20) / 20.0; // Progress from 0 to 1
                int sproutLength = static_cast<int>(sproutProgress * 20 * zoomScale);
                line(seedDrawX, seedDrawY, seedDrawX, seedDrawY - sproutLength);
            }
        }

        // MORPHING STAGES - all drawn together for smooth transition

        // Stage 1: Seedling stem and leaves (phases 1)
        if (animationPhase == 1)
        {
            double leafProgress = phaseTimer / 60.0;

            int leafDrawX = static_cast<int>((seedX + drawOffsetX) * zoomScale - (zoomScale - 1.0) * screenWidth / 2);

            // 🔥 FIX: interpolate Y from seed position to ground level
            double baseY = seedY + leafProgress * (groundLevel - seedY);

            int leafDrawY = static_cast<int>((baseY + drawOffsetY) * zoomScale - (zoomScale - 1.0) * screenHeight / 2);

            drawSeedlingLeaves(leafDrawX, leafDrawY, leafProgress);
        }

        // Stage 2-3: Tree (overlaps with leaves at start of phase 2 for smooth transition)
        if (animationPhase >= 2 || (animationPhase == 1 && phaseTimer > 50))
        {
            // Calculate blend factor for smooth transition
            double blendFactor = 1.0;
            if (animationPhase == 1)
            {
                blendFactor = (phaseTimer - 50) / 10.0; // Fade in tree during last 10 frames of leaf stage
            }

            int startX = static_cast<int>((seedX + drawOffsetX) * zoomScale - (zoomScale - 1.0) * screenWidth / 2);
            int startY = static_cast<int>((groundLevel + drawOffsetY) * zoomScale - (zoomScale - 1.0) * screenHeight / 2);
            int trunkLength = static_cast<int>(150 * zoomScale);
            double initialAngle = 3.14159 / 2;

            // Use actual treeGrowthScale which transitions smoothly
            if (treeGrowthScale > 0.01)
            {
                drawBranch(startX, startY, trunkLength, initialAngle, 8, treeGrowthScale * blendFactor, treeGrowthScale * blendFactor);
            }
        }

        // Draw falling seed - zoomed and centered
        for (const auto &seed : fallingSeeds)
        {
            if (seed.active)
            {
                // Center the seed on screen during zoom
                int centerX = screenWidth / 2;
                int centerY = screenHeight / 2;

                // Draw massive rotating seed at screen center - USE seed.angle!
                drawSeed(centerX, centerY, seed.angle, zoomScale * 2.0);
            }
        }

        displayPhaseInfo();
        setvisualpage(getactivepage());
    }

    void run()
    {
        initialize();

        while (true)
        {
            // Check for keyboard input
            if (kbhit())
            {
                char key = getch();
                if (key == 27)
                    break; // ESC to exit
                if (key == ' ')
                    resetAnimation(); // SPACE to restart
            }

            // Update animation state
            update();

            // Render frame
            render();

            // Control frame rate (~30 FPS)
            // Sleep(33);  // Windows Sleep uses milliseconds
            delay(33);
        }

        closegraph();
    }
};

int main()
{
    AnimatedTreeDrawer drawer;
    drawer.run();

    return 0;
}