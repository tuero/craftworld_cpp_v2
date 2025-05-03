// pycraftworld.cpp
// Python bindings

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "craftworld/craftworld.h"

namespace py = pybind11;

PYBIND11_MODULE(pycraftworld, m) {
    m.doc() = "CraftWorld environment module docs.";
    namespace cw = ::craftworld;
    using T = cw::CraftWorldGameState;
    py::enum_<cw::Element>(m, "Element")
        .value("kAgent", cw::Element::kAgent)
        .value("kWall", cw::Element::kWall)
        .value("kWorkshop1", cw::Element::kWorkshop1)
        .value("kWorkshop2", cw::Element::kWorkshop2)
        .value("kWorkshop3", cw::Element::kWorkshop3)
        .value("kFurnace", cw::Element::kFurnace)
        .value("kWater", cw::Element::kWater)
        .value("kStone", cw::Element::kStone)
        .value("kIron", cw::Element::kIron)
        .value("kTin", cw::Element::kTin)
        .value("kCopper", cw::Element::kCopper)
        .value("kWood", cw::Element::kWood)
        .value("kGrass", cw::Element::kGrass)
        .value("kGold", cw::Element::kGold)
        .value("kGem", cw::Element::kGem)
        .value("kBronzeBar", cw::Element::kBronzeBar)
        .value("kStick", cw::Element::kStick)
        .value("kPlank", cw::Element::kPlank)
        .value("kRope", cw::Element::kRope)
        .value("kNails", cw::Element::kNails)
        .value("kBronzeHammer", cw::Element::kBronzeHammer)
        .value("kBronzePick", cw::Element::kBronzePick)
        .value("kBridge", cw::Element::kBridge)
        .value("kIronPick", cw::Element::kIronPick)
        .value("kGoldBar", cw::Element::kGoldBar)
        .value("kGemRing", cw::Element::kGemRing)
        .value("kEmpty", cw::Element::kEmpty);
    py::enum_<cw::RewardCode>(m, "RewardCode")
        .value("kRewardCodeCraftBronzeBar", cw::RewardCode::kRewardCodeCraftBronzeBar)
        .value("kRewardCodeCraftStick", cw::RewardCode::kRewardCodeCraftStick)
        .value("kRewardCodeCraftPlank", cw::RewardCode::kRewardCodeCraftPlank)
        .value("kRewardCodeCraftRope", cw::RewardCode::kRewardCodeCraftRope)
        .value("kRewardCodeCraftNails", cw::RewardCode::kRewardCodeCraftNails)
        .value("kRewardCodeCraftBronzeHammer", cw::RewardCode::kRewardCodeCraftBronzeHammer)
        .value("kRewardCodeCraftBronzePick", cw::RewardCode::kRewardCodeCraftBronzePick)
        .value("kRewardCodeCraftIronPick", cw::RewardCode::kRewardCodeCraftIronPick)
        .value("kRewardCodeCraftBridge", cw::RewardCode::kRewardCodeCraftBridge)
        .value("kRewardCodeCraftGoldBar", cw::RewardCode::kRewardCodeCraftGoldBar)
        .value("kRewardCodeCraftGemRing", cw::RewardCode::kRewardCodeCraftGemRing)
        .value("kRewardCodeUseAxe", cw::RewardCode::kRewardCodeUseAxe)
        .value("kRewardCodeUseBridge", cw::RewardCode::kRewardCodeUseBridge)
        .value("kRewardCodeCollectTin", cw::RewardCode::kRewardCodeCollectTin)
        .value("kRewardCodeCollectCopper", cw::RewardCode::kRewardCodeCollectCopper)
        .value("kRewardCodeCollectWood", cw::RewardCode::kRewardCodeCollectWood)
        .value("kRewardCodeCollectGrass", cw::RewardCode::kRewardCodeCollectGrass)
        .value("kRewardCodeCollectIron", cw::RewardCode::kRewardCodeCollectIron)
        .value("kRewardCodeCollectGold", cw::RewardCode::kRewardCodeCollectGold)
        .value("kRewardCodeCollectGem", cw::RewardCode::kRewardCodeCollectGem)
        .value("kRewardCodeUseAtWorkstation1", cw::RewardCode::kRewardCodeUseAtWorkstation1)
        .value("kRewardCodeUseAtWorkstation2", cw::RewardCode::kRewardCodeUseAtWorkstation2)
        .value("kRewardCodeUseAtWorkstation3", cw::RewardCode::kRewardCodeUseAtWorkstation3)
        .value("kRewardCodeUseAtFurnace", cw::RewardCode::kRewardCodeUseAtFurnace);

    py::class_<T>(m, "CraftWorldGameState")
        .def(py::init<const std::string &>())
        .def_readonly_static("name", &T::name)
        .def_readonly_static("num_actions", &craftworld::kNumActions)
        .def(py::self == py::self)    // NOLINT (misc-redundant-expression)
        .def(py::self != py::self)    // NOLINT (misc-redundant-expression)
        .def("__hash__", [](const T &self) { return self.get_hash(); })
        .def("__copy__", [](const T &self) { return T(self); })
        .def("__deepcopy__", [](const T &self, py::dict) { return T(self); })
        .def("__repr__",
             [](const T &self) {
                 std::stringstream stream;
                 stream << self;
                 return stream.str();
             })
        .def(py::pickle(
            [](const T &self) {    // __getstate__
                auto s = self.pack();
                return py::make_tuple(s.rows, s.cols, s.agent_idx, s.grid, s.goal, s.reward_signal, s.hash,
                                      s.inventory);
            },
            [](py::tuple t) -> T {    // __setstate__
                if (t.size() != 8) {
                    throw std::runtime_error("Invalid state");
                }
                T::InternalState s;
                s.rows = t[0].cast<int>();                                  // NOLINT(*-magic-numbers)
                s.cols = t[1].cast<int>();                                  // NOLINT(*-magic-numbers)
                s.agent_idx = t[2].cast<int>();                             // NOLINT(*-magic-numbers)
                s.grid = t[3].cast<std::vector<int>>();                     // NOLINT(*-magic-numbers)
                s.goal = t[4].cast<int>();                                  // NOLINT(*-magic-numbers)
                s.reward_signal = t[5].cast<uint64_t>();                    // NOLINT(*-magic-numbers)
                s.hash = t[6].cast<uint64_t>();                             // NOLINT(*-magic-numbers)
                s.inventory = t[7].cast<std::unordered_map<int, int>>();    // NOLINT(*-magic-numbers)
                return {std::move(s)};
            }))
        .def("apply_action",
             [](T &self, int action) {
                 if (action < 0 || action >= T::action_space_size()) {
                     throw std::invalid_argument("Invalid action.");
                 }
                 self.apply_action(static_cast<craftworld::Action>(action));
             })
        .def("is_solution", &T::is_solution)
        .def("observation_shape", &T::observation_shape)
        .def("get_observation",
             [](const T &self) {
                 py::array_t<float> out = py::cast(self.get_observation());
                 return out.reshape(self.observation_shape());
             })
        .def("image_shape", &T::image_shape)
        .def("to_image",
             [](T &self) {
                 py::array_t<uint8_t> out = py::cast(self.to_image());
                 const auto obs_shape = self.observation_shape();
                 return out.reshape({static_cast<py::ssize_t>(obs_shape[1] * craftworld::SPRITE_HEIGHT),
                                     static_cast<py::ssize_t>(obs_shape[2] * craftworld::SPRITE_WIDTH),
                                     static_cast<py::ssize_t>(craftworld::SPRITE_CHANNELS)});
             })
        .def("get_reward_signal", &T::get_reward_signal)
        .def("get_agent_index", &T::get_agent_index)
        .def("get_indices", &T::get_indices)
        .def("add_to_inventory", &T::add_to_inventory)
        .def("check_inventory", &T::check_inventory);
}
