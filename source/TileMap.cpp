#include "TileMap.hpp"
#include "Dungeon2D.hpp"
#include <cmath>
#include <fstream>
#include <random>
#include <sstream>

// Recursively carves EMPTY_TILE for rooms in BSP leaf nodes.
static void carveLeafRooms(const BSPNode* node, std::vector<int>& cells, int width, int height) {
    if (!node) return;
    if (node->isLeaf()) {
        for (int r = node->room.top; r <= node->room.bottom; ++r) {
            for (int c = node->room.left; c <= node->room.right; ++c) {
                if (c >= 0 && c < width && r >= 0 && r < height) {
                    cells[r * width + c] = EMPTY_TILE;
                }
            }
        }
        return;
    }
    carveLeafRooms(node->left.get(), cells, width, height);
    carveLeafRooms(node->right.get(), cells, width, height);
}

// Sizes the matrix and carves rooms and corridors as EMPTY_TILE.
void TileMap::loadFromDungeon(const Dungeon2D& dungeon) {
    m_width = dungeon.width;
    m_height = dungeon.height;
    m_cells.assign(m_width * m_height, WALL_TILE);

    carveLeafRooms(dungeon.root.get(), m_cells, m_width, m_height);

    for (const auto& corridor : dungeon.corridors) {
        for (int r = corridor.top; r <= corridor.bottom; ++r) {
            for (int c = corridor.left; c <= corridor.right; ++c) {
                if (c >= 0 && c < m_width && r >= 0 && r < m_height) {
                    m_cells[r * m_width + c] = EMPTY_TILE;
                }
            }
        }
    }
}

// Saves the matrix to a CSV file row by row without trailing commas.
bool TileMap::saveCSV(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    for (int r = 0; r < m_height; ++r) {
        for (int c = 0; c < m_width; ++c) {
            if (c > 0) file << ',';
            file << m_cells[r * m_width + c];
        }
        file << '\n';
    }
    return file.good();
}

// Loads a CSV file into the matrix; leaves state untouched on failure.
bool TileMap::loadCSV(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::vector<int> loadedCells;
    std::string line;
    int colsCount = -1;
    int rowsCount = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        int currentCols = 0;

        while (std::getline(ss, token, ',')) {
            if (token.empty()) return false;
            try {
                loadedCells.push_back(std::stoi(token));
                ++currentCols;
            } catch (...) {
                return false;
            }
        }

        if (colsCount == -1) {
            colsCount = currentCols;
        } else if (colsCount != currentCols) {
            return false;
        }
        ++rowsCount;
    }

    if (rowsCount == 0 || colsCount <= 0) return false;

    m_width = colsCount;
    m_height = rowsCount;
    m_cells = std::move(loadedCells);
    return true;
}

// Returns matrix width.
int TileMap::width() const noexcept {
    return m_width;
}

// Returns matrix height.
int TileMap::height() const noexcept {
    return m_height;
}

// Returns tile at (col, row). Precondition: indices are in range.
int TileMap::at(int col, int row) const {
    return m_cells[row * m_width + col];
}

// Checks if cell at (col, row) is a wall. Precondition: indices in range.
bool TileMap::isWall(int col, int row) const {
    return m_cells[row * m_width + col] == WALL_TILE;
}

// Returns pointer to raw contiguous cell buffer.
const int* TileMap::data() const noexcept {
    return m_cells.data();
}

// Translates continuous world coordinates into discrete cell indices.
void TileMap::toIndices(const Vector2D& world, int& col, int& row) const noexcept {
    col = static_cast<int>(std::floor(world.x / TILE_SIZE));
    row = static_cast<int>(std::floor(world.y / TILE_SIZE));
}

// Returns the center world coordinates of the given cell.
Vector2D TileMap::toWorld(int col, int row) const noexcept {
    return Vector2D((col + 0.5f) * TILE_SIZE, (row + 0.5f) * TILE_SIZE);
}

// Picks a random EMPTY_TILE cell. Precondition: at least one empty tile exists.
void TileMap::randomEmptyCell(int& col, int& row) const {
    std::vector<int> emptyIndices;
    for (std::size_t i = 0; i < m_cells.size(); ++i) {
        if (m_cells[i] == EMPTY_TILE) {
            emptyIndices.push_back(static_cast<int>(i));
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> dist(0, emptyIndices.size() - 1);

    int selected = emptyIndices[dist(gen)];
    col = selected % m_width;
    row = selected / m_width;
}