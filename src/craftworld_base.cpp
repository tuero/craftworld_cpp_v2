
#include "craftworld_base.h"

#include <cstdint>
#include <sstream>
#include <type_traits>

#include "definitions.h"

namespace craftworld {

namespace {
[[noreturn]] inline void unreachable() {
    // Uses compiler specific extensions if possible.
    // Even if no extension is used, undefined behavior is still raised by
    // an empty function body and the noreturn attribute.
#if defined(_MSC_VER) && !defined(__clang__)    // MSVC
    __assume(false);
#else    // GCC, Clang
    __builtin_unreachable();
#endif
}

constexpr uint64_t SPLIT64_S1 = 30;
constexpr uint64_t SPLIT64_S2 = 27;
constexpr uint64_t SPLIT64_S3 = 31;
constexpr uint64_t SPLIT64_C1 = 0x9E3779B97f4A7C15;
constexpr uint64_t SPLIT64_C2 = 0xBF58476D1CE4E5B9;
constexpr uint64_t SPLIT64_C3 = 0x94D049BB133111EB;

template <class E>
constexpr inline auto to_underlying(E e) noexcept -> std::underlying_type_t<E> {
    return static_cast<std::underlying_type_t<E>>(e);
}

auto to_local_hash(int flat_size, Element el, int offset) noexcept -> uint64_t {
    uint64_t seed = (flat_size * to_underlying(el)) + offset;
    uint64_t result = seed + SPLIT64_C1;
    result = (result ^ (result >> SPLIT64_S1)) * SPLIT64_C2;
    result = (result ^ (result >> SPLIT64_S2)) * SPLIT64_C3;
    return result ^ (result >> SPLIT64_S3);
}

auto to_local_inventory_hash(int flat_size, Element el, int count) noexcept -> uint64_t {
    uint64_t seed = (flat_size * kNumElements) + (flat_size * to_underlying(el)) + count;    // NOLINT(*-magic-numbers)
    uint64_t result = seed + SPLIT64_C1;
    result = (result ^ (result >> SPLIT64_S1)) * SPLIT64_C2;
    result = (result ^ (result >> SPLIT64_S2)) * SPLIT64_C3;
    return result ^ (result >> SPLIT64_S3);
}
}    // namespace

CraftWorldGameState::CraftWorldGameState(const std::string &board_str) {
    std::stringstream board_ss(board_str);
    std::string segment;
    std::vector<std::string> seglist;
    // string split on |
    while (std::getline(board_ss, segment, '|')) {
        seglist.push_back(segment);
    }
    if (seglist.size() < 4) {
        throw std::invalid_argument("Board string should have at minimum 4 values separated by '|'.");
    }

    // Get general info
    rows = std::stoi(seglist[0]);
    cols = std::stoi(seglist[1]);
    int _goal = std::stoi(seglist[2]);
    if (seglist.size() != static_cast<std::size_t>(rows * cols) + 3) {
        throw std::invalid_argument("Supplied rows/cols does not match input board length.");
    }
    if (_goal < kPrimitiveStart || _goal >= (kNumPrimitive + kNumRecipeTypes + kPrimitiveStart)) {
        throw std::invalid_argument("Unknown goal element.");
    }
    goal = static_cast<Element>(_goal);

    // Parse grid
    for (std::size_t i = 3; i < seglist.size(); ++i) {
        int el_idx = std::stoi(seglist[i]);
        if (el_idx < 0 || el_idx > kNumElements) {
            throw std::invalid_argument(std::string("Unknown element type: ") + seglist[i]);
        }

        if (static_cast<Element>(el_idx) == Element::kAgent) {
            agent_idx = static_cast<int>(i) - 3;
        }

        grid.push_back(static_cast<Element>(el_idx));
    }
    assert(static_cast<int>(grid.size()) == rows * cols);

    // Set initial hash for game world
    int flat_size = rows * cols;
    hash = 0;
    for (int i = 0; i < flat_size; ++i) {
        hash ^= to_local_hash(flat_size, grid.at(i), i);
    }
}

CraftWorldGameState::CraftWorldGameState(InternalState &&internal_state)
    : rows(internal_state.rows),
      cols(internal_state.cols),
      agent_idx(internal_state.agent_idx),
      goal(static_cast<Element>(internal_state.goal)),
      reward_signal(internal_state.reward_signal),
      hash(internal_state.hash) {
    grid.clear();
    for (const auto &el : internal_state.grid) {
        grid.push_back(static_cast<Element>(el));
    }
    inventory.clear();
    for (const auto &[el, count] : internal_state.inventory) {
        inventory[static_cast<Element>(el)] = count;
    }
}

auto CraftWorldGameState::operator==(const CraftWorldGameState &other) const noexcept -> bool {
    return rows == other.rows && cols == other.cols && agent_idx == other.agent_idx && grid == other.grid &&
           goal == other.goal && inventory == other.inventory;
}

auto CraftWorldGameState::operator!=(const CraftWorldGameState &other) const noexcept -> bool {
    return !(*this == other);
}

// ---------------------------------------------------------------------------

void CraftWorldGameState::RemoveItemFromBoard(int index) noexcept {
    Element el = grid.at(index);
    auto flat_size = rows * cols;
    hash ^= to_local_hash(flat_size, el, index);
    grid.at(index) = Element::kEmpty;
    hash ^= to_local_hash(flat_size, Element::kEmpty, index);
}

void CraftWorldGameState::HandleAgentMovement(Action action) noexcept {
    // Move if in bound and empty tile
    auto new_idx = IndexFromAction(agent_idx, action);
    int flat_size = rows * cols;
    if (InBounds(agent_idx, action) && grid.at(new_idx) == Element::kEmpty) {
        // Undo hash
        hash ^= to_local_hash(flat_size, Element::kAgent, agent_idx);
        hash ^= to_local_hash(flat_size, Element::kEmpty, new_idx);
        // Change hash
        hash ^= to_local_hash(flat_size, Element::kAgent, new_idx);
        hash ^= to_local_hash(flat_size, Element::kEmpty, agent_idx);
        // Move
        grid.at(new_idx) = Element::kAgent;
        grid.at(agent_idx) = Element::kEmpty;
        agent_idx = new_idx;
    }
}

void CraftWorldGameState::HandleAgentUse() noexcept {
    for (const auto &action : kAllActions) {
        if (!InBounds(agent_idx, action)) {
            continue;
        }
        int neighbour_idx = IndexFromAction(agent_idx, action);
        // Nothing on this index to do something
        if (grid.at(neighbour_idx) == Element::kEmpty) {
            continue;
        }

        if (IsPrimitive(neighbour_idx)) {
            // Primitive elements on map are collectable, add to inventory
            const Element el = grid.at(neighbour_idx);
            if (el != Element::kGrass) {
                AddToInventory(el, 1);
            }
            RemoveItemFromBoard(neighbour_idx);
            reward_signal |= static_cast<std::underlying_type_t<RewardCode>>(kPrimitiveRewardMap.at(el));
            break;
        } else if (grid.at(neighbour_idx) == Element::kIron && HasItemInInventory(Element::kBronzePick)) {
            // Iron ingot is special primitive where we need a cobble stone pickaxe to gather
            const Element el = grid.at(neighbour_idx);
            AddToInventory(el, 1);
            RemoveItemFromBoard(neighbour_idx);
            reward_signal |= static_cast<std::underlying_type_t<RewardCode>>(kPrimitiveRewardMap.at(el));
            break;
        } else if (IsWorkShop(neighbour_idx)) {
            const Element el_workshop = grid.at(neighbour_idx);
            for (const auto &[recipe_type, recipe_item] : kRecipeMap) {
                // Skip recipes not legal at this workshop
                const auto recipe_workshop = recipe_item.location;
                if (recipe_workshop != el_workshop) {
                    continue;
                }
                // Skip if we don't have the items required in our inventory
                if (!CanCraftItem(recipe_item)) {
                    continue;
                }

                // Add crafted item and remove ingredients from inventory
                AddToInventory(recipe_item.output, 1);
                for (auto const &ingredient_item : recipe_item.inputs) {
                    RemoveFromInventory(ingredient_item.element, ingredient_item.count);
                }
                reward_signal |=
                    static_cast<std::underlying_type_t<RewardCode>>(kRecipeRewardMap.at(recipe_item.recipe));
                reward_signal |= static_cast<std::underlying_type_t<RewardCode>>(kWorkstationRewardMap.at(el_workshop));
                break;
            }
            break;
        } else if (IsItem(neighbour_idx, Element::kWater)) {
            // Remove water with a bridge
            if (HasItemInInventory(Element::kBridge)) {
                RemoveFromInventory(Element::kBridge, 1);
                RemoveItemFromBoard(neighbour_idx);
                reward_signal |= static_cast<std::underlying_type_t<RewardCode>>(RewardCode::kRewardCodeUseBridge);
                break;
            }
        } else if (IsItem(neighbour_idx, Element::kStone)) {
            // Remove stone with an axe
            if (HasItemInInventory(Element::kIronPick)) {
                RemoveFromInventory(Element::kIronPick, 1);
                RemoveItemFromBoard(neighbour_idx);
                reward_signal |= static_cast<std::underlying_type_t<RewardCode>>(RewardCode::kRewardCodeUseAxe);
                break;
            }
        }
    }
}

void CraftWorldGameState::apply_action(Action action) {
    assert(is_valid_action(action));
    reward_signal = 0;
    if (action == Action::kUse) {
        HandleAgentUse();
    } else {
        HandleAgentMovement(action);
    }
}

auto CraftWorldGameState::is_solution() const noexcept -> bool {
    // Inventory contains the goal item
    return inventory.find(goal) != inventory.end();
}

auto CraftWorldGameState::observation_shape() const noexcept -> std::array<int, 3> {
    // pad boarder with inventory items
    return {kNumElements, rows + 4, cols + 4};
}

auto CraftWorldGameState::get_observation() const noexcept -> std::vector<float> {
    const auto rows_obs = rows + 4;
    const auto cols_obs = cols + 4;
    const auto channel_length = rows_obs * cols_obs;

    std::vector<float> obs(kNumElements * channel_length, 0);

    // Inner border is wall
    for (int w = 1; w < cols_obs - 1; ++w) {
        const auto channel = static_cast<std::size_t>(Element::kWall);
        obs[channel * channel_length + (1 * cols_obs + w)] = 1;
        obs[channel * channel_length + ((rows_obs - 2) * cols_obs + w)] = 1;
    }
    for (int h = 1; h < rows_obs - 1; ++h) {
        const auto channel = static_cast<std::size_t>(Element::kWall);
        obs[channel * channel_length + (h * cols_obs + 1)] = 1;
        obs[channel * channel_length + (h * cols_obs + (cols_obs - 2))] = 1;
    }

    // Outer border is empty (for now, dont forget to undo for inventory items)
    for (int w = 0; w < cols_obs; ++w) {
        const auto channel = static_cast<std::size_t>(Element::kEmpty);
        obs[channel * channel_length + (0 * cols_obs + w)] = 1;
        obs[channel * channel_length + ((rows_obs - 1) * cols_obs + w)] = 1;
    }
    for (int h = 1; h < rows_obs - 1; ++h) {
        const auto channel = static_cast<std::size_t>(Element::kEmpty);
        obs[channel * channel_length + (h * cols_obs + 0)] = 1;
        obs[channel * channel_length + (h * cols_obs + (cols_obs - 1))] = 1;
    }

    // Board environment + primitives + agent
    std::size_t i = 0;
    for (int r = 2; r < rows_obs - 2; ++r) {
        for (int c = 2; c < cols_obs - 2; ++c) {
            const auto el = grid.at(i);
            auto idx = (r * cols_obs) + c;
            obs.at(static_cast<std::size_t>(el) * channel_length + idx) = 1;
            ++i;
        }
    }

    // Inventory (fill around the border)
    for (const auto &[inv_el, inv_count] : inventory) {
        switch (inv_el) {
            case Element::kWood:
                obs.at(static_cast<std::size_t>(inv_el) * channel_length + 0) = 1;
                obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 0) = 0;
                if (inv_count > 1) {
                    obs.at(static_cast<std::size_t>(inv_el) * channel_length + 1) = 1;
                    obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 1) = 0;
                }
                break;
            case Element::kCopper:
                obs.at(static_cast<std::size_t>(inv_el) * channel_length + 2) = 1;
                obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 2) = 0;
                break;
            case Element::kTin:
                obs.at(static_cast<std::size_t>(inv_el) * channel_length + 3) = 1;
                obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 3) = 0;
                break;
            case Element::kIron:
                obs.at(static_cast<std::size_t>(inv_el) * channel_length + 4) = 1;
                obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 4) = 0;
                break;
            case Element::kStick:
                obs.at(static_cast<std::size_t>(inv_el) * channel_length + 5) = 1;
                obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 5) = 0;
                if (inv_count > 1) {
                    obs.at(static_cast<std::size_t>(inv_el) * channel_length + 6) = 1;
                    obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 6) = 0;
                }
                break;
            case Element::kBronzeBar:
                obs.at(static_cast<std::size_t>(inv_el) * channel_length + 7) = 1;
                obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 7) = 0;
                break;
            case Element::kBronzePick:
                obs.at(static_cast<std::size_t>(inv_el) * channel_length + 8) = 1;
                obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 8) = 0;
                break;
            case Element::kIronPick:
                obs.at(static_cast<std::size_t>(inv_el) * channel_length + 9) = 1;
                obs.at(static_cast<std::size_t>(Element::kEmpty) * channel_length + 9) = 0;
                break;
            default:
                break;
        }
    }
    return obs;
}

// Spite assets
#include "assets_all.inc"
namespace {
void fill_sprite(std::vector<uint8_t> &img, const std::vector<uint8_t> &sprite_data, std::size_t h, std::size_t w,
                 std::size_t cols) {
    const std::size_t img_idx_top_left = h * (SPRITE_DATA_LEN * cols) + (w * SPRITE_DATA_LEN_PER_ROW);
    for (std::size_t r = 0; r < SPRITE_HEIGHT; ++r) {
        for (std::size_t c = 0; c < SPRITE_WIDTH; ++c) {
            const std::size_t data_idx = (r * SPRITE_DATA_LEN_PER_ROW) + (3 * c);
            const std::size_t img_idx = (r * SPRITE_DATA_LEN_PER_ROW * cols) + (SPRITE_CHANNELS * c) + img_idx_top_left;
            img[img_idx + 0] = sprite_data[data_idx + 0];
            img[img_idx + 1] = sprite_data[data_idx + 1];
            img[img_idx + 2] = sprite_data[data_idx + 2];
        }
    }
}
}    // namespace

auto CraftWorldGameState::image_shape() const noexcept -> std::array<int, 3> {
    const auto r = rows + 4;
    const auto c = cols + 4;
    return {r * SPRITE_HEIGHT, c * SPRITE_WIDTH, SPRITE_CHANNELS};
}

auto CraftWorldGameState::to_image() const noexcept -> std::vector<uint8_t> {
    // Pad board with black border
    const auto rows_img = rows + 4;
    const auto cols_img = cols + 4;
    const auto channel_length = rows_img * cols_img;
    std::vector<uint8_t> img(channel_length * SPRITE_DATA_LEN, 0);

    // Inner border is wall
    for (int w = 1; w < cols_img - 1; ++w) {
        fill_sprite(img, img_asset_map.at(Element::kWall), 1, w, cols_img);
        fill_sprite(img, img_asset_map.at(Element::kWall), rows_img - 2, w, cols_img);
    }
    for (int h = 1; h < rows_img - 1; ++h) {
        fill_sprite(img, img_asset_map.at(Element::kWall), h, 1, cols_img);
        fill_sprite(img, img_asset_map.at(Element::kWall), h, cols_img - 2, cols_img);
    }

    // Outer border is inventory
    std::vector<std::pair<std::size_t, std::size_t>> indices;
    for (int w = 0; w < cols_img; ++w) {
        indices.emplace_back(0, w);
    }
    for (int w = 0; w < cols_img; ++w) {
        indices.emplace_back(rows_img - 1, w);
    }
    std::size_t inv_idx = 0;
    for (const auto &[inv_item, inv_count] : inventory) {
        for (int i = 0; i < inv_count; ++i) {
            fill_sprite(img, img_asset_map.at(inv_item), indices[inv_idx].first, indices[inv_idx].second, cols_img);
            ++inv_idx;
        }
    }

    // Reset of board is inside the border
    std::size_t board_idx = 0;
    for (int h = 2; h < rows_img - 2; ++h) {
        for (int w = 2; w < cols_img - 2; ++w) {
            const auto el = grid[board_idx];
            fill_sprite(img, img_asset_map.at(el), h, w, cols_img);
            ++board_idx;
        }
    }
    return img;
}

auto CraftWorldGameState::get_reward_signal() const noexcept -> uint64_t {
    return reward_signal;
}

auto CraftWorldGameState::get_hash() const noexcept -> uint64_t {
    return hash;
}

void CraftWorldGameState::add_to_inventory(Element element, int count) {
    if (!is_valid_element(element)) {
        throw std::invalid_argument("Unknown element type.");
    }
    AddToInventory(element, count);
}

int CraftWorldGameState::check_inventory(Element element) const {
    if (inventory.find(element) == inventory.end()) {
        return 0;
    }
    return inventory.at(element);
}

auto CraftWorldGameState::get_agent_index() const noexcept -> int {
    return agent_idx;
}

auto CraftWorldGameState::get_indices(Element element) const noexcept -> std::vector<int> {
    assert(is_valid_element(element));
    std::vector<int> indices;
    for (int index = 0; index < rows * cols; ++index) {
        if (grid.at(index) == element) {
            indices.push_back(index);
        }
    }
    return indices;
}

std::ostream &operator<<(std::ostream &os, const CraftWorldGameState &state) {
    for (int w = 0; w < state.cols + 2; ++w) {
        os << "-";
    }
    os << std::endl;
    for (int h = 0; h < state.rows; ++h) {
        os << "|";
        for (int w = 0; w < state.cols; ++w) {
            auto idx = h * state.cols + w;
            os << kElementToSymbolMap.at(state.grid[idx]);
        }
        os << "|" << std::endl;
    }
    for (int w = 0; w < state.cols + 2; ++w) {
        os << "-";
    }
    os << std::endl;
    os << "Goal: " << kElementToNameMap.at(state.goal) << std::endl;
    os << "Inventory: ";
    for (const auto &[inv_item, inv_count] : state.inventory) {
        os << "(" << kElementToNameMap.at(inv_item) << ", " << inv_count << ") ";
    }
    return os;
}

// ---------------------------------------------------------------------------

auto CraftWorldGameState::IndexFromAction(int index, Action action) const noexcept -> int {
    switch (action) {
        case Action::kUp:
            return index - cols;
        case Action::kRight:
            return index + 1;
        case Action::kDown:
            return index + cols;
        case Action::kLeft:
            return index - 1;
        case Action::kUse:
            return index;
        default:
            unreachable();
    }
}

auto CraftWorldGameState::InBounds(int index, Action action) const noexcept -> bool {
    int col = index % cols;
    int row = (index - col) / cols;
    const std::pair<int, int> &offsets = kDirectionOffsets.at(static_cast<std::size_t>(action));
    col += offsets.first;
    row += offsets.second;
    return col >= 0 && col < cols && row >= 0 && row < rows;
}

auto CraftWorldGameState::IsWorkShop(int index) const noexcept -> bool {
    return kWorkShops.find(grid.at(static_cast<std::size_t>(index))) != kWorkShops.end();
}

auto CraftWorldGameState::IsPrimitive(int index) const noexcept -> bool {
    return kPrimitives.find(grid.at(static_cast<std::size_t>(index))) != kPrimitives.end();
}

auto CraftWorldGameState::IsItem(int index, Element element) const noexcept -> bool {
    return grid.at(static_cast<std::size_t>(index)) == element;
}

auto CraftWorldGameState::HasItemInInventory(Element element, int min_count) const noexcept -> bool {
    return inventory.find(element) != inventory.end() && inventory.at(element) >= min_count;
}

void CraftWorldGameState::RemoveFromInventory(Element element, int count) noexcept {
    // Caller needs to verify that we can remove from inventory
    // Decrement item `count` times and change game state hash
    assert(inventory.find(element) != inventory.end());
    assert(inventory.at(element) >= count);
    int flat_size = rows * cols;
    for (int i = 0; i < count; ++i) {
        hash ^= to_local_inventory_hash(flat_size, element, inventory.at(element));
        --inventory.at(element);
    }
    if (inventory.at(element) <= 0) {
        inventory.erase(element);
    }
}

void CraftWorldGameState::AddToInventory(Element element, int count) noexcept {
    // Increment item `count` times and change game state hash
    int flat_size = rows * cols;
    for (int i = 0; i < count; ++i) {
        ++inventory[element];
        hash ^= to_local_inventory_hash(flat_size, element, inventory.at(element));
    }
}

auto CraftWorldGameState::CanCraftItem(RecipeItem recipe_item) const noexcept -> bool {
    for (auto const &ingredient_item : recipe_item.inputs) {
        if (!HasItemInInventory(ingredient_item.element, ingredient_item.count)) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------

}    // namespace craftworld
