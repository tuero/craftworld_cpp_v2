#ifndef CRAFTWORLD_BASE_H_
#define CRAFTWORLD_BASE_H_

#include <array>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>

#include "definitions.h"

namespace craftworld {

// Image properties
constexpr int SPRITE_WIDTH = 32;
constexpr int SPRITE_HEIGHT = 32;
constexpr int SPRITE_CHANNELS = 3;
constexpr int SPRITE_DATA_LEN_PER_ROW = SPRITE_WIDTH * SPRITE_CHANNELS;
constexpr int SPRITE_DATA_LEN = SPRITE_WIDTH * SPRITE_HEIGHT * SPRITE_CHANNELS;

// Game state
class CraftWorldGameState {
public:
    // Internal use for packing/unpacking with pybind11 pickle
    struct InternalState {
        int rows;
        int cols;
        int agent_idx;
        std::vector<int> grid;
        int goal;
        uint64_t reward_signal = 0;
        uint64_t hash = 0;
        std::unordered_map<int, int> inventory;
    };

    CraftWorldGameState(const std::string &board_str);
    CraftWorldGameState(InternalState &&internal_state);

    auto operator==(const CraftWorldGameState &other) const noexcept -> bool;
    auto operator!=(const CraftWorldGameState &other) const noexcept -> bool;

    static inline std::string name = "craftworld";

    /**
     * Check if the given element is valid.
     * @param element Element to check
     * @return True if element is valid, false otherwise
     */
    [[nodiscard]] constexpr static auto is_valid_element(Element element) -> bool {
        return static_cast<int>(element) >= 0 && static_cast<int>(element) < static_cast<int>(kNumElements);
    }

    /**
     * Check if the given action is valid.
     * @param action Action to check
     * @return True if action is valid, false otherwise
     */
    [[nodiscard]] constexpr static auto is_valid_action(Action action) -> bool {
        return static_cast<int>(action) >= 0 && static_cast<int>(action) < static_cast<int>(kNumActions);
    }

    /**
     * Get the number of possible actions
     * @return Count of possible actions
     */
    [[nodiscard]] constexpr static auto action_space_size() noexcept -> int {
        return kNumActions;
    }

    /**
     * Apply the action to the current state, and set the reward and signals.
     * @param action The action to apply, should be one of the legal actions
     */
    void apply_action(Action action);

    /**
     * Check if the state is in the solution state (agent inside exit).
     * @return True if terminal, false otherwise
     */
    [[nodiscard]] auto is_solution() const noexcept -> bool;

    /**
     * Get the shape the observations should be viewed as.
     * @return vector indicating observation CHW
     */
    [[nodiscard]] auto observation_shape() const noexcept -> std::array<int, 3>;

    /**
     * Get a flat representation of the current state observation.
     * The observation should be viewed as the shape given by observation_shape().
     * @return vector where 1 represents element at position
     */
    [[nodiscard]] auto get_observation() const noexcept -> std::vector<float>;

    /**
     * Get the shape the image should be viewed as.
     * @return array indicating observation HWC
     */
    [[nodiscard]] auto image_shape() const noexcept -> std::array<int, 3>;

    /**
     * Get the flat (HWC) image representation of the current state
     * @return flattened byte vector represending RGB values (HWC)
     */
    [[nodiscard]] auto to_image() const noexcept -> std::vector<uint8_t>;

    /**
     * Get the current reward signal as a result of the previous action taken.
     * @return bit field representing events that occured
     */
    [[nodiscard]] auto get_reward_signal() const noexcept -> uint64_t;

    /**
     * Get the hash representation for the current state.
     * @return hash value
     */
    [[nodiscard]] auto get_hash() const noexcept -> uint64_t;

    /**
     * Add the given element to the inventory
     */
    void add_to_inventory(Element element, int count);

    /**
     * Get the query element count in the inventory
     * @param element Element to query
     * @return element count in inventory
     */
    [[nodiscard]] int check_inventory(Element element) const;

    /**
     * Get the agent index position, even if in exit
     * @return Agent index
     */
    [[nodiscard]] auto get_agent_index() const noexcept -> int;

    /**
     * Get all indices for a given element type
     * @param element The hidden cell type of the element to search for
     * @return flat indices for each instance of element
     */
    [[nodiscard]] auto get_indices(Element element) const noexcept -> std::vector<int>;

    friend auto operator<<(std::ostream &os, const CraftWorldGameState &state) -> std::ostream &;

    [[nodiscard]] auto pack() const -> InternalState {
        std::vector<int> _grid;
        std::unordered_map<int, int> _inventory;
        _grid.reserve(grid.size());
        for (const auto &el : grid) {
            _grid.push_back(static_cast<int>(el));
        }
        for (const auto &[el, count] : inventory) {
            _inventory[static_cast<int>(el)] = count;
        }
        return {
            .rows = rows,
            .cols = cols,
            .agent_idx = agent_idx,
            .grid = _grid,
            .goal = static_cast<int>(goal),
            .reward_signal = reward_signal,
            .hash = hash,
            .inventory = _inventory,
        };
    }

private:
    auto IndexFromAction(int index, Action action) const noexcept -> int;
    auto InBounds(int index, Action action) const noexcept -> bool;
    auto IsWorkShop(int index) const noexcept -> bool;
    auto IsPrimitive(int index) const noexcept -> bool;
    auto IsItem(int index, Element element) const noexcept -> bool;
    auto HasItemInInventory(Element element, int min_count = 1) const noexcept -> bool;
    void RemoveFromInventory(Element element, int count) noexcept;
    void AddToInventory(Element element, int count) noexcept;
    auto CanCraftItem(RecipeItem recipe_item) const noexcept -> bool;
    void HandleAgentMovement(Action action) noexcept;
    void HandleAgentUse() noexcept;
    void RemoveItemFromBoard(int index) noexcept;

    int rows{};
    int cols{};
    int agent_idx{};
    std::vector<Element> grid;
    Element goal;
    uint64_t reward_signal = 0;
    uint64_t hash = 0;
    std::unordered_map<Element, int> inventory;    // Inventory of items
};

}    // namespace craftworld

#endif    // CRAFTWORLD_BASE_H_
