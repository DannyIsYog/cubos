#include <cubos/core/gl/grid.hpp>
#include <cubos/core/gl/palette.hpp>
#include <cubos/core/log.hpp>

#include <unordered_map>

using namespace cubos::core::gl;

Grid::Grid(const glm::uvec3& size)
{
    if (size.x < 1 || size.y < 1 || size.z < 1)
    {
        logWarning("Grid size must be at least 1 in each dimension: was ({}, {}, {}), defaulting to (1, 1, 1).", size.x,
                   size.y, size.z);
        this->size = {1, 1, 1};
    }
    else
        this->size = size;
    this->indices.resize(this->size.x * this->size.y * this->size.z, 0);
}

Grid::Grid(const glm::uvec3& size, const std::vector<uint16_t>& indices)
{
    if (size.x < 1 || size.y < 1 || size.z < 1)
    {
        logWarning("Grid size must be at least 1 in each dimension: was ({}, {}, {}), defaulting to (1, 1, 1).", size.x,
                   size.y, size.z);
        this->size = {1, 1, 1};
    }
    else if (indices.size() != size.x * size.y * size.z)
    {
        logWarning("Grid size and indices size mismatch: was ({}, {}, {}), indices size is {}.", size.x, size.y, size.z,
                   indices.size());
        this->size = {1, 1, 1};
    }
    else
        this->size = size;
    this->indices = indices;
}

Grid::Grid(Grid&& other) : size(other.size)
{
    new (&this->indices) std::vector<uint16_t>(std::move(other.indices));
}

Grid::Grid()
{
    this->size = {1, 1, 1};
    this->indices.resize(1, 0);
}

Grid& Grid::operator=(const Grid& rhs)
{
    this->size = rhs.size;
    this->indices = rhs.indices;

    return *this;
}

void Grid::setSize(const glm::uvec3& size)
{
    if (size == this->size)
        return;
    else if (size.x < 1 || size.y < 1 || size.z < 1)
    {
        logWarning("Grid size must be at least 1 in each dimension: preserving original dimensions (tried to set to "
                   "({}, {}, {}))",
                   size.x, size.y, size.z);
        return;
    }

    this->size = size;
    this->indices.clear();
    this->indices.resize(this->size.x * this->size.y * this->size.z, 0);
}

const glm::uvec3& Grid::getSize() const
{
    return size;
}

void Grid::clear()
{
    for (size_t i = 0; i < this->indices.size(); i++)
        this->indices[i] = 0;
}

uint16_t Grid::get(const glm::ivec3& position) const
{
    assert(position.x >= 0 && position.x < this->size.x && position.y >= 0 && position.y < this->size.y &&
           position.z >= 0 && position.z < this->size.z);
    return this->indices[position.x + position.y * size.x + position.z * size.x * size.y];
}

void Grid::set(const glm::ivec3& position, uint16_t mat)
{
    assert(position.x >= 0 && position.x < this->size.x && position.y >= 0 && position.y < this->size.y &&
           position.z >= 0 && position.z < this->size.z);
    this->indices[position.x + position.y * size.x + position.z * size.x * size.y] = mat;
}

bool Grid::convert(const Palette& src, const Palette& dst, float min_similarity)
{
    // Find the mappings for every material in the source palette.
    std::unordered_map<uint16_t, uint16_t> mappings;
    for (uint16_t i = 0; i <= src.getSize(); ++i)
    {
        uint16_t j = dst.find(src.get(i));
        if (src.get(i).similarity(dst.get(j)) >= min_similarity)
        {
            mappings[i] = j;
        }
    }

    // Check if the mappings are complete for every material being used in the grid.
    for (uint32_t i = 0; i < this->size.x * this->size.y * this->size.z; ++i)
    {
        if (mappings.find(this->indices[i]) == mappings.end())
        {
            return false;
        }
    }

    // Apply the mappings.
    for (uint32_t i = 0; i < this->size.x * this->size.y * this->size.z; ++i)
    {
        this->indices[i] = mappings[this->indices[i]];
    }

    return true;
}

void cubos::core::data::serialize(Serializer& serializer, const gl::Grid& grid, const char* name)
{
    serializer.beginObject(name);
    serializer.write(grid.size, "size");
    serializer.write(grid.indices, "data");
    serializer.endObject();
}

void cubos::core::data::deserialize(Deserializer& deserializer, gl::Grid& grid)
{
    deserializer.beginObject();
    deserializer.read(grid.size);
    deserializer.read(grid.indices);
    deserializer.endObject();

    if (grid.size.x * grid.size.y * grid.size.z != static_cast<int>(grid.indices.size()))
    {
        logWarning(
            "Could not deserialize grid: grid size and indices size mismatch: was ({}, {}, {}), indices size is {}.",
            grid.size.x, grid.size.y, grid.size.z, grid.indices.size());
        grid.size = {1, 1, 1};
        grid.indices.clear();
        grid.indices.resize(1, 0);
    }
}
